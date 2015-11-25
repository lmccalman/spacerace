from flask import Flask, jsonify, request
from helpers import (make_random_name, make_context, make_address,
                     make_control_str)

import zmq

app = Flask(__name__)
app.config.from_object('settings')


# TODO: context and sockets are never closed. Need to determine the best place
context = make_context()

lobby_sock = context.socket(zmq.REQ)
lobby_sock.connect(make_address(app.config.get('SPACERACE_SERVER'),
                                app.config.get('SPACERACE_LOBBY_PORT')))

control_sock = context.socket(zmq.PUSH)
control_sock.connect(make_address(app.config.get('SPACERACE_SERVER'),
                                  app.config.get('SPACERACE_CONTROL_PORT')))


@app.route('/')
def hello_world():
    return jsonify(msg='Hello World!')


@app.route('/lobby')
def lobby():
    name = request.args.get('name', default=make_random_name())
    team = request.args.get('team', default=make_random_name())

    app.logger.debug('Registering "{0}" under team "{1}"'.format(name, team))
    lobby_sock.send_json(dict(name=name, team=team))

    app.logger.debug('Awaiting response from lobby...')
    response = lobby_sock.recv_json()

    return jsonify(response)


@app.route('/control/<string:secret>', methods=['GET'])
def control(secret):
    linear = request.args.get('linear')
    rotation = request.args.get('rotation')

    control_str = make_control_str(secret, linear, rotation)

    app.logger.debug('Sending control "{}"'.format(control_str))
    control_sock.send_string(control_str)

    # Empty response HTTP_202_ACCEPTED code
    return '', 202

if __name__ == '__main__':
    app.run(debug=True)
    lobby_sock.close()
