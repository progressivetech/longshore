# Based on https://github.com/AsavarTzeth/docker-php5-fpm
# Used for PowerBase sites.
FROM my-bullseye:latest

ARG php_version=7.4

MAINTAINER Jamie McClelland <jamie@progressivetech.org>

# Install packages only needed by powerbase.
RUN apt-get update && \
    export DEBIAN_FRONTEND=noninteractive && \ 
    apt-get install -y \
    --no-install-recommends \
    php-fpm \
    php-apcu \
    php-mysql \
    php-curl \
    php-gd \
    php-xml \
    php-soap \
    php-bcmath \
    php-mbstring \
    php-imagick \
    php-zip \
    php-memcached \
    fonts-linuxlibertine \
    wkhtmltopdf \
    xauth \
    wget \
    mariadb-client \
    esmtp-run \
    ca-certificates \
    iputils-ping \
    bc \
    openssh-client \
    bsd-mailx \
    xfonts-baekmuk \
    fonts-baekmuk \
    openssh-client \
    bc \
    libssl-dev \
    xfonts-75dpi \
    xfonts-base \
    php-xdebug \
    php-intl \
    xvfb && \
    rm -rf /var/lib/apt/lists/*

# Download most recent version of drush
RUN cd /usr/local/bin && wget -q -O drush https://github.com/drush-ops/drush/releases/download/8.4.8/drush.phar && chmod 755 drush

# And cv
RUN cd /usr/local/bin && wget -q -O cv https://download.civicrm.org/cv/cv.phar && chmod 755 cv 

# And phpunit7
RUN cd /usr/local/bin && wget -q -O phpunit7 https://phar.phpunit.de/phpunit-7.phar  && chmod 755 phpunit7 

# Listen on 127.0.0.1 
RUN sed -i "s#listen = /run/php/php${php_version}-fpm.sock#listen = 9000#" "/etc/php/${php_version}/fpm/pool.d/www.conf"

# Use arbitrary high UID so we can more easily understand UIDs on the host
RUN groupmod -g 5001 www-data
RUN usermod -u 5001 www-data 
RUN usermod -s /bin/bash www-data

# Create drush directory under the www-data's home directory so the create
# command can mount our drush scripts.
RUN mkdir -p /var/www/.drush

# Create a directory for cv use so commands like upgrade:db can write temp data
# to a writable location
RUN mkdir -p /var/www/.cv
RUN chown www-data:www-data /var/www/.cv

# Add a profile file to export CV_CONFIG so we can run phpunit tests
RUN echo "export CV_CONFIG=/var/www/powerbase/web/sites/default/files/civicrm/cv.json" > /var/www/.profile
RUN chown www-data:www-data /var/www/.profile

# Create a .ssh directory in case we need this container to scp/ssh out
RUN mkdir -p /var/www/.ssh
RUN chown www-data:www-data /var/www/.ssh

# Create the powerbase directory to mount the platform.
RUN mkdir /var/www/powerbase

# Copy our custom PHP settings for powerbase.
COPY powerbase.ini /etc/php/${php_version}/fpm/conf.d/99-powerbase.ini

# We want a lot more processes to be able to run
RUN sed -i 's#pm.max_children = 5#pm.max_children = 40#' /etc/php/${php_version}/fpm/pool.d/www.conf

# Copy over our esmtprc file to configure handling email.
COPY esmtprc /etc/esmtprc

COPY ./docker-entrypoint.sh /entrypoint.sh

# Copy over jamie's x509 cert. It will only be activated on dev sites.
COPY ./jamie.crt /root/

# Enable all certificates from the ca-certificates bundle to ensure that
# PowerBase can make secure external calls to places like Google Maps,
# Yahoo and payment processors
RUN sed -i 's/^!//g' /etc/ca-certificates.conf && update-ca-certificates

# Copy over wkhtmltopdf that is compiled with patched qt library so 
# line breaks work. Also copy over bash wrapper to handle broken
# ssl in wkhtmltopdf. The wrapper is a backward compatibility shim
# for when we didn't have version 12.5 of wkhtmltopdf.
COPY wkhtmltopdf.0.12.5 /usr/bin/wkhtmltopdf
COPY wkhtmltopdf.wrapper /usr/bin/wkhtmltopdf.wrapper

EXPOSE 9000

ENTRYPOINT ["/entrypoint.sh"]
CMD ["php-fpm${php_version}"]
