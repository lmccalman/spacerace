#! /usr/bin/env python3
""" Turn a bitmap of appropriate format into a map for the spacerace game. """


import click
import skfmm
import numpy as np
import matplotlib.pyplot as pl
from skimage import io, measure, filters


@click.command()
@click.argument('image')
@click.option('--mapname', default=None, help='Output map file name, same as '
                                              'IMAGE.npy unless specified.')
def buildmap(image, mapname):

    print(image, mapname)

    # Read in map
    mapim = io.imread(image)
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

    fig, ax = pl.subplots()
    ax.imshow(occmap, interpolation='nearest', cmap=pl.cm.gray)

    # Calculate start->end potential field
    distfromend = -skfmm.distance(np.ma.MaskedArray(~endim, occmap), dx=1e-2)
    dfx, dfy = np.gradient(distfromend)
    # dfx, dfy = -dfx, -dfy  # reverse flow field to end


    # blurmap = filters.gaussian_filter(occmap, sigma=3.0)
    # contours = measure.find_contours(blurmap, 0.5)
    # for contour in contours:
    #     ax.plot(contour[:, 1], contour[:, 0], linewidth=2)

    # ax.axis('image')
    # conmap = np.zeros_like(blurmap, dtype=bool)
    # for contour in contours:
    #     crow = np.round(contour[:, 0]).astype(int)
    #     ccol = np.round(contour[:, 1]).astype(int)
    #     conmap[crow, ccol] = True

    # Calculate wall normals
    distmap = skfmm.distance(~occmap, dx=1e-2)

    # Calculate distance to walls
    dnx, dny = np.gradient(np.ma.MaskedArray(distmap, occmap))

    # plotting
    skip = (slice(None, None, 30), slice(None, None, 30))
    x, y = np.mgrid[0:w, 0:h]

    pl.figure()
    pl.quiver(y[skip], x[skip], dny[skip], dnx[skip], color='r', angles='xy')
    pl.gca().invert_yaxis()
    pl.imshow(distmap, interpolation='none', cmap=pl.cm.gray)
    pl.colorbar()

    pl.figure()
    pl.quiver(y[skip], x[skip], dfy[skip], dfx[skip], color='r', angles='xy')
    pl.gca().invert_yaxis()
    pl.imshow(distfromend, interpolation='none', cmap=pl.cm.gray)
    pl.colorbar()

    pl.show()
    # Save layers

    # import IPython; IPython.embed()


if __name__ == "__main__":
    buildmap()
