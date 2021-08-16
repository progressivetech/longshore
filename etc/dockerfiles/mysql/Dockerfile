FROM my-bullseye:latest
MAINTAINER Jamie McClelland <jamie@progressivetech.org>
RUN apt-get update && \
  export DEBIAN_FRONTEND=noninteractive && \
  apt-get install --no-install-recommends -y \
  mariadb-server && \
  rm -rf /var/lib/apt/lists/*

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
CMD ["mysqld"]

