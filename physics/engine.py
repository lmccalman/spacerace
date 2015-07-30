""" Physics test sandbox for the space race game!
    Alistair Reid 2015
"""
import matplotlib.pyplot as pl
import numpy as np
from numpy.linalg import norm
from time import time, sleep
import os

def integrate(states, props, inp, walls, bounds, dt):
    """ Implementing 4th order Runge-Kutta for a time stationary DE.
    """
    derivs = lambda y: physics(y, props, inp, walls, bounds)
    k1 = derivs(states)
    k2 = derivs(states + 0.5*k1*dt)
    k3 = derivs(states + 0.5*k2*dt)
    k4 = derivs(states + k3*dt)
    states += (k1 + 2*k2 + 2*k3 + k4)/6. * dt


def physics(states, props, inp, walls, bounds):

    # Unpack state, input and property vectors
    P = states[:, :2]
    Th = states[:, 2:3]
    V = states[:, 3:5]
    W = states[:, 5:6]
    m = props[:, 0:1]
    I = props[:, 1:2]
    rad = props[:, 2:3]
    cd_a = props[:, 3:4]  # coeff drag * area
    f = inp[:, :1] * np.hstack((np.cos(Th), np.sin(Th)))
    trq = inp[:, 1:2]
    n = P.shape[0]

    # Physics model parameters (hand tuned to feel right)
    rho = 0.1  # Air density (or absorb into cd_a?)
    k_elastic = 4000.  # spring normal force
    spin_drag_ratio = 1.8  # spin drag to forward drag
    eps = 1e-5  # avoid divide by zero warnings
    mu = 0.05  # coefficient of friction (tangent force/normal force)
    mu_wall = 0.01  # wall friction param
    sigmoid = lambda x: -1 + 2./(1. + np.exp(-x))

    # Compute drag
    f -= cd_a * rho * V * norm(V, axis=1)[:, np.newaxis]
    trq -= spin_drag_ratio*cd_a * rho * W * np.abs(W) * rad**2

    # Inter-ship collisions
    checks = shortlist_collisions(P, 1.)  # Apply test spatial hashing
    for i, j in checks:
        dP = P[j] - P[i]
        dist = norm(dP) + eps
        diameter = rad[i] + rad[j]
        if dist < diameter:

            # Direct collision: linear spring normal force
            f_magnitude = (diameter-dist)*k_elastic
            f_norm = f_magnitude * dP
            f[i] -= f_norm
            f[j] += f_norm

            # Spin effects (ask Al to draw a free body diagram)
            perp = np.array([-dP[1], dP[0]])/dist  # surface perpendicular
            v_rel = rad[i]*W[i] + rad[j]*W[j] + np.dot(V[i] - V[j], perp)
            fric = f_magnitude * mu * sigmoid(v_rel)
            f_fric = fric * perp
            f[i] += f_fric
            f[j] -= f_fric
            trq[i] -= fric * rad[i]
            trq[j] -= fric * rad[j]

    # Wall collisions --> single body collisions
    wall_info = linear_interpolate(walls, bounds, P)

    # import IPython
    # IPython.embed()
    # exit()

    for i in range(n):
        dist = wall_info[i][0] - rad[i]
        if dist < 0:
            normal = wall_info[i][1:3]

            # Linear spring normal force
            f_norm_mag = -dist*k_elastic
            f[i] += f_norm_mag * normal

            # surface tangential force
            perp = [-normal[1], normal[0]]  # points left 90 degrees
            v_rel = W[i] * rad[i] - np.dot(V[i], perp)
            fric = f_norm_mag * mu_wall * sigmoid(v_rel)
            f[i] += fric*perp
            trq[i] -= fric * rad[i]

    # Compose the gradient vector
    return np.hstack((V, W, f/m, trq/I))


def shortlist_collisions(P, r):
    # Use spatial hashing to shortlist possible collisions
    n = P.shape[0]
    all_cells = dict()  # potential collisions
    checks = set()
    grid = r * 2. + 1e-5  # base off diameter
    offsets = r*np.array([[1,1],[1,-1], [-1,1], [-1,-1]])
    for my_id in range(n):
        bins = [tuple(m) for m in np.floor((P[my_id] + offsets)/grid)]
        for bin in bins:
            if bin in all_cells:
                for friend in all_cells[bin]:
                    checks.add((my_id, friend))
                all_cells[bin].append(my_id)
            else:
                all_cells[bin] = [my_id]
    return checks


