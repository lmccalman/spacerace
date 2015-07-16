# spacerace
Multiplayer Asteroids-like racing game for the 2015 ETD winter retreat.



# Payload Specifications

The primary state content for game will be stored within two JSON objects, a Map, and 
a GameState. The following specifies the format of these JSON objects for version `1.0`.

## Map object

The top level JSON object shall comprise:
- `version`: The version of the Map format, as a string. (e.g., `"1.0"`)
- `layers`: An array of **Map Layers**. Format specified below.
- `start`: A line describing the start point
- `finish`: A line describing the end point
- `raceline`: A spline that (non-optimally) links `start` to `finish`. 


### Map Layers

A JSON object containing at least a `type` attribute. The `type`
attribute should be one of the following strings:

- `raster`
- `vector`
- `diagram`

Each of these item types represents completely different information
and is specified independently.

#### `raster`

TODO - Just basic occupancy, or include start zone etc.

### `vector`

TODO - Probably SVG

Optional attributes:
- `style`: TODO


## GameState

TODO - but will include an array of Players' State objects

### Player State

- position
- velocity
- orientation

- is accelerating TODO:should this be observable?
- is rotating TODO:should this be observable?
