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

# Download Drupal and confirm checksum (this command will fail, causing build
# to fail if it fails).
RUN version=7.34 checksum=bb4d212e1eb1d7375e41613fbefa04f2 && cd /var/www/html && \
  wget --quiet http://ftp.drupal.org/files/projects/drupal-${version}.tar.gz && \
  echo "$checksum drupal-${version}.tar.gz" | md5sum -c && \
  tar -xzf "drupal-${version}.tar.gz" && \
  mv "drupal-${version}" d7
  rm -f "/var/www/html/drupal-${version}.tar.gz"

# Error logs to docker log collector. We will continue
# writing access logs to the filesystem (mounted on the host)
RUN ln -sf /dev/stderr /var/log/nginx/error.log

# Used to hold our x509 certs and keys
RUN mkdir /etc/nginx/x509

# Used to hold the drupal sites directories
RUN mkdir /srv/sites

COPY docker-entrypoint.sh /entrypoint.sh

EXPOSE 80 443
ENTRYPOINT ["/entrypoint.sh"]
CMD ["nginx"]
