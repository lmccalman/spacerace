#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# manned_spacecraft.py
# HTTP client for 2015 Summer Scholars Hackathon
# https://github.com/lmccalman/spacerace
#
# Created by Louis Tiao on 26/11/2015.
#
import matplotlib
matplotlib.use('TkAgg')

import matplotlib.pyplot as plt

import argparse
import requests
import logging
import pprint
import string
import random

DEFAULTS = {
    'server': 'http://127.0.0.1:5000',  #'192.168.1.110', #'localhost',
}

# Setup basic logging
logger = logging.getLogger(__name__)

logging.basicConfig(
    level=logging.INFO,
    datefmt='%I:%M:%S %p',
    format='%(asctime)s [%(levelname)s]: %(message)s'
)

# Helper functions
make_address = 'tcp://{}:{}'.format
make_random_name = lambda length: ''.join(random.choice(string.ascii_letters) \
    for _ in range(length))


class Client:

    def __init__(self, addr, ship_name, team_name):

        self.pressed = set()
        self.addr = addr

        url = '/'.join((self.addr, 'lobby'))
        params = dict(name=ship_name, team=team_name)

        logger.info('Connecting to {} with params {}'.format(url, params))
        r = requests.get(url, params=params)

        logger.info('Awaiting confirmation from lobby endpoint...')
        lobby_response_data = r.json()

        logging.debug(pprint.pformat(lobby_response_data))

        self.secret = lobby_response_data['secret']
        self.game_name = lobby_response_data['game']
        self.game_map = lobby_response_data['map']

        logging.info('Ship [{}] was registered to participate in game [{}]'
                     ' (secret [{}])'.format(ship_name, self.game_name, self.secret))

    def recv_state(self):
        logger.info('Awaiting message from state endpoint...')
        r = requests.get('/'.join((self.addr, 'state')))
        return r.json()

    def send_control(self, linear, rotation):
        url = '/'.join((self.addr, 'control', self.secret))
        params = dict(linear=linear, rotation=rotation)
        logger.info('Connecting to {} with params {}'.format(url, params))
        r = requests.post(url, json=params)
        return r.status_code

    def update_control(self):
        linear = int('up' in self.pressed)
        rotation = 0
        rotation += int('left' in self.pressed)
        rotation -= int('right' in self.pressed)
        self.send_control(linear, rotation)

    def press(self, event):
        self.pressed.add(event.key)
        self.update_control()

    def release(self, event):
        self.pressed.discard(event.key)
        self.update_control()


if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='Spacerace: Manned Spacecraft'
    )

    parser.add_argument('--server', type=str, help='Server address',
                        default=DEFAULTS['server'])

    parser.add_argument('--version', action='version', version='%(prog)s 1.0')
    parser.add_argument('--ship_name', '-n', type=str,
                        default=make_random_name(10), help='Ship Name')
    parser.add_argument('--team_name', '-t', type=str,
                        default=make_random_name(10), help='Ship Name')

    args = parser.parse_args()

    logger.debug(args)

    client = Client(args.server, args.ship_name, args.team_name)

    fig, ax = plt.subplots()

    fig.canvas.mpl_connect('key_press_event', client.press)
    fig.canvas.mpl_connect('key_release_event', client.release)

    plt.show()
