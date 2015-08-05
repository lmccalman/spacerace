# spacerace

Spacerace is a multiplayer Asteroids-like racing game for the 2015 ETD winter
retreat.


## Overview

This is a multi-round racing game in which the game state is managed from a
central game server. The aim of this game is for the player's spaceship to
complete the track before anyone else's, and within the time limit of the
round. The round ends as soon as the first player reaches a finish, or the time
is up. Then each player is assigned a score which is the percentage of the
track they completed by the finish of the round. 

The game involves controlling a spaceship that has a main thruster to
accelerate it forward, and small rotational thrusters to spin it. The world is
2D and flat, and the ships encounter air resistance that limits their maximum
velocity and turn rate.

There is no friction on the track, and so ships behave much like a hovercraft.
However the walls are (semi) elastic, and the ships will bounce off them. Ships
can also collide with eachother without damage. Spin forces are reasonably well
approximated, so glancing off a wall or another ship can send your ship into a
spin.

Multiple ships can form a team, and the game keeps both a team score and an
individual ship score. Clients are welcome to add multiple ships to the game,
but the server may limit the total number of clients in a game or playing for a
particular team.

The game cycles through a list of different maps. If a player tries to join a
whilst a game is in progress, they will be added to the lobby for the next
game.


## Coordinate System

We use "mathematical" co-ordinate conventions. These may be different from the
conventions used in computer graphics. With respect to your screen, the
coordinate system is described below:


![Coodinate system](coords.png)


## General Instructions for Building a Client

A minimal client has three [ZeroMQ](http://zeromq.org/) sockets:

1. *Lobby*: a request socket, asks for a connection to the next game, receives 
confirmation of the next game that will be joined.
2. *Game state*: subscription ("sub") socket, receive a JSON object of the 
current games state (details below).
3. *Control*: a "push" socket, push your client control actions as a ZeroMQ
frame (see below for details).

There is an additional (optional) *info* socket that may send useful error and
status messages to debug clients.

The pseudo-code of a very simple single-threaded client is given below:

### Pseudo code

    while(true) // loop that connects and plays in every round of the game
        send a connection message over the lobby socket
        receive a confirmation message from the lobby about your next game
        subscribe your state socket to the game name given by the lobby
        running = true
        while(running)
          receive the game state from the state socket
          send a control command on the control socket 
          set running = false if the game has finished

Each of these sockets, and the message formats they send and expect to receive,
are outlined in the following sections.


### Lobby socket

Some more detailed information on how a game is run:

**1.** Connect to the lobby by sending:

```
    { 
        "name": "yournamehere",
        "team": "yourteam"
    }
```

**2.** You will receive the following confirmation json:

```
    {
        "name": "yournamehere",
        "game": "gamename",
        "map": "mapname",
        "secret": "yoursecretkey"
    }
```

If there is an error TODO.


### State Socket

Then you will need to subscribe your *state* socket to "gamename".

**3.** Some time later when the game starts, you will start receiving game
state on the state socket. The game state will be received periodically (approx
60 Hz), with the following format,

```
    {
        "state": "running",
        "data": [list_of_players]
    }
```

A player has the following attributes:

```
    {
        "id": "xxxx",
        "x":
        "y":
        "vx":
        "vy":
        "theta": float,     (heading, angle w.r.t. counter-clockwise from horizontal)
        "omega": float,     (angular velocity)
        "Tl": float,        ({0,1} for linear thrust off/on)
        "Tr": float         ({-1, 0, 1} for angular thrust clockwise/off/ counter-clockwise)
    }
```


### Control Socket

Sending a ship control message. The following is sent to the control "push"
socket, which is a single ZMQ frame,

```
    <yoursecretkey>,<main_engine>,<rotation>
```

- `<yoursecretkey>` is the string you were given by the lobby upon connection
- `<main_engine>` is either a 0 or a 1, for the main engine being off or on
- `<rotation>` is either a -1, 0 or 1. 1 is for +ve (anti-clockwise) rotation
thrust, -1 is for -ve (clockwise) rotation thrust, and 0 is no rotation thrust

The last command is repeated each time-step, and only the last command received
in each time step is used.

**5.** Game ending, this happens when either a player finishes the track or the
game times out. Then you will receive the following on the state socket;

```
    {
        "state": "finished"
        some other end stuff here, like score etc. TODO
    }
```


### Info Socket

TODO

## Maps

The maps are raster data, and will be stored in a location that we will
communicate on the day. The map name that will be in the next round is
communicated upon connection to the lobby in the confirmation JSON object (the
"map" property), see point **2** above.

It is then up to you to download the map information from the supplied
location. There are a number of bitmap and (compressed) CSV files that convey
different information about the upcoming round that you may find useful
(especially for path planning).

Also, if you don't feel like making an AI, please have a go at making a map, or
even a front-end to visually display the game state!


### Game Map Files

The following files are associated with each map. There are two file types,
`.png` or any bitmap format and `.csv.gz` are gzipped space-separated 32-bit 
float value files:
- `mapname.png` the actual track bitmap 
- `mapname_start.csv.gz` bitmask of the location(s) of the start
- `mapname_end.csv.gz` bitmask of the location(s) of the end
- `mapname_occupancy.csv.gz` bitmask of the obstacles
- `mapname_enddist.csv.gz` distance to nearest end (pixels)
- `mapname_flowx.csv.gz` flow-field to nearest end, unit vector horizontal
  component 
- `mapname_flowy.csv.gz` flow-field to nearest end, unit vector vertical
  component

Here is an example of this data (you will not have access to the track wall
normals/distance to walls);

![Example map products.](mapbuilder/example_map_products.png)


### How to contribute a map

It's easy! For the actual track, you just need to create a bitmap (preferably
PNG but any image format will do) that conforms to the following
specifications:

- Occupied regions must be **black** `#000000, rgb(0, 0, 0)`
- Free/race track regions must be **white** `#FFFFFF, rgb(255, 255, 255)`
- Start position(s) must be **green** `#00FF00, rgb(0, 255, 0)`
- End position(s) must be **red** `#FF0000, 
  rgb(255, 0, 0)`

Also, we haven't put a constraint on the size, but I recommend less than 
1500px X 1500px. Here is an example:

![Example map](mapbuilder/testmap.png)

To then make this map readable by the game engine, you need to run the
`buildmap.py` script in the `mapbuilder` directory. Here is an example (from
where you cloned this repo),

    $ cd mapbuilder
    $ ./buildmap pathto/yourmapname.png --visualise

This will then output all of the necessary files into the same directory as
your map. You can call `./buildmap.py --help` for more info. Also, have a look
at `mapbuilder/requirements.txt` for all of the required python packages.

Finally, you can optionally provide a skin for you map to make it look pretty!
Just make sure it is the same size as your original map and has the suffix
`_skin`, e.g.

![Example map skin](mapbuilder/testmap_skin.png)

Now just upload all of the generated files to the location we will specify!




# Docker
    docker build -t spacerace .
    docker run -it -p 5556-5559:5556-5559 spacerace 
