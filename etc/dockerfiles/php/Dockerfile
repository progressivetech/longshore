# Based on https://github.com/AsavarTzeth/docker-php5-fpm
# Minimal PHP server used for phpmyadmin
FROM my-buster:latest

MAINTAINER Jamie McClelland <jamie@progressivetech.org>

# Install packages needed by both phpmyadmin and powerbase.
RUN apt-get update && \
    apt-get install -y \
    --no-install-recommends \
    php-fpm \
    php-apcu \
    php-mbstring \
    php-mysql && \
    rm -rf /var/lib/apt/lists/*

# Listen on 127.0.0.1 
RUN sed -i 's#listen = /run/php/php7.3-fpm.sock#listen = 9000#' /etc/php/7.3/fpm/pool.d/www.conf

COPY ./docker-entrypoint.sh /entrypoint.sh

EXPOSE 9000

ENTRYPOINT ["/entrypoint.sh"]
CMD ["php5-fpm"]
