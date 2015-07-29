var io = require("io"); // Note socket.io is added globally in the html page
var d3 = require("d3/d3.min.js");

require("./style.css");

var socket = io();
var requestID;
var gameState;
var lastUpdateTime;

var updates = 0;
var draws = 0;
var fpsqueue = [];

var playerDiv = d3.select("#leaderboard");

socket.on('Log', function (msg) {
    var rawPacket = JSON.parse(msg);
    //console.log(rawPacket);

    if(rawPacket.category === "lobby"){
        console.log("Lobby Status received");
        console.log(rawPacket);

        d3.selectAll("#lobby ul").remove();


        var lobby = d3.selectAll("#lobby")
            .append("ul")
            .selectAll("#lobby li")
            .data(rawPacket.data.players);

        lobby.enter()
            .append("li")
            .text(function(d, i){ return d; })


    }
    else {
        console.log("Got unknown category");
        console.log(rawPacket);
    }
});


socket.on('GameState', function (msg) {
    var rawPacket = JSON.parse(msg);
    //console.log(rawPacket);

    if (rawPacket.state === "running") {
        gameState = rawPacket.data;
        updates += 1;

        if (updates == 1) {
            setupGame()
        }
    }

    if (rawPacket.state === "finished") {
        console.log("Game Over");
    }
});

//socket.on('GameMap', function (msg) {
//    var data = JSON.parse(msg).data;
//    setupGame(data)
//}

var svgContainer = d3.select('#game')
    .attr("width", "100%")
    .attr("height", "500");

var mapContainer = svgContainer.append("g");

var shipG = svgContainer.append("defs")
    .append("symbol")
    .attr("viewBox", "0 0 50 50")
    .attr("id", "ship")
    .attr("class", "ship");

shipG.append("polygon")
    .attr("points", "25,2 47,37 3,37")
    .attr("class", "shipWings");

// For now the jet "backwards" is simply a solid line
shipG.append("line")
    .attr("class", "shipJet")
    .attr("x1", 25)
    .attr("y1", 35)
    .attr("x2", 25)
    .attr("y2", 45);

// Main circular body of the ship
shipG.append("circle")
    .attr("class", "shipBody")
    .attr('cx', 25)
    .attr('cy', 25)
    .attr('r', function (d, i) {
        return 16;
    });


shipG.append("polygon")
    .attr("points", "23,30 27,30 25,5")
    .attr("class", "shipStripe");


var shipGroup = svgContainer.append("g").attr("id", "shipsParentGroup");

var ships;

var x, y;

var width, height;

function loadMap() {
    // TODO Deal with all the maps
    // => DataUrl if the map file is smaller that 1Mb
    var mapData = require("url?limit=1000000!../maps/bt-circle1.png");

    var mapImage = document.createElement('img');
    mapImage.addEventListener('load', function () {
        /*
         * Find out the real size/aspect ratio of the
         * image so we draw our ships correctly
         * */
        width = mapImage.width;
        height = mapImage.height;
        var aspectRatio = width / height;

        console.log("Loaded map image. Size = (%s, %s)", width, height);

        // Set the game's height to match the map's aspect ratio?
        var actualWidth = parseInt(svgContainer.style("width"), 10);
        var actualHeight = actualWidth / aspectRatio;

        svgContainer
            .attr("height", actualHeight);

        // Add an <image> to our <svg>
        // TODO test performance of putting it in a div behind the svg?
        mapContainer.append("image")
            .attr("id", "GameMap")
            .attr("width", actualWidth)
            .attr("height", actualHeight)
            .attr("xlink:href", mapData);

        // Assume positions are between 0 and 100 for now
        // (0,0) is at the bottom left
        x = d3.scale.linear().domain([0, 100]).range([0, actualWidth]);
        y = d3.scale.linear().domain([0, 100]).range([actualHeight, 0]);

    });
    mapImage.src = mapData;
}

loadMap();

var selectedShip;

var fps = d3.select("#fps span");

var setupGame = function () {
    var initState = gameState;
    var SHIPSIZE = "10";


    initState.map(function(d){

        d.color = "hsl(" + Math.random() * 360 + ",75%, 50%)";
    });

    // Show each player
    var playerContainer = playerDiv.append("ul");
    var players = playerContainer.selectAll("li")
        .data(initState);

    players.exit().remove();
    players.enter().append("li").append("button")
        .attr("title", "Click to select")
        .on("click", function(d, i){
            console.log("Selecting ship for player " + d.id);
            selectedShip = i;
        })
        .style("color", function(d, i){
            return d.color;
        })
        .text(function(d, i){
            return d.id;
        });


    ships = shipGroup
        .selectAll('.ship')
        .data(initState)
        .enter()
        .append('use')
        .attr("transform", function (d, i) {
            return "translate(0, 0)";
        })
        .attr("id", function (d, i) {
            return "ship" + i;
        })
        .attr("xlink:href", "#ship")
        .attr("width", SHIPSIZE)
        .attr("height", SHIPSIZE)
        .attr('fill', function (d) {
            return d.color;
        });

    // Trigger the first full draw
    updateState();

};

var updateState = function (highResTimestamp) {

    requestID = requestAnimationFrame(updateState);

    if (updates >= draws) {
        // Only update the ships if we have gotten an update from the server
        draws += 1;

        // Calculate an fps counter
        if (fpsqueue.length >= 100) {
            fps.text(d3.mean(fpsqueue).toFixed(0));
            fpsqueue = fpsqueue.slice(1, 100);
        }
        fpsqueue.push(Math.round(1000 / (highResTimestamp - lastUpdateTime)));
        lastUpdateTime = highResTimestamp;


        ships
            .data(gameState)

            .style("fill", function(d, i){
                if(selectedShip === i){
                    return "black";
                }
            })

            // Perhaps we can add/remove the jet with display="none"
            //.attr('opacity', function (d, i) {
            //    // Show accelerating with opacity toggle
            //    var accelerating = d.Tl == 1;
            //    return accelerating ? 1.0 : 0.2;
            //})
            .attr("x", function (d, i) {
                return x(d.x);
            })
            .attr("y", function (d, i) {
                return y(d.y);
            })
            .attr("transform", function (d, i) {
                // It is possible to both update ships' position and rotation using a single SVG transform/rotation
                // command however it doesn't seem as performant as using the x,y attributes as above.
                // Note SVG rotate takes degrees not radians, and it also takes positon (X, Y)
                // to center the rotation around.
                var shipX = x(d.x),
                    shipY = y(d.y);

                return "rotate(" +
                    (90 - d.theta * 360 / (2 * Math.PI)) +
                    "," + shipX + ", " + shipY +
                    ")";
            });
    }
};



