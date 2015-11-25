# state_watcher.py
#
# threaded state monitoring - pushed from spacerace over zmq

import json
import argparse
import time
import threading
import zmq
# from zmq.eventloop import ioloop
# from zmq.eventloop.ioloop import ZMQIOLoop
# from zmq.eventloop.zmqstream import ZMQStream
# install PyZMQ's IOLoop
# ioloop.install()
import logging
logger = logging.getLogger(__name__)
lock = threading.Lock()

DEFAULTS = {
    'hostname': 'localhost',
    'state_port': 5556,
    'control_port': 5557,
    'lobby_port': 5558,
}

# Helper functions
make_address = 'tcp://{}:{}'.format


def main():
    parser = argparse.ArgumentParser(
        description='Spacerace: Manned Spacecraft')
    parser.add_argument('--hostname', type=str, help='Server hostname',
                        default=DEFAULTS['hostname'])
    parser.add_argument('--state_port', type=int, help='State port',
                        default=DEFAULTS['state_port'])
    parser.add_argument('--lobby_port', type=int, help='Lobby port',
                        default=DEFAULTS['lobby_port'])
    args = parser.parse_args()
    logger.debug(args)
    game_state = [None]
    t = threading.Thread(target=state_client,
                         args=(args.hostname, args.state_port, game_state))
    t.daemon = True  # die with main thread
    t.start()

    # Now do our own "Game Loop"
    print("Original thread game loop:")
    while True:
        with lock:
            state = game_state[0]
        if state:
            print(state)
        else:
            print("Not ready")
        time.sleep(1)


def state_client(hostname, state_port, game_state):
    context = zmq.Context()
    context.linger = 0
    state_address = make_address(hostname, state_port)
    # Using State socket
    print('Connecting to state socket at [{0}]...'.format(state_address))
    game_name = ""  # I'm hoping this applies to all games
    state_sock = context.socket(zmq.SUB)
    print("Subscribe")
    state_sock.setsockopt_string(zmq.SUBSCRIBE, game_name)
    print("Connect")
    state_sock.connect(state_address)
    print("Recv Loop init.")
    # note: messages will always be multipart, even if length 1

    while True:
        msg_filter_b, state_b = state_sock.recv_multipart()
        with lock:
            game_state[0] = json.loads(state_b.decode())

    return


if __name__ == '__main__':
    main()
