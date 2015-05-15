# Based on https://github.com/AsavarTzeth/docker-php5-fpm
FROM ptp-php:latest

MAINTAINER Jamie McClelland <jamie@progressivetech.org>

# Install packages only needed by powerbase.
RUN apt-get update && \
    export DEBIAN_FRONTEND=noninteractive && \ 
    apt-get install -y \
    --no-install-recommends \
    php5-curl \
    php5-gd \
    liboauth-php \
    fonts-linuxlibertine \
    wkhtmltopdf \
    drush \
    wget \
    mysql-client \
    esmtp-run \
    vim-tiny \
    xvfb && \
    rm -rf /var/lib/apt/lists/*

# FIXME - this address seems to get redirected to http
RUN wget https://downloads.sourceforge.net/project/wkhtmltopdf/0.12.2.1/wkhtmltox-0.12.2.1_linux-jessie-amd64.deb -O /tmp/wkhtmltox-0.12.2.1_linux-jessie-amd64.deb

# Install over the package we installed via apt. We install via apt so
# we are sure to have the dependencies.
RUN dpkg -i /tmp/wkhtmltox-0.12.2.1_linux-jessie-amd64.deb 

# User arbitrary high UID so we can more easily understand UIDs on the host
RUN groupmod -g 5001 www-data
RUN usermod -u 5001 www-data 
RUN usermod -s /bin/bash www-data

# Create drush directory under the www-data's home directory so the create
# command can mount our drush scripts.
RUN mkdir -p /var/www/.drush

# Create the powerbase directory to mount the platform.
RUN mkdir /var/www/powerbase

# Copy our custom PHP settings for powerbase.
COPY powerbase.ini /etc/php5/conf.d/powerbase.ini

# Copy over our esmtprc file to configure handling email.
COPY esmtprc /etc/esmtprc
