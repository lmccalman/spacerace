#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# manned_spacecraft.py
# Game client for 2015 ETD Winter retreat
# https://github.com/lmccalman/spacerace
# 
# Created by Louis Tiao on 28/07/2015.
# 

import argparse
import logging

logger = logging.getLogger(__name__)

logging.basicConfig(
	level = logging.DEBUG,
	datefmt = '%I:%M:%S %p',
	format = '%(asctime)s [%(levelname)s]: %(message)s'
)

if __name__ == '__main__':

	parser = argparse.ArgumentParser(
		description='Spacerace: Manned Spacecraft'
	)

	parser.add_argument('--version', action='version', version='%(prog)s 1.0')
	parser.add_argument('hostname', type=str, help='Server hostname')
	parser.add_argument('state_port', type=int, help='State port')
	parser.add_argument('control_port', type=int, help='Control port')
	parser.add_argument('lobby_port', type=int, help='Lobby port')

	args = parser.parse_args()

	logger.debug(args)