var zmq = require('zmq');
var sock = zmq.socket('pub');

sock.bindSync('tcp://127.0.0.1:3000');
sock.bindSync("ipc://spacerace-gamestate.ipc");

console.log('Dummy Game State producer bound to port 3000');

setInterval(function(){
    console.log('sending dummy state');
    var num = Math.abs(Math.round(Math.random() * 100));
    sock.send(JSON.stringify({
        "data": [
            {position: [num, num], velocity: [num, num], orientation: num}
        ]
    }));
}, 500);