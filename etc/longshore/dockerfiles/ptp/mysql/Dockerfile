# This is a comment
FROM ptp-base:latest
MAINTAINER Jamie McClelland <jamie@progressivetech.org>
RUN apt-get update && \
  export DEBIAN_FRONTEND=noninteractive && \
  apt-get install --no-install-recommends -y \
  mysql-client \
  mysql-server && \
  rm -rf /var/lib/apt/lists/*

# Ensure that our databases survive restarts.
VOLUME /var/lib/mysql

# Ensure networking will work.
RUN sed -i "s/^bind-address/#bind-address/g" /etc/mysql/my.cnf

# Avoid key buffer size warnings and myisam-recover warnings
RUN sed -i "s/^key_buffer\t/key_buffer_size\t/g" /etc/mysql/my.cnf
RUN sed -i "s/^myisam-recover\t/myisam-recover-options\t/g" /etc/mysql/my.cnf

COPY optimize-ptp-civicrm.cnf /etc/mysql/conf.d/
COPY docker-entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
EXPOSE 3306
CMD ["mysqld"]

