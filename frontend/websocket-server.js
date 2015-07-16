var zmq = require('zmq');

var subscriber = zmq.socket('sub');

var filter = "";
subscriber.subscribe(filter);


subscriber.on('message', function(msg){
    console.log('Received: %s', msg.toString());
});

console.log('Websocket server trying to connect to ZMQ pub socket on port 3000');
subscriber.connect('tcp://127.0.0.1:3000');