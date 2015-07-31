var io = require("io"); // Note socket.io is added globally in the html page
var d3 = require("d3/d3.min.js");

require("./style.css");

var spaceraceSettings = require("json!./../spacerace.json");
console.log("Welcome to spacerace");

console.log("Global Settings");
console.log(spaceraceSettings);

var pixelSize = spaceraceSettings.simulation.world.pixelSize;

var socket = io();
var requestID;
var gameState;
var lastUpdateTime;
var nextMap = false;

var updates = 0;
var draws = 0;
var fpsqueue = [];

var playerDiv = d3.select("#leaderboard");

function checkStarted(data){
    if(nextMap == false){
        // First time, load map
        nextMap = data.map;
        loadMap();
    }
    nextMap = data.map;
}

socket.on('Log', function (msg) {
    var rawPacket = JSON.parse(msg);
    //console.log(rawPacket);

    if(rawPacket.category === "lobby"){
        console.log("Lobby Message received");
        console.log(rawPacket);

        if (rawPacket.subject === "status"){
            checkStarted(rawPacket.data);
            nextMap = rawPacket.data.map;

            d3.selectAll("#lobby ul").remove();

            var lobby = d3.selectAll("#lobby")
                .append("ul")
                .selectAll("#lobby li")
                .data(rawPacket.data.players);

            lobby.enter()
                .append("li")
                .text(function(d, i){ return d; });
        }

        return;
    }

    if (rawPacket.category === "game") {
        // General Game updates
        if (rawPacket.subject === "status"){
            var d = rawPacket.data;
            console.log(d.game + ' on ' + d.map + ' is ' + d.state);

            if (d.state === 'finished'){
                console.log("Stopping animation");
                cancelAnimationFrame(requestID);
            }
        } else {
            console.log("unknown subject");
        }

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
            loadMap(setupGame);

        }
    }

    if (rawPacket.state === "finished") {
        console.log("Game Over");

        // Load the next map (assuming it has changed)
        loadMap(setupGame);
    }
});


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

var mapWidth, mapHeight;

function loadMap(cb) {
    if(!nextMap){
        console.log("Not loading map as we don't know what map to load");
        return;
    }

    // Deal with all the maps
    // => DataUrl if the map file is smaller that 1Mb
    var mapData = require("url?limit=1000000!../maps/" + nextMap + ".png");

    var mapImage = document.createElement('img');
    mapImage.addEventListener('load', function () {
        /*
         * Find out the real size/aspect ratio of the
         * image so we draw our ships correctly
         * */
        mapWidth = mapImage.width;
        mapHeight = mapImage.height;
        var aspectRatio = mapWidth / mapHeight;

        console.log("Loaded map image. Size = (%s, %s)", mapWidth, mapHeight);

        // Set the game's height to match the map's aspect ratio?
        var displayWidth = parseInt(svgContainer.style("width"), 10);
        var displayHeight = displayWidth / aspectRatio;

        console.log("Display size = (%s, %s)", displayWidth, displayHeight);

        svgContainer
            .attr("height", displayHeight)
            .attr('width', displayWidth);

        // Add an <image> to our <svg>
        // TODO test performance of putting it in a div behind the svg?
        mapContainer.selectAll("image").remove();
        mapContainer.append("image")
            .attr("id", "GameMap")
            .attr("width", displayWidth)
            .attr("height", displayHeight)
            .attr("xlink:href", mapData);


        /* Set up mappings between image pixels, game units, and display pixels
         * pixelSize is approx 10.
         * If a map is 1500px wide, the physics engine will give positions between [0,150]
         * (0,0) is at the bottom left
         * */

        x = d3.scale.linear().domain([0, mapWidth / pixelSize]).range([0, displayWidth]);
        y = d3.scale.linear().domain([0, mapHeight / pixelSize]).range([displayHeight, 0]);

        if(cb){
            cb();
        }
    });

    mapImage.src = mapData;
}

var selectedShip;

var fps = d3.select("#fps span");

// Runs once per game
var setupGame = function () {

    if(!nextMap){
        console.log("Not setting up game as we don't know what map to load");
        return;
    }

    var initState = gameState;

    // Note: The ship is 2 * pixelSize wide in game units (radius of the ship = 1 map scale)
    // Ship size in display pixels, Divide by 50 for the def file.
    console.log("Pixel size is ", pixelSize);
    var SHIPSIZE = ( x(2 * pixelSize / 50) ).toString();

    console.log("Ship size will be " + SHIPSIZE);

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
        .data(initState);

    ships.exit().remove();
    ships.enter()
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
        console.log(gameState);
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



