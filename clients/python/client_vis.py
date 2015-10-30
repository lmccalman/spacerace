#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# client_vis.py
# Game client for 2015 ETD Winter retreat
# Client-side Visualization
# https://github.com/lmccalman/spacerace
#
# Created by Louis Tiao on 28/07/2015.
#

# For Mac OS X
import matplotlib
matplotlib.use('TkAgg')

import matplotlib.pyplot as plt
import matplotlib.animation as animation

import logging
import pprint
import string
import random
import zmq

from matplotlib.patches import Wedge
from argparse import ArgumentParser
from client import Client

DEFAULTS = {
    'hostname': 'localhost',
    'state_port': 5556,
    'control_port': 5557,
    'lobby_port': 5558,
}

# Setup basic logging
logger = logging.getLogger(__name__)

logging.basicConfig(
    level = logging.INFO,
    datefmt = '%I:%M:%S %p',
    format = '%(asctime)s [%(levelname)s]: %(message)s'
)

# Helper functions
make_random_name = lambda length: ''.join(random.choice(string.ascii_letters) \
    for _ in range(length))


class MPLController:

    def __init__(self, ship_name, team_name, fig, ax, client=None, *args, **kwargs):

        if client is None:
            client = Client(*args, **kwargs)

        self.client = client
        self.keys = set()

        fig.canvas.mpl_connect('key_press_event', self.press)
        fig.canvas.mpl_connect('key_release_event', self.release)

        self.text = ax.text(0.25, 0.25, 'Hello!')

        anim = animation.FuncAnimation(fig, self.anim, frames=self.state_gen(),
                                       init_func=self.init_anim, interval=25,
                                       blit=True, repeat=False)

        response = self.client.lobby.register(ship_name, team_name)

        self.game = response.game
        self.ship = response.name
        self.secret = response.secret

        self.client.state.subscribe(self.game)

    def update_control(self):
        linear = int('up' in self.pressed)
        rotation = 0
        rotation += int('left' in self.pressed)
        rotation -= int('right' in self.pressed)
        self.client.control.send(self.secret, linear, rotation)

    def press(self, event):
        self.pressed.add(event.key)
        self.update_control()

    def release(self, event):
        self.pressed.discard(event.key)
        self.update_control()

    def init_anim(self):
        self.text.set_text('')
        return self.text,

    def anim(self, state):
        self.text.set_text(pprint.pformat(state))
        return self.text,

    def state_gen(self):
        return self.client.state.state_gen()

if __name__ == '__main__':

    parser = ArgumentParser(
        description='Spacerace: Manned Spacecraft'
    )

    parser.add_argument('--hostname', type=str, help='Server hostname', default=DEFAULTS['hostname'])
    parser.add_argument('--state_port', type=int, help='State port', default=DEFAULTS['state_port'])
    parser.add_argument('--control_port', type=int, help='Control port', default=DEFAULTS['control_port'])
    parser.add_argument('--lobby_port', type=int, help='Lobby port', default=DEFAULTS['lobby_port'])

    parser.add_argument('--version', action='version', version='%(prog)s 1.0')

    parser.add_argument('--ship_name', '-n', type=str,
                        default=make_random_name(10), help='Ship Name')
    parser.add_argument('--team_name', '-t', type=str,
                        default=make_random_name(10), help='Ship Name')

    args = parser.parse_args()

    logger.debug(args)

    fig, ax = plt.subplots()

    client = Client(args.hostname, args.lobby_port, args.control_port, args.state_port)
    
    c = MPLController(args.ship_name, args.team_name, fig, ax, client)

    plt.show()
