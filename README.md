# spacerace

Multiplayer Asteroids-like racing game for the 2015 ETD winter retreat.

    docker build -t="spacerace" .
    docker run -it -p 5556:5556 -p 5558:5558 -p 5559:5559 spacerace


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


##Lobby
TODO -- connect to lobby with a REQ socket. Send a message with an ascii string
containing your ships name. The server will send a reply with 2 frames -- the
first is a secret key used to make sure only you can control your ship, and the
second is the map specification for the next game.

Once this has happened you're ready to play, start listening on the game state
socket for the beginning of the game!

## GameState

Is a plain json object. There is one key **ships** - an array of 
Player State objects:

### Player State

- **id [string]** - The player's chosen identifier.
- **x, y** - position
- **vx, vy** - linear velocity
- **theta** - orientation
- **omega** - rotational velocity
- **Tl** - is accelerating (linear thrusters are on). `0 or 1`
- **Tr** - is rotating (rotational thrusters on). `-1 or 0 or 1`

All values are JSON Numbers - not strings.


## Control Input

Control messages are sent from a client to the server during a game via a
zeromq "push" socket. The control input is a simple comma separated string.
The string is of the form:

    <yoursecretkey>,<main_engine>,<rotation>


- `<yoursecretkey>` is the string you were given by the lobby upon connection
- `<main_engine>` is either a 0 or a 1, for the main engine being off or on
- `<rotation>` is either a -1, 0 or 1. 1 is for +ve (anti-clockwise) rotation
thrust, -1 is for -ve (clockwise) rotation thrust, and 0 is no rotation thrust



