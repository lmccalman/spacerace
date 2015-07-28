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


logger = logging.getLogger(__name__)

logging.basicConfig(
    level = logging.DEBUG,
    datefmt = '%I:%M:%S %p',
    format = '%(asctime)s [%(levelname)s]: %(message)s'
)

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

        self.control_sock = self.context.socket(zmq.PUSH)
        self.control_sock.connect(self.control_address)

        self.state_socket = self.context.socket(zmq.SUB)
        self.state_socket.setsockopt_string(zmq.SUBSCRIBE, self.game_name)

    def enter_game(self, ship_name):
        self.lobby_sock = self.context.socket(zmq.REQ)

        logger.info('Connecting to lobby at [{0}]...' \
            .format(self.lobby_address))
        self.lobby_sock.connect(self.lobby_address)

        logger.info('Sending ship name [{0}]'.format(ship_name))
        self.lobby_sock.send_string(ship_name)

        logger.info('Awaiting confirmation from lobby...')
        lobby_response_data = self.lobby_sock.recv_json()

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

    exit(0)

    context = zmq.Context()
    context.linger = 0

    lobby_sock = context.socket(zmq.REQ)
    lobby_address = make_address(args.hostname, args.lobby_port)

    logger.info('Connecting to lobby at [{0}]...'.format(lobby_address))
    lobby_sock.connect(lobby_address)

    logger.info('Sending ship name [{0}]'.format(args.name))
    lobby_sock.send_string(args.name)

    lobby_reply_data = lobby_sock.recv_json()

    # lobby_reply_data = json.loads(lobby_reply.decode())

    logger.info(lobby_reply_data)

    control_socket = context.socket(zmq.PUSH)
    
    state_socket = context.socket(zmq.SUB)
    state_socket.setsockopt_string(zmq.SUBSCRIBE, lobby_reply_data['game'])

    control_address = make_address(args.hostname, args.control_port)
    state_address = make_address(args.hostname, args.state_port)

    control_socket.connect(control_address)
    state_socket.connect(state_address)

    while True:
        print("receiving state info...")
        game_name, state_data = state_socket.recv_multipart()
        print(game_name.decode())
        pprint.pprint(json.loads(state_data.decode()))
        j = json.loads(state_info[1].decode())
        if j['status'] == "GAME OVER":
            break
        print("state_info: {}".format(state_info))
        print("sending control...")
        linear_control = str(random.choice([1,1,1,1,0]))
        rotational_control = str(random.choice([-1,-1,1,1,0,0,0,0,0,0,0]))
        control_socket.send("{},{},{}".format(my_secret_code,
            linear_control, rotational_control).encode())
        print("control sent")
