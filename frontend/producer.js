var zmq = require('zmq');
var sock = zmq.socket('pub');

var NUM_SHIPS = 50;

sock.bindSync('tcp://127.0.0.1:5556');

console.log('Dummy Game State producer bound to port 3000');
console.log("Making up movements for " + NUM_SHIPS + " ships");

var r = function(n){return Math.random() * n};
var data = [];
for (var i = 0; i < NUM_SHIPS; i++) {
    data.push(
        {
            position: [r(100), r(100)],
            velocity: [r(1), r(1)],

            theta: r(2*Math.PI),
            omega: r(2*Math.PI/60) - Math.PI/60
        }
    );
}


function wrap(position){
    var wrapat = 25;
    if(position < 0){
        return position + wrapat;
    }
    if(position > wrapat){
        return position - wrapat;
    }
    return position;
}

setInterval(function(){

    for (var i = 0; i < data.length; i++) {
        var v = data[i].velocity;
        var theta = data[i].theta;

        // Update the current heading
        data[i].theta += data[i].omega;

        data[i].position[0] += v[0] * Math.sin(theta);
        data[i].position[1] += v[1] * Math.cos(theta);

        // Wrap the position for now...
        data[i].position[0] = wrap(data[i].position[0]);
        data[i].position[1] = wrap(data[i].position[1]);

    }
    sock.send(JSON.stringify({
        "data": data
    }));

}, 20);