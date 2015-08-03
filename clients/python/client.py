#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# client.py
# Game client for 2015 ETD Winter retreat
# https://github.com/lmccalman/spacerace
# 
# Created by Louis Tiao on 28/07/2015.
# 

import argparse
import logging
import pprint
import string
import random
import json
import zmq

from zmq.eventloop.ioloop import ZMQIOLoop
from zmq.eventloop.zmqstream import ZMQStream
from threading import Thread

DEFAULTS = {
    'hostname': 'localhost',
    'state_port': 5556,
    'control_port': 5557,
    'lobby_port': 5558,
}

# Setup basic logging
logger = logging.getLogger(__name__)
logging.basicConfig(
    level = logging.DEBUG,
    datefmt = '%H:%M:%S',
    format = '%(asctime)s,%(msecs)03d (%(threadName)s) [%(levelname)s]: %(message)s'
)

# Helper functions
make_address = 'tcp://{}:{}'.format

make_handshake_msg = lambda ship, team: dict(name=ship, team=team)

make_control_str = lambda secret, linear, rotational: ','.join(\
    [secret, repr(linear), repr(rotational)])

make_random_name = lambda length: ''.join(random.choice(string.ascii_letters) \
    for _ in range(length))

def make_context():
    context = zmq.Context()
    context.linger = 0
    return context

class BaseClient:

    def __init__(self, hostname, port, context=None):

        if context is None:
            self.context = make_context()
        else:
            self.context = context

        self.addr = make_address(hostname, port)

class LobbyClient(BaseClient):

    def register(self, ship_name, team_name):
        
        with self.context.socket(zmq.REQ) as sock:
            sock.connect(self.addr)
            sock.send_json(make_handshake_msg(ship_name, team_name))
            return sock.recv_json()

class ControlClient(BaseClient):

    def send(self, secret_key, linear, rotational):

        with self.context.socket(zmq.PUSH) as sock:
            sock.connect(self.addr)
            sock.send_string(make_control_str(secret_key, linear, rotational))

class StateClient(BaseClient):

    def subscribe(self, game_name):

        logger.debug(self.addr)
        self.thread = Thread(name='my_service', target=self.setup, args=(game_name,))
        self.thread.start()

    def setup(self, game_name):

        self.loop = ZMQIOLoop()
        self.loop.install()

        logger.info('HI!!!')

        with self.context.socket(zmq.SUB) as sock:
            sock.setsockopt_string(zmq.SUBSCRIBE, game_name)
            sock.connect(self.addr)

            stream = ZMQStream(sock, self.loop)
            stream.on_recv(self._process_state_msg)            

            self.loop.start()

    def _process_state_msg(self, multipart):

        msg_filter_b, msg_b = multipart
        state_data = json.loads(msg_b.decode())
        if state_data['state'] == 'finished':
            self.loop.stop()
        logger.info(pprint.pformat(state_data))

if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='Spacerace: Dummy Spacecraft'
    )

    parser.add_argument('--hostname', type=str, help='Server hostname', default=DEFAULTS['hostname'])
    parser.add_argument('--state_port', type=int, help='State port', default=DEFAULTS['state_port'])
    parser.add_argument('--control_port', type=int, help='Control port', default=DEFAULTS['control_port'])
    parser.add_argument('--lobby_port', type=int, help='Lobby port', default=DEFAULTS['lobby_port'])

    parser.add_argument('--version', action='version', version='%(prog)s 1.0')
    parser.add_argument('--ship_name', '-n', type=str,
        default=make_random_name(10), help='Ship Name')
    parser.add_argument('--team_name', '-t', type=str,
        default=make_random_name(10), help='Team Name')

    args = parser.parse_args()

    logger.debug(args)

    with make_context() as context:
        l = LobbyClient(args.hostname, args.lobby_port, context)
        response = l.register(args.ship_name, args.team_name)

        s = StateClient(args.hostname, args.state_port, context)
        s.subscribe(response['game'])
        s.thread.join()