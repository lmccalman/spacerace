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

# Setup basic logging
logger = logging.getLogger(__name__)

logging.basicConfig(
    level = logging.DEBUG,
    datefmt = '%I:%M:%S %p',
    format = '%(asctime)s [%(levelname)s]: %(message)s'
)

# Helper functions
make_address = 'tcp://{}:{}'.format
make_random_name = lambda length: ''.join(random.choice(string.ascii_letters) \
    for _ in range(length))

class GameClient:

    def __init__(self, server, state_port, control_port, lobby_port):
        self.context = zmq.Context()
        self.context.linger = 0

        self.control_address = make_address(server, control_port)
        self.state_address = make_address(server, state_port)
        self.lobby_address = make_address(server, lobby_port)

        # Connect to lobby
        self.lobby_sock = self.context.socket(zmq.REQ)

        logger.info('Connecting to lobby at [{0}]...' \
            .format(self.lobby_address))
        self.lobby_sock.connect(self.lobby_address)

        # Connect to control
        self.control_sock = self.context.socket(zmq.PUSH)
        self.control_sock.connect(self.control_address)

        # Connect to state
        self.state_socket = self.context.socket(zmq.SUB)
        # self.state_socket.setsockopt_string(zmq.SUBSCRIBE, self.game_name)
        self.state_socket.connect(self.state_address)

    def enter_game(self, ship_name):

        logger.info('Sending ship name [{0}]'.format(ship_name))
        self.lobby_sock.send_string(ship_name)

        logger.info('Awaiting confirmation from lobby...')
        lobby_response_data = self.lobby_sock.recv_json()

        # TODO: close sockets

        logging.debug(pprint.pformat(lobby_response_data))

        self.secret_key = lobby_response_data['secret']
        self.ship_name  = lobby_response_data['name']
        self.game_name  = lobby_response_data['game']
        # TODO: map

        logging.info('Ship [{}] was registered to participate in game [{}]'
                     ' (secret [{}])'.format(self.ship_name, self.game_name, \
                            self.secret_key))

        # TODO: should return game state object
        return self.game_name

    def recv_state(self):
        msg_filter_b, state_b = self.state_sock.recv_multipart()
        return json.loads(state_b.decode())

    def send_control(self, linear, rotational):
        control_lst = [self.secret_key, linear, rotational]
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

    client = GameClient(args.hostname, args.state_port, args.control_port, args.lobby_port)
    client.enter_game(args.ship_name)
    while True:
        client.send_control(1, 1)
