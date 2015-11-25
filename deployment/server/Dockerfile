FROM spacerace:base
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>

#Server build
RUN cmake /usr/src/spacerace && make

CMD ['/spacerace/spacerace-server', '-s', '/usr/src/spacerace/spacerace.json']

EXPOSE 5556 5557 5558 5559

