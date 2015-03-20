# Based on https://github.com/AsavarTzeth/docker-php5-fpm
FROM ptp-base:latest

MAINTAINER Jamie McClelland <jamie@progressivetech.org>

RUN apt-get update && \
    apt-get install -y \
    --no-install-recommends \
    php5-fpm \
    php5-curl \
    php5-gd \
    php5-mysql \
    php5-mcrypt \
    php-apc \
    liboauth-php \
    fonts-linuxlibertine \
    wkhtmltopdf \
    xvfb && \
    rm -rf /var/lib/apt/lists/*

# RUN wget https://downloads.sourceforge.net/project/wkhtmltopdf/0.12.2.1/wkhtmltox-0.12.2.1_linux-jessie-amd64.deb
# User arbitrary high UID so we can more easily understand UIDs on the host
RUN groupmod -g 5001 www-data
RUN usermod -u 5001 www-data 

# Listen on 127.0.0.1 
RUN sed -i 's#listen = /var/run/php5-fpm.sock#listen = 9000#' /etc/php5/fpm/pool.d/www.conf

COPY ./docker-entrypoint.sh /entrypoint.sh
COPY powerbase.ini /etc/php5/conf.d/powerbase.ini

EXPOSE 9000

ENTRYPOINT ["/entrypoint.sh"]
CMD ["php5-fpm"]
