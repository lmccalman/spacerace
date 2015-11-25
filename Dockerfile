FROM ubuntu:15.04
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  libboost-all-dev \
  libboost-program-options1.55.0\
  libboost-system1.55.0\
  libboost-filesystem1.55.0\
  libzmq3\
  libzmq3-dev

RUN apt-get install -y nodejs npm && \
    ln -s /usr/bin/nodejs /usr/bin/node && \
    npm install -g webpack

RUN mkdir -p /usr/src/spacerace /spacerace
COPY . /usr/src/spacerace

WORKDIR /spacerace
RUN cp -r /usr/src/spacerace/maps /spacerace/maps
RUN cmake /usr/src/spacerace && make

# Front end
RUN cp -r /usr/src/spacerace/frontend /spacerace/frontend
WORKDIR /spacerace/frontend
RUN npm install
RUN webpack


# Clean up APT when done
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

WORKDIR /spacerace
EXPOSE 5556 5557 5558 5559 8000
# CMD /bin/bash -c '/spacerace/spacerace-server -s /usr/src/spacerace/spacerace.json & cd /spacerace/frontend && node server.js'

