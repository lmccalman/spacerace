FROM ubuntu:15.04
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>
RUN apt-get update && apt-get install -y \
  g++ \
  make \
  cmake \
  libboost-all-dev \
  libboost-program-options1.55.0 \
  libboost-system1.55.0 \
  libboost-filesystem1.55.0 \
  libzmq3 \
  libzmq3-dev \
  && mkdir -p /usr/src/spacerace/server /spacerace/config /spacerace/maps

COPY . /usr/src/spacerace/server

#Server build
WORKDIR /spacerace
RUN cmake /usr/src/spacerace/server && make

# Clean up APT when done
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

EXPOSE 5556 5557 5558 5559

CMD ["/spacerace/spacerace-server", "-s", "/spacerace/config/spacerace.json"]

