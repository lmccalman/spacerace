#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# client.py
# Game client for 2015 ETD Winter retreat
# https://github.com/lmccalman/spacerace
#
# Created by Louis Tiao on 28/07/2015.
#

import string
import random
import zmq


# Helper functions
make_control_str = lambda *args: ','.join(args)
make_random_name = lambda length=10: ''.join(random.choice(string.ascii_letters) for _ in range(length))
make_address = 'tcp://{0}:{1}'.format


def make_context():
    context = zmq.Context()
    context.linger = 0
    return context
