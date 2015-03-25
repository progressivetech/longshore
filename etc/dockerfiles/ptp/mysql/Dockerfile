FROM ptp-base:latest
MAINTAINER Jamie McClelland <jamie@progressivetech.org>
RUN apt-get update && \
  export DEBIAN_FRONTEND=noninteractive && \
  apt-get install --no-install-recommends -y \
  mysql-server && \
  rm -rf /var/lib/apt/lists/*

# Shutdown mysql so we can manipulate it's user and
# data directories.
RUN /etc/init.d/mysql stop 

# Remove the default databases that are created becuase
# we will be mounting a new data directory over this one
# when we start.
RUN rm -rf /var/lib/mysql/*

# Make the mysql user the same UID as the longshore user
# on the host so we can delete databases from the host
# without needing root.
RUN usermod -u 5000 mysql 
RUN groupmod -g 5000 mysql 

RUN chown -R mysql:mysql /var/run/mysqld

# Ensure networking will work.
RUN sed -i "s/^bind-address/#bind-address/g" /etc/mysql/my.cnf

# Avoid key buffer size warnings and myisam-recover warnings
# See: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=751840
RUN sed -i "s/^key_buffer\s/key_buffer_size\t/g" /etc/mysql/my.cnf
RUN sed -i "s/^myisam-recover\s/myisam-recover-options\t/g" /etc/mysql/my.cnf

COPY ptp.cnf /etc/mysql/conf.d/
COPY docker-entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
EXPOSE 3306
CMD ["mysqld"]

