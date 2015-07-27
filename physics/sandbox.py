""" Physics test sandbox for the space race game!
    Alistair Reid 2015
"""
import matplotlib.pyplot as pl
import numpy as np
from numpy.linalg import norm
from time import time


def integrate(states, props, inp, bounds, dt):
    """ Implementing 4th order Runge-Kutta for a time stationary DE.
    """
    derivs = lambda y: physics(y, props, inp, bounds)
    k1 = derivs(states)
    k2 = derivs(states + 0.5*k1*dt)
    k3 = derivs(states + 0.5*k2*dt)
    k4 = derivs(states + k3*dt)
    states += (k1 + 2*k2 + 2*k3 + k4)/6. * dt


def physics(states, props, inp, B):

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

    # Physics model parameters
    rho = 0.1  # Air density (or absorb into cd_a?)
    k_elastic = 4000.  # spring normal force
    spin_drag_ratio = 4.  # spin drag to forward drag
    eps = 1e-8  # avoid divide by zero warnings
    mu = 0.2  # coefficient of friction (tangent force/normal force)
    sigmoid = lambda x: -1 + 2.*np.exp(x)/(1. + np.exp(x))

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
    # TODO (Al): integrate with Dan-code
    normals = ((1, 0), (-1, 0), (0, 1), (0, -1))
    normals = [np.array(q) for q in normals]
    offsets = (B[0], -B[1], B[2], -B[3])
    for i in range(n):
        for normal, offset in zip(normals, offsets):
            dist = np.dot(P[i], normal) - offset - rad[i]
            if dist < 0:

                # Linear spring normal force
                f_norm_mag = -dist*k_elastic
                f[i] += f_norm_mag * normal

                # surface tangential force
                perp = [-normal[1], normal[0]]  # points left 90 degrees
                v_rel = W[i] * rad[i] - np.dot(V[i], perp)
                fric = f_norm_mag * mu * sigmoid(v_rel)
                f[i] += fric*perp
                trq[i] -= fric * rad[i]

    # Compose the gradient vector
    return np.hstack((V, W, f/m, trq/I))


def shortlist_collisions(P, r):
    # Use spatial hashing to shortlist possible collisions
    # Requires radius = 1 even though I originally allowed it to vary.
    n = P.shape[0]
    all_cells = dict()  # potential collisions
    checks = set()
    r = 1.  # assume all radius 1
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


def main():
    n = 30
    masses = 1. + 2*np.random.random(n)
    Is = masses/2.
    radius = np.ones(n)
    cda = np.ones(n)
    properties = np.vstack((masses, Is, radius, cda)).T
    colours = ['r', 'b', 'g', 'c', 'm', 'y', 'k']*5
    colours = colours[:n]
    # x, y, th, vx, vy, w
    x0 = np.random.random(n) * 10 - 5
    y0 = np.random.random(n) * 10 - 5
    th0 = np.random.random(n) * np.pi * 2
    vx0 = np.random.random(n) * 2 - 1
    vy0 = np.random.random(n) * 2 - 1
    w0 = np.random.random(n) * 2 - 1
    states0 = np.vstack((x0, y0, th0, vx0, vy0, w0)).T
    bounds = [-12, 12, -28, 28]

    # Set up our spaceships
    pl.figure()
    ax = pl.subplot(111)
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
    t = 0
    t_max = 50
    states = states0
    while t < t_max:
        inputs = np.zeros((n, 2))
        inputs[:, 1] = 3.0  # try to turn
        inputs[:, 0] = 25  # some forward thrust!
        while t < time() - start_time:
            t += dt
            integrate(states, properties, inputs, bounds, dt)
        for state, col, r, sprite in zip(states, colours, radius, sprites):
            draw_outline(state, col, r, handle=sprite)
        pl.draw()
    print('Time is up!')


def draw_outline(state, c, radius, handle=None, n=25):
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


if __name__ == '__main__':
    main()
