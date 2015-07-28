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
console.log('Frontend is trying to connect to ' + server);

var lobbySocket = zmq.socket('req');
var logSocket = zmq.socket('sub');
var gameStateSocket = zmq.socket('sub');


//lobbySocket.subscribe("");
logSocket.subscribe("");

gameStateSocket.subscribe("");

// On receiving a message from the game server
// Note the `msg` is a string or buffer, not JSON
lobbySocket.connect('tcp://' + server + ':5558');

lobbySocket.on('message', function(msg){
    console.log('[LOBBY] Received: %s', msg.toString());

    // Assume game start message?
    var data = JSON.parse(msg);

    var gameName = data.game;
    var gameMap = data.map;

    console.log("[LOBBY] Next game " + gameName + " will be on " + gameMap);

});


logSocket.on('message', function(msg){
    // Note the `msg` is a string, not JSON
    console.log('[LOG] Received: %s', msg.toString());
});

gameStateSocket.on('message', function(topic, msg){
    console.log('[GAME] Received State for: %s', topic.toString());
    //console.log('[GAME] State: %s', msg.toString());

    io.emit('GameState', msg.toString());
});


gameStateSocket.connect('tcp://' + server + ':5556');
logSocket.connect('tcp://' + server + ':5559');
