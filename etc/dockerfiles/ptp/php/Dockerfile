# Based on https://github.com/AsavarTzeth/docker-php5-fpm
FROM my-jessie:latest

MAINTAINER Jamie McClelland <jamie@progressivetech.org>

# Install packages needed by both phpmyadmin and powerbase.
RUN apt-get update && \
    apt-get install -y \
    --no-install-recommends \
    php5-fpm \
    php-apc \
    php5-mcrypt \
    php5-mysql && \
    rm -rf /var/lib/apt/lists/*

# Listen on 127.0.0.1 
RUN sed -i 's#listen = /var/run/php5-fpm.sock#listen = 9000#' /etc/php5/fpm/pool.d/www.conf

COPY ./docker-entrypoint.sh /entrypoint.sh

EXPOSE 9000

ENTRYPOINT ["/entrypoint.sh"]
CMD ["php5-fpm"]
