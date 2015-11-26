FROM ubuntu:15.10
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>
RUN apt-get update && apt-get install -y \
  gunicorn3 \
  python3-flask \
  python3-zmq \
  && mkdir -p /spacerace/httpserver

COPY . /spacerace/httpserver

WORKDIR /spacerace/httpserver

# Clean up APT when done
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

EXPOSE 5001

CMD ["gunicorn3", "—workers", "5", "—bind", "0.0.0.0:5001", "app"]
