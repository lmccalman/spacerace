#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import logging
import string
import random
import zmq

from client import Client, StateClient
from collections import defaultdict
from argparse import ArgumentParser

DEFAULTS = {
    'hostname': 'localhost',
    'state_port': 5556,
    'control_port': 5557,
    'lobby_port': 5558,
}

# Setup basic logging
logger = logging.getLogger(__name__)

logging.basicConfig(
    level=logging.DEBUG,
    datefmt='%I:%M:%S %p',
    format='%(asctime)s [%(levelname)s]: %(message)s'
)

# Helper functions
make_random_name = lambda length: ''.join(random.choice(string.ascii_letters) for _ in range(length))
make_random_control = lambda: (random.choice([1, 1, 1, 1, 0]),
                               random.choice([-1, -1, -1, 0, 1]))


def make_context():
    context = zmq.Context()
    context.linger = 0
    return context

if __name__ == '__main__':

    parser = ArgumentParser(
        description='Spacerace: Dummy Spacecraft'
    )

    parser.add_argument('--version', action='version', version='%(prog)s 1.0')

    parser.add_argument('--hostname', type=str, help='Server hostname', default=DEFAULTS['hostname'])
    parser.add_argument('--state_port', type=int, help='State port', default=DEFAULTS['state_port'])
    parser.add_argument('--control_port', type=int, help='Control port', default=DEFAULTS['control_port'])
    parser.add_argument('--lobby_port', type=int, help='Lobby port', default=DEFAULTS['lobby_port'])

    parser.add_argument('--num_ships', '-n', type=int, default=1, help='Number of clients to Spawn')

    args = parser.parse_args()
    logger.debug(args)

    # with make_context() as context:

    context = make_context()

    client = Client(args.hostname, args.lobby_port, args.control_port, args.state_port, context)

    game_num = 0

    while True:

        secrets = defaultdict(list)
        for i in range(args.num_ships):
            password = 'password-{}'.format(i)
            instance = client.lobby.register("g{}team{}".format(game_num, i), "g{}player{}".format(game_num,i), password)
            secrets[instance.game].append(password)

        for game in secrets:

            state_client = StateClient(args.hostname, args.state_port, context).subscribe(game)

            for s in state_client.state_gen():
                print(s)
                for secret in secrets[game]:
                    client.control.send(secret, *make_random_control())

            state_client.close()

        game_num += 1

    client.close()
