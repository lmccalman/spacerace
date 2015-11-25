from flask import Flask, jsonify, request

from string import ascii_letters
from random import choice

import zmq

app = Flask(__name__)
app.config.from_object('settings')

make_random_name = lambda length=10: ''.join(choice(ascii_letters) for _ in range(length))
make_address = 'tcp://{0}:{1}'.format


def make_context():
    context = zmq.Context()
    context.linger = 0
    return context

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
    lobby_sock.send_json(dict(name=name, team=team))
    return jsonify(lobby_sock.recv_json())


if __name__ == '__main__':
    app.run()
