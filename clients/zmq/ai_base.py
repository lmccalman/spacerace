import matplotlib.pyplot as plt
import argparse
import logging
import client
import numpy as np
# from scipy.interpolate import interp2d
import ship
import time
import matplotlib.pyplot as pl
import sys
import time
logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)


def main(args):

    context = client.make_context()

    # Users may want to use this client to control more than one ship
    game = client.Client(args.hostname, args.lobby_port,
                         args.control_port, args.state_port, context)
    my_name = args.ship_name
    base_ship = ship.Ship(game, my_name, args.team_name, args.password)
    response = game.lobby.register(args.ship_name, args.team_name,
                                    args.password)
    print(response)
    # Subscribe to the game state
    game.state.subscribe(response.game)

    # load the map:
    mapname = response['map']
    map_path = '/home/areid/code/spacerace/maps/'
    maps = map_path+mapname
    # load the flow field
    # you may also want + '_wnormx' + '_wnormy' + '_flowx' + '_flowy'
    # _goal or _end
    x_map = np.load(maps+'_flowx.npy')
    y_map = np.load(maps+'_flowy.npy')
    occupancy0 = np.load(maps + '_occupancy.npy')


    ctrl = 0
    dt = 0.01
    go = True
    timeStart = 0.
    while True:
        print('Getting state')
        state = game.state.recv()
        print('Got state')

        # Idle until game start
        if state['state'] != 'running':
            if not go:
                print('Race over...')
                break
            time.sleep(0.1)
            continue
        if go:
            go = False
            print('Have a head start...!')
            timeStart = time.time()
            continue

        # Control the attitude of the craft
        alldata = state['data']
        data = alldata[my_name]  # find self in the list
        ship_x = data['x']
        ship_y = data['y']
        ship_vx = data['vx']
        ship_vy = data['vy']
        flow_xh = x_map[int(ship_y), int(ship_x)]
        flow_yh = y_map[int(ship_y), int(ship_x)]
        ship_theta = data['theta']
        cos_th = np.cos(ship_theta)  # ship X component should be this
        sin_th = np.sin(ship_theta)  # ship y component should be this
        fwd_vel = ship_vx*cos_th + ship_vy*sin_th
        # Cross product of ship vector with flow
        cross = (-flow_xh * sin_th + flow_yh * cos_th)
        omega = data['omega']


        # your ai goes here - 
        thrust = 1  # ... [0 or 1]
        rotation = -1 ##... [-1, 0 or 1]
        
        # I suggest some sort of pulse modulation eg
        # thrust = int(np.random.random() < thrust_level)

        # Send results
        print((thrust, rotation))
        base_ship.send_control(thrust, rotation)
        time.sleep(dt)

    return()


if __name__ == '__main__':

    # Process Args ***************************************************
    parser = argparse.ArgumentParser(
        description='Spacerace: Manned Spacecraft'
    )
    DEFAULTS = client.DEFAULTS
    DEFAULTS['hostname'] = '127.0.0.1'  # 'geothermaltv.dynhost.nicta.com.au'
    DEFAULT_NAME = "Default "  + str(np.random.randint(1e4))
    DEFAULT_TEAM = "Rebellion"
    DEFAULT_PASSWORD = "PASSWORD" + DEFAULT_NAME
    parser.add_argument('--version', action='version', version='%(prog)s 1.0')
    parser.add_argument('--hostname', type=str, help='Server hostname',
                        default=DEFAULTS['hostname'])
    parser.add_argument('--state_port', type=int, help='State port',
                        default=DEFAULTS['state_port'])
    parser.add_argument('--control_port', type=int, help='Control port',
                        default=DEFAULTS['control_port'])
    parser.add_argument('--lobby_port', type=int, help='Lobby port',
                        default=DEFAULTS['lobby_port'])
    parser.add_argument('--ship_name', '-s', type=str, default=DEFAULT_NAME,
                        help='Ship Name')
    parser.add_argument('--team_name', '-t', type=str, default=DEFAULT_TEAM,
                        help='Team Name')
    parser.add_argument('--password', '-p', type=str,
                        default=DEFAULT_PASSWORD, help='Password')
    args = parser.parse_args()
    logger.debug(args)

    # ******************************************************************

    main(args)

