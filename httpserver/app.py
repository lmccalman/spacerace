from flask import Flask, jsonify, request
from helpers import (make_random_name, make_context, make_address,
                     make_control_str, InvalidUsage)

import json
import zmq

app = Flask(__name__)
app.config.from_object('settings')


# TODO: context and sockets are never closed. Need to determine the best place
# TODO: these should actually be initialized on a per-request basis. question
# is whether the context can be shared.
context = make_context()

lobby_sock = context.socket(zmq.REQ)
lobby_sock.connect(make_address(app.config.get('SPACERACE_SERVER'),
                                app.config.get('SPACERACE_LOBBY_PORT')))

control_sock = context.socket(zmq.PUSH)
control_sock.connect(make_address(app.config.get('SPACERACE_SERVER'),
                                  app.config.get('SPACERACE_CONTROL_PORT')))


state_sock = context.socket(zmq.SUB)
state_sock.setsockopt_string(zmq.SUBSCRIBE, u'')
state_sock.connect(make_address(app.config.get('SPACERACE_SERVER'),
                                app.config.get('SPACERACE_STATE_PORT')))


@app.errorhandler(InvalidUsage)
def handle_invalid_usage(error):
    response = jsonify(error.to_dict())
    response.status_code = error.status_code
    return response


@app.route('/')
def greeting():
    return jsonify(msg='Welcome to spacerace!')


@app.route('/lobby')
def lobby():
    name = request.args.get('name', default=make_random_name())
    team = request.args.get('team', default=make_random_name())

    app.logger.debug('Registering "{}" under team "{}"'.format(name, team))
    lobby_sock.send_json(dict(name=name, team=team))

    app.logger.debug('Awaiting response from lobby...')
    # TODO: Need to set timeout for ZMQ socket here so the HTTP API server
    # doesn't just die if the socket is blocking
    response = lobby_sock.recv_json()

    return jsonify(response)


@app.route('/state')
def state():

    _, msg_b = state_sock.recv_multipart()
    current_state = json.loads(msg_b.decode())

    return jsonify(current_state)


@app.route('/control/<string:secret>', methods=['POST', 'PUT'])
@app.route('/control', methods=['POST', 'PUT'])
def control(secret=None):

    try:

        if secret is None:
            secret = request.json['secret']

        linear = str(request.json['linear'])
        rotation = str(request.json['rotation'])

    except KeyError:
        raise InvalidUsage('One or more required arguments not provided.')

    control_str = make_control_str(secret, linear, rotation)

    app.logger.debug('Sending control message "{0}"'.format(control_str))
    control_sock.send_string(control_str)

    # Response message and HTTP_202_ACCEPTED code
    return jsonify(message='Sent control message "{0}"'.format(control_str))

if __name__ == '__main__':
    app.run(debug=True)
