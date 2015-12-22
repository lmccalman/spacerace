from flask import Flask, jsonify, request
from flask.ext.cors import CORS
from helpers import (make_random_name, make_context, make_address,
                     make_control_str, InvalidUsage)

import threading
import json
import zmq

app = Flask(__name__)
CORS(app)

app.config.from_object('settings')

# TODO: context and sockets are never closed. Need to determine the best place
# TODO: these should actually be initialized on a per-request basis. question
# is whether the context can be shared.
context = make_context()

lobby_sock = context.socket(zmq.REQ)
addr = make_address(app.config.get('SPACERACE_SERVER'),
                    app.config.get('SPACERACE_LOBBY_PORT'))
app.logger.info('Connecting to lobby "{}"...'.format(addr))
lobby_sock.connect(addr)

control_sock = context.socket(zmq.PUSH)
addr = make_address(app.config.get('SPACERACE_SERVER'),
                    app.config.get('SPACERACE_CONTROL_PORT'))
app.logger.info('Connecting to control "{}"...'.format(addr))
control_sock.connect(addr)

state_sock = context.socket(zmq.SUB)
state_sock.setsockopt_string(zmq.SUBSCRIBE, '')
addr = make_address(app.config.get('SPACERACE_SERVER'),
                    app.config.get('SPACERACE_STATE_PORT'))
app.logger.info('Connecting to state "{}"...'.format(addr))
state_sock.connect(addr)

# This state gets written to in the main loop
state_lock = threading.Lock()
game_state = [{'state': 'finished'}]


def state_watcher():
    while True:
        _, state_b = state_sock.recv_multipart()
        new_game_state = game_state[0]

        try:
            new_game_state = json.loads(state_b.decode())
        except:
            app.logger.warning('Could not parse game state "{}"'
                               .format(state_b.decode()))
            continue

        with state_lock:
            game_state[0] = new_game_state

# Start state monitoring thread
t = threading.Thread(target=state_watcher)
t.daemon = True  # die with main thread
t.start()


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
    with state_lock:
        current_state = game_state[0]

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

    return jsonify(message='Sent control message "{0}"'.format(control_str))


if __name__ == '__main__':
    # Start server
    app.run(debug=True)
