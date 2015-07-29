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
  
RUN mkdir -p /usr/src/spacerace /spacerace
COPY . /usr/src/spacerace
WORKDIR /spacerace
RUN cmake /usr/src/spacerace && make

# Clean up APT when done
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
EXPOSE 5556 5557 5558 5559
CMD ["./spacerace-server", "-s", "/usr/src/spacerace/spacerace.json"]

