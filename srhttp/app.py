from flask import Flask, jsonify, request
from helpers import make_random_name, make_context, make_address

import zmq

app = Flask(__name__)
app.config.from_object('settings')


# TODO: context and sockets are never closed
context = make_context()
lobby_sock = context.socket(zmq.REQ)
lobby_sock.connect(make_address(app.config.get('SPACERACE_SERVER'), app.config.get('SPACERACE_LOBBY_PORT')))


@app.route('/')
def hello_world():
    return jsonify(msg='Hello World!')


@app.route('/lobby')
def lobby():
    name = request.args.get('name', default=make_random_name())
    team = request.args.get('team', default=make_random_name())

    app.logger.debug('Registering ship "{0}" under team "{1}"'.format(name, team))
    lobby_sock.send_json(dict(name=name, team=team))

    app.logger.debug('Awaiting response from lobby...')
    response = lobby_sock.recv_json()

    return jsonify(response)


if __name__ == '__main__':
    app.run(debug=True)
    lobby_sock.close()
