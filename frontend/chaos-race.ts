/*
* A dumb bot for testing the server and front end.
* Launch with:
*   node chaos-race [number of ships] [server if not localhost]
* */

var zmq = require('zmq');

var NUM_SHIPS = parseInt(process.argv.length > 2 ? process.argv[2] : '10');
var server = process.argv.length > 3 ? process.argv[3] : 'localhost';

var state_port = 5556;
var control_port = 5557;
var lobby_port = 5558;

// Share one state socket
var latestState;
var gameNotOver = true;
var stateSocket = zmq.socket('sub');
stateSocket.subscribe('');
stateSocket.on('message', function(topic, msg) {
    console.log('[GAME] Received State for: %s', topic.toString());
    latestState = JSON.parse(msg.toString());
    if (latestState.status === "GAME OVER"){
        gameNotOver = false;
    }
});

var connect = (id: string) => {
    var lobbySocket = zmq.socket('req');
    console.log("Connecting to lobby");
    lobbySocket.connect('tcp://' + server + ':' + lobby_port);
    console.log("Sending oohhoo aaahha");
    lobbySocket.send(id);
    lobbySocket.on('messgae', (msg) => {
        var data = JSON.parse(msg);
        console.log("Monkey %s received: ", id);
        console.log(data);

        playGame(data, monkeyID);

    })
};

var playGame = (gameData, id: string) => {
    console.log(id + " is starting to reek havoc");

    var controlSocket = zmq.socket('push');
    controlSocket.connect('tcp://' + server + ':' + control_port);
    // These chaos monkeys don't even need to watch the state socket

    var step = () => {
        var f = 1,
            r = 0;

        controlSocket.send(gameData.secret + "," + f + "," + r);

        if (gameNotOver) {
            setInterval(step, 16);
        }
    };
    step()
};

console.log("Making up movements for " + NUM_SHIPS + " ships");

var r = (n) => {return Math.random() * n};

for (var i = 0; i < NUM_SHIPS; i++) {
    var monkeyID = "chaos-" + i;
    connect(monkeyID);


}

