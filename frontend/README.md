# Spacerace UI

TODO:

- Highlight a player
- Number of players connected in lobby for next game etc
- Who is playing in current game
- webgl

## Running the UI

Install the node dependencies:
    
    $ npm install
 
Build the static front end content:

    $ webpack

Start the zmq -> socket.io and static asset server:

    $ node server.js

The front end should be visible at <http://localhost:8000>
The project level folder `maps` will be served from `http://localhost:8000/maps/`


## Communication

Since the server will be offering zmq sockets, the webserver will
also port the stream to a websocket connection.


## Frontend Libraries

- D3 (SVG first, then explore `three.js`)
