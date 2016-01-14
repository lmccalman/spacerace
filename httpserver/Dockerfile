FROM ubuntu:15.10
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>

RUN apt-get update && apt-get install -y \
  python3 \
  libc6 \
  libzmq3 \
  libzmq-dev \
  python3-pip

RUN pip3 install flask flask-cors gunicorn zmq 

RUN mkdir -p /spacerace/httpserver

COPY . /spacerace/httpserver

WORKDIR /spacerace/httpserver

# Clean up APT when done
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

EXPOSE 5001

CMD ["gunicorn", "--workers", "5", "--bind", "0.0.0.0:5001", "--access-logfile", "-", "app:app"]
