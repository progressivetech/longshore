FROM my-stretch:latest
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

# Ensure networking will work.
RUN sed -i "s/^bind-address/#bind-address/g" /etc/mysql/mariadb.conf.d/50-server.cnf

COPY ptp.cnf /etc/mysql/conf.d/
COPY docker-entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
EXPOSE 3306
CMD ["mysqld_safe"]

