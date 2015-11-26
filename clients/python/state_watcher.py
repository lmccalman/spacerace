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
log = logging.getLogger(__name__)
state_lock = threading.Lock()

DEFAULTS = {
    'hostname': 'localhost',
    'state_port': 5556,
    'control_port': 5557,
    'lobby_port': 5558,
}

# Helper functions
make_address = 'tcp://{}:{}'.format


def main():
    log.setLevel(logging.INFO)
    parser = argparse.ArgumentParser(
        description='Spacerace: Manned Spacecraft')
    parser.add_argument('--hostname', type=str, help='Server hostname',
                        default=DEFAULTS['hostname'])
    parser.add_argument('--state_port', type=int, help='State port',
                        default=DEFAULTS['state_port'])
    parser.add_argument('--lobby_port', type=int, help='Lobby port',
                        default=DEFAULTS['lobby_port'])
    args = parser.parse_args()
    log.debug(args)
    game_state = [{'state': 'finished'}]
    t = threading.Thread(target=state_client,
                         args=(args.hostname, args.state_port, game_state))
    t.daemon = True  # die with main thread
    t.start()

    # Now do our own "Game Loop"
    log.info("Original thread game loop:")
    while True:
        with state_lock:
            state = game_state[0]
        log.info(state)
        time.sleep(1)


def state_client(hostname, state_port, game_state):
    context = zmq.Context()
    context.linger = 0
    state_address = make_address(hostname, state_port)
    # Using State socket
    log.info('Connecting to state socket at [{0}]...'.format(state_address))
    game_name = ""  # I'm hoping this applies to all games
    state_sock = context.socket(zmq.SUB)
    log.info("Subscribe")
    state_sock.setsockopt_string(zmq.SUBSCRIBE, game_name)
    log.info("Connect")
    state_sock.connect(state_address)
    log.info("Recv Loop init.")

    while True:
        msg_filter_b, state_b = state_sock.recv_multipart()
        new_game_state = game_state[0]

        try:
            new_game_state = json.loads(state_b.decode())
        except:
            continue

        with state_lock:
            game_state[0] = new_game_state

    return


if __name__ == '__main__':
    main()
