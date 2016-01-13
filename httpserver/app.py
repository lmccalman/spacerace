from flask import Flask, jsonify, request
from helpers import (make_random_name, make_context, make_address,
                     make_control_str, InvalidUsage)
from flask.ext.cors import CORS
from pprint import pformat

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
states_dict = {}


def state_watcher():
    while True:
        game_name_b, state_b = state_sock.recv_multipart()
        game_name = game_name_b.decode()

        try:
            latest_state = json.loads(state_b.decode())
            with state_lock:
                states_dict[game_name] = latest_state
        except:
            app.logger.warning('Could not parse game state "{}"'
                               .format(state_b.decode()))
            continue

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


@app.route('/lobby', methods=['POST'])
def lobby():

    name = request.json.get('name', make_random_name())
    team = request.json.get('team', make_random_name())

    try:
        password = request.json['password']
    except KeyError:
        raise InvalidUsage('Password required')

    app.logger.debug('Registering "{}" under team "{}"'.format(name, team))
    lobby_sock.send_json(dict(name=name, team=team, password=password))

    app.logger.debug('Awaiting response from lobby...')
    # TODO: Need to set timeout for ZMQ socket here so the HTTP API server
    # doesn't just die if the socket is blocking
    response = lobby_sock.recv_json()

    return jsonify(response)


@app.route('/games')
def games():

    with state_lock:
        statuses = {game: states_dict[game]['state'] for game in states_dict}

    return jsonify(statuses)


@app.route('/state')
@app.route('/state/<string:game>')
def state(game=None):

    try:
        if game is None:
            game = request.args['game']
    except KeyError:
        raise InvalidUsage('Game not specified.')

    with state_lock:
        try:
            current_state = states_dict[game]
        except KeyError:
            raise InvalidUsage("Game '{}' does not exist!".format(game))

    return jsonify(current_state)


@app.route('/control', methods=['POST', 'PUT'])
def control():

    try:
        password = str(request.json['password'])
        linear = str(request.json['linear'])
        rotation = str(request.json['rotation'])
    except KeyError:
        raise InvalidUsage('One or more required arguments not provided.')

    control_str = make_control_str(password, linear, rotation)

    app.logger.debug('Sending control message "{0}"'.format(control_str))
    control_sock.send_string(control_str)

    return jsonify(message='Sent control message "{0}"'.format(control_str))


if __name__ == '__main__':
    # Start server
    app.run()
