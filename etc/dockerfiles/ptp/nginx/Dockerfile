FROM ptp-base:latest

MAINTAINER Jamie McClelland <jamie@progressivetech.org>

RUN apt-get update && \
    apt-get install -y \
    --no-install-recommends \ 
    ca-certificates \
    nginx \
    wget && \
    rm -rf /var/lib/apt/lists/*

# Use consistent UIDs so we can more easily understand ownership from the host.
RUN groupmod  -g 5000 www-data 
RUN usermod -u 5000 www-data 

# Create directories for each of our platforms 
RUN mkdir /var/www/html/ourpowerbase-d7-c4.5 && mkdir /var/www/html/ourpowerbase-d7-c4.4 

# Error logs to docker log collector. We will continue
# writing access logs to the filesystem (mounted on the host)
#RUN sed -i "s#/var/log/nginx/error.log#/dev/stderr#" /etc/nginx/nginx.conf && rm /var/log/nginx/error.log

# Used to hold our x509 certs and keys
RUN mkdir /etc/nginx/x509

# Used to hold the drupal sites directories
RUN mkdir /srv/sites

COPY docker-entrypoint.sh /entrypoint.sh

EXPOSE 80 443
ENTRYPOINT ["/entrypoint.sh"]
CMD ["nginx"]
