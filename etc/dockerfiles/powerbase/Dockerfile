# Based on https://github.com/AsavarTzeth/docker-php5-fpm
# Used for PowerBase sites.
FROM my-bookworm:latest

ARG php_version=8.1

MAINTAINER Jamie McClelland <jamie@progressivetech.org>
RUN apt-get update && \
    export DEBIAN_FRONTEND=noninteractive && \ 
    apt-get install -y \
    lsb-release \
    ca-certificates \
    curl

RUN curl -sSLo /usr/share/keyrings/deb.sury.org-php.gpg https://packages.sury.org/apt.gpg
RUN sh -c 'echo "deb [signed-by=/usr/share/keyrings/deb.sury.org-php.gpg] https://packages.sury.org/php/ $(lsb_release -sc) main" > /etc/apt/sources.list.d/php.list'

# Install packages only needed by powerbase.
RUN apt-get update && \
    export DEBIAN_FRONTEND=noninteractive && \ 
    apt-get install -y \
    --no-install-recommends \
    php8.1-fpm \
    php8.1-apcu \
    php8.1-mysql \
    php8.1-curl \
    php8.1-gd \
    php8.1-xml \
    php8.1-soap \
    php8.1-bcmath \
    php8.1-mbstring \
    php8.1-imagick \
    php8.1-zip \
    php8.1-memcached \
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
    php8.1-xdebug \
    php8.1-intl \
    xvfb && \
    rm -rf /var/lib/apt/lists/*

# Download most recent version of drush8 for drupal7 sites.
RUN cd /usr/local/bin && wget -q -O drush https://github.com/drush-ops/drush/releases/download/8.4.12/drush.phar && chmod 755 drush

# For Drupal 9+ sites a more modern drush will be provided via composer. 

# And cv
RUN cd /usr/local/bin && wget -q -O cv https://download.civicrm.org/cv/cv.phar && chmod 755 cv 

# And phpunit8
RUN cd /usr/local/bin && wget -q -O phpunit8 https://phar.phpunit.de/phpunit-8.phar  && chmod 755 phpunit8 

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

# Copy cv.json - also only used on dev sites
COPY ./cv.json /var/www/.cv.json
RUN chown www-data:www-data /var/www/.cv.json

# Enable all certificates from the ca-certificates bundle to ensure that
# PowerBase can make secure external calls to places like Google Maps,
# Yahoo and payment processors
RUN sed -i 's/^!//g' /etc/ca-certificates.conf && update-ca-certificates

EXPOSE 9000

ENTRYPOINT ["/entrypoint.sh"]
CMD ["php-fpm${php_version}"]
