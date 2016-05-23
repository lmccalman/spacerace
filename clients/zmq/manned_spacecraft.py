#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
# manned_spacecraft.py
# Game client for 2015 ETD Winter retreat
# https://github.com/lmccalman/spacerace
#
# Created by Louis Tiao on 28/07/2015.
#

# Matplotlib backend shim for Mac OSX
import matplotlib
import sys

if sys.platform.startswith('darwin'):
    matplotlib.use('TkAgg')

import matplotlib.pyplot as plt

import argparse
import logging

from client import make_random_name, Client

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


class MPLClient(Client):

    def __init__(self, hostname, lobby_port, control_port, state_port, context=None):
        self.pressed = set()
        super(MPLClient, self).__init__(hostname, lobby_port, control_port,
                                        state_port, context)

    def register(self, name, team, password):
        self.password = password
        self.lobby.register(name, team, password)

    def update_control(self):
        linear = int('up' in self.pressed)
        rotation = 0
        rotation += int('left' in self.pressed)
        rotation -= int('right' in self.pressed)
        self.control.send(self.password, linear, rotation)

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

    args = parser.parse_args()
    logger.debug(args)

    client = MPLClient(args.hostname, args.lobby_port, args.control_port,
                       args.state_port)
    client.register(args.name, args.team, args.password)

    fig, ax = plt.subplots()

    fig.canvas.mpl_connect('key_press_event', client.press)
    fig.canvas.mpl_connect('key_release_event', client.release)

    plt.show()
