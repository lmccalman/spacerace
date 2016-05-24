FROM ubuntu:15.04
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  libzmq3 \
  libzmq3-dev \
  pkg-config \
  && mkdir -p /spacerace/frontend

RUN ldconfig && apt-get install -y nodejs npm && \
    ln -s /usr/bin/nodejs /usr/bin/node && \
    npm install -g webpack

COPY . /spacerace/frontend

# Front end build
WORKDIR /spacerace/frontend
RUN npm install && webpack

# Clean up APT when done
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

EXPOSE 8000

CMD sleep 10s && node server.js server