def main():

    resources = os.getcwd()[:-8]+'/mapbuilder/testmap_%s.npy'

    wnx = np.load(resources % 'wnormx')
    wny = np.load(resources % 'wnormy')
    norm = np.sqrt(wnx**2 + wny**2) + 1e-5
    wnx /= norm
    wny /= norm
    wdist = np.load(resources % 'walldist')

    mapscale = 10
    walls = np.dstack((wdist/mapscale, wnx, wny))

    map_img = np.load(resources % 'occupancy')  # 'walldist')
    all_shape = np.array(map_img.shape).astype(float) / mapscale
    bounds = [0, all_shape[1], 0, all_shape[0]]
    map_img = 0.25*(map_img[::2, ::2] + map_img[1::2,::2] + \
                    map_img[::2, 1::2] + map_img[1::2, 1::2])
    spawn = np.array([25, 25])/2.  # x, y
    spawn_size = 6/2.
    

    n = 30
    masses = 1. + 2*np.random.random(n)
    masses[0] = 1.
    Is = 0.25*masses
    radius = np.ones(n)
    cda = np.ones(n)
    properties = np.vstack((masses, Is, radius, cda)).T
    colours = ['r', 'b', 'g', 'c', 'm', 'y']
    colours = (colours * np.ceil(n/len(colours)))[:n]
    colours[0] = 'k'
    # x, y, th, vx, vy, w
    x0 = 2*(np.random.random(n) - 0.5) * spawn_size + spawn[0]
    y0 = 2*(np.random.random(n) - 0.5) * spawn_size + spawn[1]
    th0 = np.random.random(n) * np.pi * 2
    vx0 = np.random.random(n) * 2 - 1
    vy0 = np.random.random(n) * 2 - 1
    w0 = np.random.random(n) * 2 - 1
    states0 = np.vstack((x0, y0, th0, vx0, vy0, w0)).T

    # Set up our spaceships
    fig = pl.figure()
    ax = pl.subplot(111)

    # Draw the backdrop:
    # pl.imshow(-map_img, extent=bounds, cmap=pl.cm.gray, origin='lower')
    cx = np.linspace(bounds[0], bounds[1], map_img.shape[1])
    cy = np.linspace(bounds[2], bounds[3], map_img.shape[0])
    cX, cY = np.meshgrid(cx, cy)
    pl.contour(cX, cY, map_img, 1)

    sprites = []
    for s, col, r in zip(states0, colours, radius):
        vis = draw_outline(s, col, r)
        sprites.append(vis)
    ax.set_xlim(bounds[0:2])
    ax.set_ylim(bounds[2:4])
    ax.set_aspect('equal')
    pl.show(block=False)

    dt = 0.02
    start_time = time()
    t = 0.
    states = states0
    event_count = 0
    frame_rate = 30.
    frame_time = 1./frame_rate
    next_draw = frame_time
   
    # exit gracefully
    keys = set()
    def press(event):
        keys.add(event.key)
    def unpress(event):
        keys.remove(event.key)

    fig.canvas.mpl_connect('key_press_event', press)
    fig.canvas.mpl_connect('key_release_event', unpress)
    print('Press Q to exit')

    while 'q' not in keys:

        # Advance the game state
        while t < next_draw:
            inputs = np.zeros((n, 2))
            inputs[:, 1] = 3.0  # try to turn
            inputs[:, 0] = 100  # some forward thrust!

            # give the user control of ship 0
            if 'right' in keys:
                inputs[0, 1] = -10.
            elif 'left' in keys:
                inputs[0, 1] = 10.
            else:
                inputs[0, 1] = 0.
            if 'up' in keys:
                inputs[0,0] = 100
            else:
                inputs[0,0] = 0

            t += dt
            integrate(states, properties, inputs, walls, bounds, dt)
        
        # Draw at the desired framerate
        this_time = time() - start_time
        if this_time > next_draw:
            next_draw += frame_time
            for state, col, r, sprite in zip(states, colours, radius, sprites):
                draw_outline(state, col, r, handle=sprite)
            event_count += 1
            pl.pause(0.001)
        else:
            sleep(next_draw - this_time)


def draw_outline(state, c, radius, handle=None, n=15):
    x, y, th, vx, vy, w = state
    # m, I, radius, c = props
    # base_theta = np.linspace(np.pi-2, np.pi+2, n-1)
    # base_theta[0] = 0
    # base_theta[-1] = 0

    base_theta = np.linspace(0, 2*np.pi, n)
    theta = base_theta + th + np.pi
    theta[0] = th
    theta[-1] = th

    # size = np.sqrt(1. - np.abs(base_theta - np.pi)/np.pi)
    size = 1
    vx = np.cos(theta) * radius * size
    vy = np.sin(theta) * radius * size
    vx += x
    vy += y
    if handle:
        handle.set_data(vx, vy)
    else:
        handle, = pl.plot(vx, vy, c)

    return handle


def linear_interpolate(img, bounds, pos):
    """ Used for interpreting Dan-maps
    Args:
        img - n*m*k
        bounds - (xmin, xmax, ymin, ymax)
        pos - n*2 position vector of query
    Returns:
        interpolated vector
    """
    h, w, ch = np.shape(img)
    xmin, xmax, ymin, ymax = bounds
    x, y = pos.T
    ix = (x - xmin) / (xmax - xmin) * (w - 1.)
    iy = (y - ymin) / (ymax - ymin) * (h - 1.)
    ix = np.minimum(np.maximum(0, ix), w-2)
    iy = np.minimum(np.maximum(0, iy), h-2)
    L = ix.astype(int)
    T = iy.astype(int)
    alphax = (ix - L)[:,np.newaxis]
    alphay = (iy - T)[:,np.newaxis]
    out = (1.-alphax)*(1.-alphay)*img[T, L] + \
        (1.-alphax)*alphay*img[T+1, L] + \
        alphax*(1-alphay)*img[T, L+1] + \
        alphax*alphay*img[T+1, L+1]
    return out


def old_shortlist_collisions(P, r):
    # Use spatial hashing to shortlist possible collisions
    # Requires radius = 1 even though I originally allowed it to vary.
    n = P.shape[0]
    all_cells = dict()  # potential collisions
    checks = set()
    grid = r * 2. + 1e-5  # base off diameter
    UP = [tuple(v) for v in np.floor((P+[r, r]) / grid)]
    DOWN = [tuple(v) for v in np.floor((P+[r, -r]) / grid)]
    LEFT = [tuple(v) for v in np.floor((P+[-r, r]) / grid)]
    RIGHT = [tuple(v) for v in np.floor((P+[-r, -r]) / grid)]
    indx = list(range(n))
    for i, u, d, l, r in zip(indx, UP, DOWN, LEFT, RIGHT):
        for my_id in (u, d, l, r):
            if my_id in all_cells:
                for friend in all_cells[my_id]:
                    checks.add((i, friend))
                all_cells[my_id].append(i)
            else:
                all_cells[my_id] = [i]
    return checks


if __name__ == '__main__':
    main()
