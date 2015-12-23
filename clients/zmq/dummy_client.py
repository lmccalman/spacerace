#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# dummy_client.py
# Game client for 2015 ETD Winter retreat
# https://github.com/lmccalman/spacerace
#
# Created by Louis Tiao on 28/07/2015.
#

import logging
import string
import random
import zmq

from client import Client
from argparse import ArgumentParser

DEFAULTS = {
    'hostname': 'localhost',
    'state_port': 5556,
    'control_port': 5557,
    'lobby_port': 5558,
}

logging.basicConfig(
    level=logging.DEBUG,
    datefmt='%I:%M:%S %p',
    format='%(asctime)s [%(levelname)s]: %(message)s'
)

# Setup basic logging
logger = logging.getLogger(__name__)

# Helper functions
make_random_name = lambda length: ''.join(random.choice(string.ascii_letters)
                                          for _ in range(length))
make_random_control = lambda: (random.choice([1, 1, 1, 1, 0]),
                               random.choice([-1, -1, 1, 1, 0, 0, 0, 0, 0, 0, 0]))


def make_context():
    context = zmq.Context()
    context.linger = 0
    return context

if __name__ == '__main__':

    parser = ArgumentParser(
        description='Spacerace: Dummy Spacecraft'
    )

    parser.add_argument('--version', action='version', version='%(prog)s 1.0')

    parser.add_argument('--hostname', type=str, help='Server hostname',
                        default=DEFAULTS['hostname'])
    parser.add_argument('--state_port', type=int, help='State port',
                        default=DEFAULTS['state_port'])
    parser.add_argument('--control_port', type=int, help='Control port',
                        default=DEFAULTS['control_port'])
    parser.add_argument('--lobby_port', type=int, help='Lobby port',
                        default=DEFAULTS['lobby_port'])

    parser.add_argument('--name', '-n', type=str,
                        default=make_random_name(10), help='Ship name')
    parser.add_argument('--team', '-t', type=str,
                        default=make_random_name(10), help='Team name')
    parser.add_argument('--password', '-p', type=str,
                        default=make_random_name(12), help='Password')

    args = parser.parse_args()
    logger.debug(args)

    context = make_context()
    client = Client(args.hostname, args.lobby_port, args.control_port,
                    args.state_port, context)

    while True:
        response = client.lobby.register(args.name, args.team, args.password)
        client.state.subscribe(response.game)

        for state_data in client.state.state_gen():
            logger.debug(state_data)
            client.control.send(args.password, *make_random_control())

    client.close()
    context.destroy()
