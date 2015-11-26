FROM ubuntu:15.04
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>
RUN apt-get update && apt-get install -y \
  uwsgi \
  uwsgi-plugin-python3 \
  python3-flask \
  python3-zmq \
  && mkdir -p /spacerace/httpserver

COPY . /spacerace/httpserver

WORKDIR /spacerace/httpserver

# Clean up APT when done
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

EXPOSE 8080

CMD ["uwsgi", "uwsgi.ini"]
