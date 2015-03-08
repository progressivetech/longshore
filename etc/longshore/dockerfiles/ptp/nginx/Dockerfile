FROM debian:jessie

MAINTAINER Jamie McClelland <jamie@progressivetech.org>

RUN apt-get update && \
    apt-get install -y \
    --no-install-recommends \ 
    ca-certificates nginx && \
    rm -rf /var/lib/apt/lists/*

# Use consistent UIDs so we can more easily understand ownership from the host.
RUN groupmod  -g 5002 www-data 
RUN usermod -u 5002 www-data 

# forward request and error logs to docker log collector
RUN ln -sf /dev/stdout /var/log/nginx/access.log
RUN ln -sf /dev/stderr /var/log/nginx/error.log

# Used to hold our x509 certs and keys
RUN mkdir /etc/nginx/x509

# Used to hold the drupal sites directories
RUN mkdir /srv/sites

COPY docker-entrypoint.sh /entrypoint.sh

EXPOSE 80 443
ENTRYPOINT ["/entrypoint.sh"]
CMD ["nginx"]
