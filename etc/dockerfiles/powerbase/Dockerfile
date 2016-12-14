# Based on https://github.com/AsavarTzeth/docker-php5-fpm
# Used for PowerBase sites.
FROM my-jessie:latest

MAINTAINER Jamie McClelland <jamie@progressivetech.org>

# Install packages only needed by powerbase.
RUN apt-get update && \
    export DEBIAN_FRONTEND=noninteractive && \ 
    apt-get install -y \
    --no-install-recommends \
    php5-fpm \
    php-apc \
    php5-mcrypt \
    php5-mysql \
    php5-curl \
    php5-gd \
    php5-imagick \
    liboauth-php \
    fonts-linuxlibertine \
    wkhtmltopdf \
    xauth \
    wget \
    mysql-client \
    esmtp-run \
    ca-certificates \
    iputils-ping \
    bc \
    openssh-client \
    heirloom-mailx \
    xfonts-baekmuk \
    ttf-baekmuk \
    openssh-client \
    bc \
    xvfb && \
    rm -rf /var/lib/apt/lists/*

# Download most recent version of drush
RUN cd /usr/local/bin && wget -q -O drush https://s3.amazonaws.com/files.drush.org/drush.phar && chmod 755 drush

# Listen on 127.0.0.1 
RUN sed -i 's#listen = /var/run/php5-fpm.sock#listen = 9000#' /etc/php5/fpm/pool.d/www.conf

# User arbitrary high UID so we can more easily understand UIDs on the host
RUN groupmod -g 5001 www-data
RUN usermod -u 5001 www-data 
RUN usermod -s /bin/bash www-data

# Create drush directory under the www-data's home directory so the create
# command can mount our drush scripts.
RUN mkdir -p /var/www/.drush

# Create a .ssh directory in case we need this container to scp/ssh out
RUN mkdir -p /var/www/.ssh
RUN chown www-data:www-data /var/www/.ssh

# Create the powerbase directory to mount the platform.
RUN mkdir /var/www/powerbase

# Copy our custom PHP settings for powerbase.
COPY powerbase.ini /etc/php5/fpm/conf.d/99-powerbase.ini

# We want a lot more processes to be able to run
RUN sed -i 's#pm.max_children = 5#pm.max_children = 40#' /etc/php5/fpm/pool.d/www.conf

# Copy over our esmtprc file to configure handling email.
COPY esmtprc /etc/esmtprc

COPY ./docker-entrypoint.sh /entrypoint.sh

# Copy over jamie's x509 cert. It will only be activated on dev sites.
COPY ./jamie.crt /root/

# Enable all certificates from the ca-certificates bundle to ensure that
# PowerBase can make secure external calls to places like Google Maps,
# Yahoo and payment processors
RUN sed -i 's/^!//g' /etc/ca-certificates.conf && update-ca-certificates

EXPOSE 9000

ENTRYPOINT ["/entrypoint.sh"]
CMD ["php5-fpm"]
