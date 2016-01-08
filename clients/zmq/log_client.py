#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# log_client.py

import zmq
import logging

from client import InfoClient
from argparse import ArgumentParser

DEFAULTS = {
    'hostname': 'localhost',
    'port': 5559
}

# Setup basic logging
logging.basicConfig(
    level=logging.DEBUG,
    datefmt='%I:%M:%S %p',
    format='%(asctime)s [%(levelname)s]: %(message)s'
)

logger = logging.getLogger(__name__)


if __name__ == '__main__':

    parser = ArgumentParser(
        description='Spacerace: Log Client'
    )

    parser.add_argument('--version', action='version', version='%(prog)s 1.0')

    parser.add_argument('--hostname', type=str, help='Server hostname',
                        default=DEFAULTS['hostname'])
    parser.add_argument('--port', type=int, help='Port',
                        default=DEFAULTS['port'])

    args = parser.parse_args()
    logger.debug(args)

    with zmq.Context() as context:
        client = InfoClient(args.hostname, args.port, context)
        while True:
            print(client.recv())
        client.close()
