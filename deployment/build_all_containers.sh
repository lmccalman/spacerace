#!/bin/bash

docker build -t spacerace:base ..

docker build -t spacerace:server server
docker build -t spacerace:nginx nginx
docker build -t spacerace:httpserver httpserver 
docker build -t spacerace:frontend frontend
