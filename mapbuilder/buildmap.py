#! /usr/bin/env python3
""" Turn a bitmap of appropriate format into a map for the spacerace game. """


import sys
import click
import json
import skfmm
import numpy as np
import matplotlib.pyplot as pl
from scipy.misc import imread, imsave
from scipy.ndimage.filters import gaussian_filter


@click.command()
@click.argument('image')
@click.option('--mapname', default=None, help='Output map file name, same as '
                                              'IMAGE.npy unless specified.')
@click.option('--settingsfile', default='../spacerace.json',
              help='Location of the spacerace settings JSON file.')
@click.option('--visualise', is_flag=True, help='Visualise the output?')
def buildmap(image, mapname, settingsfile, visualise):

    with open(settingsfile, 'r') as f:
        settings = json.load(f)

    # Read in map
    print("Reading and processing map image...")
    mapim = imread(image)
    if mapim.ndim < 3:
        print("The bitmaps need to be 3 layers!")
        sys.exit(1)
    if mapim.shape[2] > 3:
        mapim = mapim[:, :, 0:3]  # ignore alpha

    # Pad with border
    pwidth = settings['mapbuilder']['padPixels']
    mapim = np.pad(mapim, ((pwidth, pwidth), (pwidth, pwidth), (0, 0)),
                   'constant', constant_values=0)

    w, h, d = mapim.shape

    # Find start (full green pixels)
    startim = np.logical_and(mapim[:, :, 0] != 255, mapim[:, :, 1] == 255,
                             mapim[:, :, 2] != 255)

    # Find end (full red pixels)
    endim = np.logical_and(mapim[:, :, 0] == 255, mapim[:, :, 1] != 255,
                           mapim[:, :, 2] != 255)

    # Make occupancy grid (black pixels)
    occmap = np.logical_and(mapim[:, :, 0] == 0, mapim[:, :, 1] == 0,
                            mapim[:, :, 2] == 0)

    # Calculate distance to walls
    print("Calculating distance to walls...")
    distintowall = skfmm.distance(~occmap)
    distouttowall = skfmm.distance(occmap)
    distmap = distintowall - distouttowall

    # Calculate start->end potential field
    print("Calculating flow field to end...")
    distfromend = skfmm.distance(np.ma.MaskedArray(~endim, occmap))

    dfyi, dfxi = np.gradient(-distfromend)
    dfyo, dfxo = np.gradient(np.ma.MaskedArray(-distouttowall, ~occmap))
    dfy, dfx = norm_layer(combine_layers(dfyi, dfyo),
                          combine_layers(dfxi, dfxo))

    distfromend = distfromend.filled(0) + distouttowall

    # Calculate wall normals
    print("Calculating wall normals...")
    blurmap = gaussian_filter((~occmap).astype(float),
                              sigma=settings['mapbuilder']['normalBlur'])

    dny, dnx = norm_layer(*np.gradient(blurmap))

    # plotting
    if visualise:
        print("Making plots...")
        skip = (slice(None, None, 20), slice(None, None, 20))
        y, x = np.mgrid[0:w, 0:h]

        fig = pl.figure()
        fig.add_subplot(231)
        pl.imshow(occmap, cmap=pl.cm.gray, interpolation='none')
        pl.title('Occupancy map')

        fig.add_subplot(232)
        pl.imshow(startim, cmap=pl.cm.gray, interpolation='none')
        pl.title('Start point')

        fig.add_subplot(233)
        pl.imshow(endim, cmap=pl.cm.gray, interpolation='none')
        pl.title('End point')

        fig.add_subplot(234)
        pl.quiver(x[skip], y[skip], dnx[skip], dny[skip], color='r',
                  angles='xy')
        pl.gca().invert_yaxis()
        pl.imshow(distmap, interpolation='none', cmap=pl.cm.gray)
        pl.title('Distance to walls, wall normals')

        fig.add_subplot(235)
        pl.quiver(x[skip], y[skip], dfx[skip], dfy[skip], color='r',
                  angles='xy')
        pl.gca().invert_yaxis()
        pl.imshow(distfromend, interpolation='none', cmap=pl.cm.gray)
        pl.title('Distance to end, flow to end')

        fig.add_subplot(236)
        pl.imshow(mapim, interpolation='none')
        pl.title('Original map image')

        pl.show()

    # Process output names
    print("Saving results...")
    mapnamebits = image.partition('.')
    mapname = mapnamebits[0] if mapname is None else mapname.partition[0]

    # import IPython; IPython.embed()

    # Save padded map
    print('\t- padded image')
    imsave(mapname+'_padded.'+mapnamebits[2], mapim)

    # Binary layers for engine
    print('\t- binary layers')
    save_bool_map(mapname+'_start', startim)
    save_bool_map(mapname+'_end', endim)
    save_bool_map(mapname+'_occupancy', occmap)
    save_float32_map(mapname+'_walldist', distmap)
    save_float32_map(mapname+'_enddist', distfromend)
    save_float32_map(mapname+'_wnormx', dnx)
    save_float32_map(mapname+'_wnormy', dny, vec=True)
    save_float32_map(mapname+'_flowx', dfx)
    save_float32_map(mapname+'_flowy', dfy, vec=True)

    # Easy to read formats for players
    print('\t- csv layers')
    save_text_bool_map(mapname+'_start', startim)
    save_text_bool_map(mapname+'_end', endim)
    save_text_bool_map(mapname+'_occupancy', occmap)
    save_text_float32_map(mapname+'_enddist', distfromend)
    save_text_float32_map(mapname+'_flowx', dfx)
    save_text_float32_map(mapname+'_flowy', dfy, vec=True)

    print("Done!")


def combine_layers(l1, l2):

    return l1.filled(0) + l2.filled(0)


def norm_layer(y, x):

    norms = np.sqrt((y**2 + x**2))
    zmask = ~(norms == 0)
    x[zmask] = x[zmask] / norms[zmask]
    y[zmask] = y[zmask] / norms[zmask]

    return y, x


def save_bool_map(filename, boolmap):
    np.save(filename, np.flipud(boolmap.astype(bool)))


def save_float32_map(filename, floatmap, vec=False):
    arr = np.flipud(floatmap.astype('float32'))

    # if we are flipping a vector quantity about the y-axis
    if vec:
        arr = -arr

    np.save(filename, arr)


def save_text_float32_map(filename, maparray, vec=False):
    arr = np.flipud(maparray.astype('float32'))

    # if we are flipping a vector quantity about the y-axis
    if vec:
        arr = -arr

    np.savetxt(filename+'.csv.gz', arr)


def save_text_bool_map(filename, boolmap):
    np.savetxt(filename+'.csv.gz', np.flipud(boolmap.astype(bool)))


if __name__ == "__main__":
    buildmap()
