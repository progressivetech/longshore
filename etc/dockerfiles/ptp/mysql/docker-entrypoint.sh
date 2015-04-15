#!/bin/bash
set -e

sql_init_file=/tmp/temp.sql
mycnf=/root/.my.cnf

if [ "$1" = 'mysqld' ]; then
  if [ -f "$mycnf" ]; then
    # Initialize root password if /root/.my.cnf is available.
    password=$(grep password "$mycnf" | cut -d= -f2)
    if [ -z "$password" ]; then
      printf "Failed to extract password from %s.\n" "$mycnf"
      exit 1
    fi
    cat > "$sql_init_file" <<-EOSQL
      DELETE FROM mysql.user WHERE user = 'root' ;
      FLUSH PRIVILEGES ;
      CREATE USER 'root'@'%' IDENTIFIED BY "$password" ;
      GRANT ALL ON *.* TO 'root'@'%' WITH GRANT OPTION ;
      DROP DATABASE IF EXISTS test ;
EOSQL
    
    chown mysql:mysql "$sql_init_file"
    chmod 400 "$sql_init_file"

    echo 'FLUSH PRIVILEGES ;' >> "$sql_init_file"
    
    set -- "$@" --init-file="$sql_init_file"
  fi

  # On production servers we maximize the innodb_buffer_pool_size
  # to take advantage of all the RAM we can. However, on our dev
  # servers we may have far less RAM available so we adjust this
  # down to the default.
  if [ "$LONG_LIVE" = "n" ]; then
    printf "Not running on dev system\n"
    sed -i "s/innodb_buffer_pool_size =/; innodb_buffer_pool_size =/" /etc/mysql/conf.d/ptp.cnf
  else
    printf "Running on live system\n"
  fi
  touch /var/log/mysql/mysql-slow.log
  chown mysql:mysql /var/log/mysql/mysql-slow.log
  chown mysql:mysql /var/lib/mysql

  # Make sure we have been properly initialized
  if [ ! -d "/var/lib/mysql/mysql" ]; then
    printf "Initializing on a new system."
    mysql_install_db
  fi
fi

exec "$@"
