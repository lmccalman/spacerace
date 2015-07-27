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

    # Find start (full green pixels)
    startim = np.logical_and(mapim[:, :, 0] != 255, mapim[:, :, 1] == 255,
                             mapim[:, :, 2] != 255)

    # Find end (full red pixels)
    endim = np.logical_and(mapim[:, :, 0] == 255, mapim[:, :, 1] != 255,
                           mapim[:, :, 2] != 255)

    # Make occupancy grid (black pixels)
    occmap = np.logical_and(mapim[:, :, 0] == 0, mapim[:, :, 1] == 0,
                            mapim[:, :, 2] == 0)

    # Calculate wall normals
    blurmap = filters.gaussian_filter(occmap, sigma=3.0)
    contours = measure.find_contours(blurmap, 0.5)

    # Look at:
    # http://stackoverflow.com/questions/30079740/image-gradient-vector-field-in-python
    # But can use distance to walls map

    fig, ax = pl.subplots()
    ax.imshow(occmap, interpolation='nearest', cmap=pl.cm.gray)

    for contour in contours:
        ax.plot(contour[:, 1], contour[:, 0], linewidth=2)

    ax.axis('image')

    # Calculate start->end potential field
    flowmap = skfmm.distance(np.ma.MaskedArray(~endim, occmap), dx=1e-2)

    pl.figure()
    pl.imshow(flowmap, interpolation='none', cmap=pl.cm.gray)

    # Calculate distance to walls
    # conmap = np.zeros_like(blurmap, dtype=bool)
    # for contour in contours:
    #     crow = np.round(contour[:, 0]).astype(int)
    #     ccol = np.round(contour[:, 1]).astype(int)
    #     conmap[crow, ccol] = True
    distmap = skfmm.distance(~occmap, dx=1e-2)

    pl.figure()
    pl.imshow(distmap, interpolation='none', cmap=pl.cm.gray)

    pl.show()
    # Save layers


    import IPython; IPython.embed()


if __name__ == "__main__":
    buildmap()
