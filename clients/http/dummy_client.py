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
import requests

from six.moves.urllib.parse import urljoin
from argparse import ArgumentParser
from functools import partial
from pprint import pformat
from time import sleep

DEFAULTS = {
    'address': 'http://localhost:5001',
}

# Setup basic logging
logging.basicConfig(
    level=logging.DEBUG,
    datefmt='%I:%M:%S %p',
    format='%(asctime)s [%(levelname)s]: %(message)s'
)

logger = logging.getLogger(__name__)

# Helper functions
make_random_name = lambda length: ''.join(random.choice(string.ascii_letters)
                                          for _ in range(length))
make_random_control = lambda: (random.choice([1, 1, 1, 1, 0]),
                               random.choice([-1, -1, -1, 0, 1]))


if __name__ == '__main__':

    parser = ArgumentParser(
        description='Spacerace: Dummy Spacecraft'
    )

    parser.add_argument('--version', action='version', version='%(prog)s 1.0')

    parser.add_argument('--address', '-a', type=str, help='Server address',
                        default=DEFAULTS['address'])

    parser.add_argument('--name', '-n', type=str,
                        default=make_random_name(10), help='Ship name')
    parser.add_argument('--team', '-t', type=str,
                        default=make_random_name(10), help='Team name')
    parser.add_argument('--password', '-p', type=str,
                        default=make_random_name(12), help='Password')

    args = parser.parse_args()
    logger.debug(args)

    make_addr = partial(urljoin, args.address)

    lobby_addr = make_addr('lobby')
    state_addr = make_addr('state')
    control_addr = make_addr('control')

    r_lobby = requests.post(lobby_addr, json=dict(name=args.name,
                                                  team=args.team,
                                                  password=args.password))
    r_lobby.raise_for_status()

    lobby_response = r_lobby.json()
    logger.debug('Registered to participate "{}"'
                 .format(pformat(lobby_response)))

    while True:
        try:
            r_state = requests.get(state_addr,
                                   params=dict(game=lobby_response['game']))
            r_state.raise_for_status()
        except requests.exceptions.HTTPError:
            if r_state.status_code == 400:
                if r_state.json().get('message').endswith('does not exist!'):
                    logger.warning('Game does not exist yet! Trying again...')
                    sleep(1)  # wait a sec so we're not slamming the server
                    continue
            else:
                # Some other unexpected HTTPError so we re-raise
                raise
        else:
            # No HTTPError so we know the game at least exists and is either
            # queued, running or finished
            break

    status = r_state.json().get('state')

    while status != 'finished':
        r_state = requests.get(state_addr,
                               params=dict(game=lobby_response['game']))
        r_state.raise_for_status()
        game_state = r_state.json()
        logger.debug('Current state "{}"'.format(pformat(game_state)))
        status = game_state.get('state')
        if status == 'queued':
            continue
        linear, rotation = make_random_control()
        control_data = dict(password=args.password,
                            linear=linear,
                            rotation=rotation)
        logger.debug('Sending control data "{}"'.format(control_data))
        r_control = requests.post(control_addr, json=control_data)
        r_state.raise_for_status()
