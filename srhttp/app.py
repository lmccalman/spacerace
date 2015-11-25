from flask import Flask, g, jsonify, request
from helpers import (make_random_name, make_context, make_address,
                     make_control_str)

import zmq

app = Flask(__name__)
app.config.from_object('settings')


# TODO: context and sockets are never closed. Need to determine the best place
# TODO: these should actually be initialized on a per-request basis. question
# is whether the context can be shared.
context = make_context()

# lobby_sock = context.socket(zmq.REQ)
# lobby_sock.connect(make_address(app.config.get('SPACERACE_SERVER'),
#                                 app.config.get('SPACERACE_LOBBY_PORT')))

control_sock = context.socket(zmq.PUSH)
control_sock.connect(make_address(app.config.get('SPACERACE_SERVER'),
                                  app.config.get('SPACERACE_CONTROL_PORT')))


@app.before_request
def before_request():
    g.zmq_context = make_context()


@app.route('/')
def hello_world():
    return jsonify(msg='Hello World!')


@app.route('/lobby')
def lobby():
    name = request.args.get('name', default=make_random_name())
    team = request.args.get('team', default=make_random_name())

    with g.zmq_context.socket(zmq.REQ) as socket:
        socket.connect(make_address(app.config.get('SPACERACE_SERVER'),
                                    app.config.get('SPACERACE_LOBBY_PORT')))

        app.logger.debug('Registering "{0}" under team "{1}"'.format(name, team))
        socket.send_json(dict(name=name, team=team))

        app.logger.debug('Awaiting response from lobby...')
        response = socket.recv_json()

    return jsonify(response)


# TODO: Currently is GET method. Should instead be PUT or maybe POST.
@app.route('/control', defaults={'secret': None})
@app.route('/control/<string:secret>')
def control(secret):

    # print g.zmq_context

    if secret is None:
        secret = request.args.get('secret')

    linear = request.args.get('linear')
    rotation = request.args.get('rotation')

    control_str = make_control_str(secret, linear, rotation)

    # with g.zmq_context.socket(zmq.PUSH) as socket:
    # socket = context.socket(zmq.PUSH)
    # socket.connect(make_address(app.config.get('SPACERACE_SERVER'),
                                # app.config.get('SPACERACE_CONTROL_PORT')))
    app.logger.debug('Sending control message "{0}"'.format(control_str))
    control_sock.send_string(control_str)

    # Response message and HTTP_202_ACCEPTED code
    # TODO: May need to send empty response to avoid confusion
    return 'Sent control message "{0}"'.format(control_str), 202

if __name__ == '__main__':
    app.run(debug=True)
