# Spacerace UI


## Running the UI
 
For now in one terminal run the **dummy producer**:

    $ node producer.js

Then start the webserver:

    $ node websocket-server.js

The front end should be visible at <http://localhost:8000>

## Communication

Since the server will be offering zmq sockets, the webserver will
also port the stream to a websocket connection.


## Frontend Libraries

- D3 (SVG first, then explore `pathgl`)
