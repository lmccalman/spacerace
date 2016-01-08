#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# client.py
# Game client for 2015 ETD Winter retreat
# https://github.com/lmccalman/spacerace
#
# Created by Louis Tiao on 28/07/2015.
#

import logging
import string
import random
import json
import zmq

from six.moves import map, range

# Setup basic logging
logging.basicConfig(
    level=logging.INFO,
    datefmt='%I:%M:%S %p',
    format='%(asctime)s [%(levelname)s]: %(message)s'
)

logger = logging.getLogger(__name__)

# Helper functions
make_address = 'tcp://{}:{}'.format
make_handshake_msg = lambda name, team, password: locals()
make_control_str = lambda *args: ','.join(map(str, args))
make_random_name = lambda length: ''.join(random.choice(string.ascii_letters)
                                          for _ in range(length))


def make_context():
    context = zmq.Context()
    context.linger = 0
    return context


class Bunch(dict):

    def __init__(self, **kwargs):
        dict.__init__(self, kwargs)
        self.__dict__.update(kwargs)


class Client:

    def __init__(self, hostname, lobby_port, control_port, state_port, context=None):

        if context is None:
            self.context = make_context()
        else:
            self.context = context

        self.lobby = LobbyClient(hostname, lobby_port, self.context)
        self.control = ControlClient(hostname, control_port, self.context)
        self.state = StateClient(hostname, state_port, self.context)

    def close(self):
        self.state.close()
        self.control.close()
        self.lobby.close()


class BaseClient:

    def __init__(self, hostname, port, context=None):

        if context is None:
            self.context = make_context()
        else:
            self.context = context

        addr = make_address(hostname, port)

        self.sock = self.make_socket()

        logger.debug('Connecting to "{}"...'.format(addr))
        self.sock.connect(addr)

    def close(self):
        self.sock.close()


class LobbyClient(BaseClient):

    def make_socket(self):
        return self.context.socket(zmq.REQ)

    def register(self, name, team, password):

        logger.debug('Registering ship "{}" under team "{}"'.format(name, team))
        self.sock.send_json(make_handshake_msg(name, team, password))

        logger.debug('Awaiting response from lobby thread...')
        response = self.sock.recv_json()

        logger.debug('Got "{}"'.format(response))
        return Bunch(**response)


class ControlClient(BaseClient):

    def make_socket(self):
        return self.context.socket(zmq.PUSH)

    def send(self, password, linear, rotational):
        logger.debug('Sending control "{}"'.format(
                     make_control_str(password, linear, rotational)))
        self.sock.send_string(make_control_str(password, linear, rotational))


class StateClient(BaseClient):

    def make_socket(self):
        return self.context.socket(zmq.SUB)

    def subscribe(self, game_name):
        self.sock.setsockopt_string(zmq.SUBSCRIBE, game_name)
        return self

    def recv(self, *recv_args, **recv_kwargs):
        logger.debug('Receiving state data...')
        _, msg_b = self.sock.recv_multipart(*recv_args, **recv_kwargs)
        return json.loads(msg_b.decode())


class InfoClient(BaseClient):

    def make_socket(self):
        sock = self.context.socket(zmq.SUB)
        sock.setsockopt_string(zmq.SUBSCRIBE, "")
        return sock

    def recv(self, *recv_args, **recv_kwargs):
        logger.debug('Receiving log info...')
        return self.sock.recv_json(*recv_args, **recv_kwargs)
