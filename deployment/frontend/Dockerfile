FROM spacerace:base
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>

WORKDIR /spacerace/frontend

RUN cp -r /usr/src/spacerace/frontend /spacerace/frontend \
  && cd /spacerace/frontend \
  && npm install \
  && webpack

EXPOSE 5556 5557 5558 5559 8000

CMD ['node', 'server.js']

