FROM spacerace:base
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>

COPY uwsgi.ini /root/

WORKDIR /root

EXPOSE 8080

# The uwsgi 
CMD ["uwsgi", "uwsgi.ini"]
