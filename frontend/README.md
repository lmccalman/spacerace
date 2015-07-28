# Spacerace UI

TODO:

- Number of players connected

## Running the UI

Install the node dependencies:
    
    $ npm install
 
Build the static front end content:

    $ webpack --colors

Start the zmq -> socket.io and static asset server:

    $ node websocket-server.js

The front end should be visible at <http://localhost:8000>

## Communication

Since the server will be offering zmq sockets, the webserver will
also port the stream to a websocket connection.


## Frontend Libraries

- D3 (SVG first, then explore `pathgl`)
