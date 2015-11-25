from flask import Flask, jsonify, request

from string import ascii_letters
from random import choice

app = Flask(__name__)

make_random_name = lambda length=10: ''.join(choice(ascii_letters) for _ in range(length))


@app.route('/')
def hello_world():
    return jsonify(msg='Hello World!')


@app.route('/lobby')
def lobby():
    name = request.args.get('name', default=make_random_name())
    team = request.args.get('team', default=make_random_name())
    return jsonify(msg='Hello welcome to the lobby! Name {0} | Team {1}'.format(name, team))


if __name__ == '__main__':
    app.run()
