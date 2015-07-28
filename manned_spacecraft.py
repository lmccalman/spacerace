#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# manned_spacecraft.py
# Game client for 2015 ETD Winter retreat
# https://github.com/lmccalman/spacerace
# 
# Created by Louis Tiao on 28/07/2015.
# 

import itertools
import argparse
import logging
import pprint
import string
import random
import json
import zmq

from collections import defaultdict

# Setup basic logging
logger = logging.getLogger(__name__)

logging.basicConfig(
    level = logging.INFO,
    datefmt = '%I:%M:%S %p',
    format = '%(asctime)s [%(levelname)s]: %(message)s'
)

# Helper functions
make_address = 'tcp://{}:{}'.format
make_random_name = lambda length: ''.join(random.choice(string.ascii_letters) \
    for _ in range(length))

class Client:

    def __init__(self, server, state_port, control_port, lobby_port):
        
        self.context = zmq.Context()
        self.context.linger = 0

        self.control_address = make_address(server, control_port)
        self.state_address = make_address(server, state_port)
        self.lobby_address = make_address(server, lobby_port)

        self.secret_keys = {}

    def __enter__(self):

        self.lobby_sock = self.context.socket(zmq.REQ)
        logger.info('Connecting to lobby lobby at [{0}]...'.format(self.lobby_address)) 
        self.lobby_sock.connect(self.lobby_address)

        self.control_sock = self.context.socket(zmq.PUSH)
        logger.info('Connecting to control socket at [{0}]...'.format(self.control_address)) 
        self.control_sock.connect(self.control_address)

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.lobby_sock.close()
        self.control_sock.close()
        self.context.close()

    def register(self, ship_name):

        logger.info('Sending ship name [{0}]'.format(ship_name))
        self.lobby_sock.send_string(ship_name)

        logger.info('Awaiting confirmation from lobby...')
        lobby_response_data = self.lobby_sock.recv_json()

        logging.debug(pprint.pformat(lobby_response_data))

        secret_key = lobby_response_data['secret']
        ship_name  = lobby_response_data['name']
        self.secret_keys[ship_name] = secret_key
        
        self.game_name = lobby_response_data['game']
        self.game_map  = lobby_response_data['map']

        logging.info('Ship [{}] was registered to participate in game [{}]'
                     ' (secret [{}])'.format(ship_name, self.game_name, secret_key))

    def recv_state(self):
        state_sock = self.context.socket(zmq.SUB)
        state_sock.setsockopt_string(zmq.SUBSCRIBE, self.game_name)
        state_sock.connect(self.state_address)
        msg_filter_b, state_b = state_sock.recv_multipart()
        state = json.loads(state_b.decode())
        state_sock.close()
        return state

    def send_control(self, ship_name, linear, rotational):
        secret_key = self.secret_keys[ship_name]
        control_lst = [secret_key, linear, rotational]
        control_str = ','.join(map(str, control_lst))
        self.control_sock.send_string(control_str)

if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='Spacerace: Manned Spacecraft'
    )

    parser.add_argument('hostname', type=str, help='Server hostname')
    parser.add_argument('state_port', type=int, help='State port')
    parser.add_argument('control_port', type=int, help='Control port')
    parser.add_argument('lobby_port', type=int, help='Lobby port')

    parser.add_argument('--version', action='version', version='%(prog)s 1.0')
    parser.add_argument('--ship_name', '-n', type=str,
        default=make_random_name(10), help='Ship Name')

    args = parser.parse_args()

    logger.debug(args)

    with Client(args.hostname, args.state_port, args.control_port, args.lobby_port) as client:
        client.register(args.ship_name)
        while True:
            pprint.pprint(client.recv_state())
            client.send_control(args.ship_name, 1, 1)