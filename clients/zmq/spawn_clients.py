#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# spawn_client.py
# Game client for 2015 ETD Winter retreat
# https://github.com/lmccalman/spacerace
#
# Created by Louis Tiao on 28/07/2015.
#

import logging
import random

from client import Client, StateClient, make_random_name, make_context
from argparse import ArgumentParser
from collections import defaultdict
from itertools import count
from six.moves import range

DEFAULTS = {
    'hostname': 'localhost',
    'state_port': 5556,
    'control_port': 5557,
    'lobby_port': 5558,
}

# Setup basic logging
logging.basicConfig(
    level=logging.DEBUG,
    datefmt='%I:%M:%S %p',
    format='%(asctime)s [%(levelname)s]: %(message)s'
)

logger = logging.getLogger(__name__)

# Helper functions
make_random_control = lambda: (random.choice([1, 1, 1, 1, 0]),
                               random.choice([-1, 1, 0, 0, 0, 0, 0]))

if __name__ == '__main__':

    parser = ArgumentParser(
        description='Spacerace: Spawn Dummy Spacecraft'
    )

    parser.add_argument('--version', action='version', version='%(prog)s 1.0')

    parser.add_argument('--hostname', type=str, help='Server hostname',
                        default=DEFAULTS['hostname'])
    parser.add_argument('--state-port', type=int, help='State port',
                        default=DEFAULTS['state_port'])
    parser.add_argument('--control-port', type=int, help='Control port',
                        default=DEFAULTS['control_port'])
    parser.add_argument('--lobby-port', type=int, help='Lobby port',
                        default=DEFAULTS['lobby_port'])

    parser.add_argument('--name', '-n', type=str,
                        default=make_random_name(10), help='Ship name')
    parser.add_argument('--team', '-t', type=str,
                        default=make_random_name(10), help='Team name')
    parser.add_argument('--password', '-p', type=str,
                        default=make_random_name(12), help='Password')

    parser.add_argument('--num-clients', type=int, default=1,
                        help='Number of clients to Spawn')

    args = parser.parse_args()
    logger.debug(args)

    context = make_context()
    client = Client(args.hostname, args.lobby_port, args.control_port,
                    args.state_port, context)

    for game_num in count():

        passwords = defaultdict(list)
        for i in range(args.num_clients):
            password = 'password-{}'.format(i)
            instance = client.lobby.register("g{}team{}".format(game_num, i),
                                             "g{}player{}".format(game_num, i),
                                             password)
            passwords[instance.game].append(password)

        for game in passwords:

            state_client = StateClient(args.hostname, args.state_port, context).subscribe(game)

            while True:
                state_data = state_client.recv()
                if state_data['state'] == 'finished':
                    break
                for password in passwords[game]:
                    client.control.send(password, *make_random_control())

            state_client.close()

    client.close()
