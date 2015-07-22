var zmq = require('zmq');
var sock = zmq.socket('pub');

var NUM_SHIPS = 100;

sock.bindSync('tcp://127.0.0.1:5556');

console.log('Dummy Game State producer bound to port 3000');
console.log("Making up movements for " + NUM_SHIPS + " ships");

var r = function(n){return Math.random() * n};
var data = [];
for (var i = 0; i < NUM_SHIPS; i++) {

    var shipHeading = r(2*Math.PI);    // Radians!
    var shipSpeed = r(0.5);

    data.push(
        {
            x: r(100),
            y: r(100),

            dx: shipSpeed * Math.cos(shipHeading),
            dy: shipSpeed * Math.sin(shipHeading),

            theta: shipHeading,
            omega: 0.0001 * r(Math.PI),

            Tr: 0,
            Tl: i % 2
        }
    );
}


function wrap(position){
    var wrapat = 100;
    if(position <= 0){
        return position + wrapat - 2;
    }
    if(position >= wrapat){
        return position - wrapat + 2;
    }
    return position;
}

setInterval(function(){

    for (var i = 0; i < data.length; i++) {

        data[i].x += data[i].dx;
        data[i].y += data[i].dy;
        data[i].theta += data[i].omega;

        // Wrap the position for now...
        data[i].x = wrap(data[i].x);
        data[i].y = wrap(data[i].y);

    }
    sock.send(JSON.stringify({
        "data": data
    }));

}, 16);