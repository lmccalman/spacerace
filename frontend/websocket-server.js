var zmq = require('zmq');
var express = require('express');
var app = express();
var http = require('http').Server(app);

var io = require('socket.io')(http);

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
    //console.log('Received: %s', msg.toString());
    io.emit('GameState', msg.toString());
});

console.log('Frontend server is trying to connect to ZMQ pub socket on port 5556');
subscriber.connect('tcp://127.0.0.1:5556');