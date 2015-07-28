#! /usr/bin/env python3
""" Turn a bitmap of appropriate format into a map for the spacerace game. """


import click
import json
import skfmm
import numpy as np
import matplotlib.pyplot as pl
from scipy.misc import imread
from scipy.ndimage.filters import gaussian_filter


@click.command()
@click.argument('image')
@click.option('--mapname', default=None, help='Output map file name, same as '
                                              'IMAGE.npy unless specified.')
@click.option('--settingsfile', default='../server/spacerace.json',
              help='Location of the spacerace settings JSON file.')
@click.option('--visualise', is_flag=True, help='Visualise the output?')
def buildmap(image, mapname, settingsfile, visualise):

    with open(settingsfile, 'r') as f:
        settings = json.load(f)

    # Read in map
    mapim = imread(image)
    if mapim.shape[2] > 3:
        mapim = mapim[:, :, 0:3]  # ignore alpha

    w, h, _ = mapim.shape

    # Find start (full green pixels)
    startim = np.logical_and(mapim[:, :, 0] != 255, mapim[:, :, 1] == 255,
                             mapim[:, :, 2] != 255)

    # Find end (full red pixels)
    endim = np.logical_and(mapim[:, :, 0] == 255, mapim[:, :, 1] != 255,
                           mapim[:, :, 2] != 255)

    # Make occupancy grid (black pixels)
    occmap = np.logical_and(mapim[:, :, 0] == 0, mapim[:, :, 1] == 0,
                            mapim[:, :, 2] == 0)

    # Calculate start->end potential field
    distfromend = skfmm.distance(np.ma.MaskedArray(~endim, occmap), dx=1e-2)
    dfx, dfy = np.gradient(-distfromend)

    # Calculate distance to walls
    distmap = np.ma.MaskedArray(skfmm.distance(~occmap, dx=1e-2), occmap)

    # Calculate wall normals
    blurmap = gaussian_filter((~occmap).astype(float),
                              sigma=settings['map']['normalBlur'])
    dnx, dny = np.gradient(np.ma.MaskedArray(blurmap, occmap), edge_order=2)
    norms = np.sqrt((dny**2 + dnx**2))
    dnx /= norms
    dny /= norms

    # plotting
    if visualise:
        skip = (slice(None, None, 20), slice(None, None, 20))
        x, y = np.mgrid[0:w, 0:h]

        fig = pl.figure()
        fig.add_subplot(231)
        pl.imshow(occmap, cmap=pl.cm.gray)
        pl.title('Occupancy map')

        fig.add_subplot(232)
        pl.imshow(startim, cmap=pl.cm.gray)
        pl.title('Start point')

        fig.add_subplot(233)
        pl.imshow(endim, cmap=pl.cm.gray)
        pl.title('End point')

        fig.add_subplot(234)
        pl.quiver(y[skip], x[skip], dny[skip], dnx[skip], color='r',
                  angles='xy')
        pl.gca().invert_yaxis()
        pl.imshow(distmap, interpolation='none', cmap=pl.cm.gray)
        pl.title('Distance to walls, wall normals')

        fig.add_subplot(235)
        pl.quiver(y[skip], x[skip], dfy[skip], dfx[skip], color='r',
                  angles='xy')
        pl.gca().invert_yaxis()
        pl.imshow(distfromend, interpolation='none', cmap=pl.cm.gray)
        pl.title('Distance to end, flow to end')

        fig.add_subplot(236)
        pl.imshow(mapim)
        pl.title('Original map image')

        pl.show()

    # Save layers
    mapname = image.rpartition('.')[0] if mapname is None else mapname
    np.save(mapname+'_start', startim)
    np.save(mapname+'_end', endim)
    np.save(mapname+'_occupancy', occmap)
    np.save(mapname+'_walldist', distmap.filled(np.NaN))
    np.save(mapname+'_enddist', distfromend.filled(np.NaN))
    np.save(mapname+'_wnormx', dnx.filled(np.NaN))
    np.save(mapname+'_wnormy', dny.filled(np.NaN))
    np.save(mapname+'_flowx', dfx.filled(np.NaN))
    np.save(mapname+'_flowy', dfy.filled(np.NaN))


if __name__ == "__main__":
    buildmap()
