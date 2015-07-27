var zmq = require('zmq');
var express = require('express');
var app = express();
var http = require('http').Server(app);
var io = require('socket.io')(http);


// Game Server Address
var server = process.argv.length > 2 ? process.argv[2] : 'localhost';
console.log("Game Server:" + server);

// Webserver component
app.get('/', function(req, res){
  res.sendFile(__dirname + '/index.html');
});
// TODO use gulp/webpack etc
app.use(express.static('node_modules'));
http.listen(8000, function(){
  console.log('listening on *:8000');
});


// Websocket component
io.on('connection', function(ws){
    console.log('A websocket connected');

    ws.on('debug', function(msg){
        console.log('WWW Debug: ' + msg);
    });

    ws.on('disconnect', function(){
        console.log('Websocket disconnected');
    });

});

// ZMQ component
var subscriber = zmq.socket('sub');

var filter = "";
subscriber.subscribe(filter);

subscriber.on('message', function(msg){
    // Note the `msg` is a string, not JSON
    //console.log('Received: %s', msg.toString());
    io.emit('GameState', msg.toString());
});

console.log('Frontend server is trying to connect to ' + server +' ZMQ pub socket on port 5556');
subscriber.connect('tcp://' + server + ':5556');