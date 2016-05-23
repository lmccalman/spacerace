#!/usr/bin/env python
# -*- coding: utf-8 -*-

import logging
import string
import random
import requests

from six.moves.urllib.parse import urljoin
from collections import defaultdict
from argparse import ArgumentParser
from functools import partial

DEFAULTS = {
    'server': 'http://127.0.0.1:5001',
}

# Setup basic logging
logger = logging.getLogger(__name__)

logging.basicConfig(
    level=logging.DEBUG,
    datefmt='%I:%M:%S %p',
    format='%(asctime)s [%(levelname)s]: %(message)s'
)

# Helper functions
make_random_name = lambda length: ''.join(random.choice(string.ascii_letters)
                                          for _ in range(length))
make_random_control = lambda: (random.choice([1, 1, 1, 1, 0]),
                               random.choice([-1, -1, -1, 0, 1]))


if __name__ == '__main__':

    parser = ArgumentParser(
        description='Spacerace: Spawn Dummy HTTP Spacecrafts'
    )

    parser.add_argument('--version', action='version', version='%(prog)s 1.0')
    parser.add_argument('--server', type=str, help='Server address',
                        default=DEFAULTS['server'])
    parser.add_argument('--num_ships', '-n', type=int, default=1,
                        help='Number of clients to Spawn')

    args = parser.parse_args()
    logger.debug(args)

    make_addr = partial(urljoin, args.server)

    lobby_addr = make_addr('lobby')
    state_addr = make_addr('state')
    control_addr = make_addr('control')

    secrets = defaultdict(list)
    for i in range(args.num_ships):
        name = "team{}".format(i)
        team = "player{}".format(i)
        r_lobby = requests.get(lobby_addr,
                               params=dict(name=name, team=team))
        instance = r_lobby.json()
        secrets[instance['game']].append(instance['secret'])

    for game in secrets:

        while True:
            r_state = requests.get(state_addr)
            game_state = r_state.json()
            logger.debug('Current state "{}"'.format(game_state))

            # if game_state['state'] == 'finished':
            #     break

            for secret in secrets[game]:

                linear, rotation = make_random_control()
                control_data = dict(secret=secret, linear=linear, rotation=rotation)
                logger.debug('Sending control data "{}"'.format(control_data))
                requests.post(control_addr, json=control_data)
