#!/bin/bash
set -e

sql_init_file=/tmp/temp.sql
mycnf=/root/.my.cnf

if [ "$1" = 'mysqld_safe' ]; then
  if [ -n "$MYSQL_UID" ]; then
    # Ensure the mysql user has the same UID as the one passed in. If not,
    # change it. We make this change so that the user on the host can delete
    # mysql folders from the host if needed.
    current_uid=$(getent passwd mysql | cut -d: -f3)
    if [ "$current_uid" != "$MYSQL_UID" ]; then
      printf "Updating UID AND GID of mysql user from %s to %s.\n" "$current_uid" "$MYSQL_UID"
      usermod -u "$MYSQL_UID" mysql
      groupmod -g "$MYSQL_UID" mysql
    fi
  fi

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
    printf "Not running on live system\n"
    sed -i "s/^innodb_buffer_pool_size =/; innodb_buffer_pool_size =/" /etc/mysql/conf.d/ptp.cnf
  else
    printf "Running on live system\n"
  fi

  touch /var/log/mysql/mysql-slow.log
  chown mysql:mysql /var/log/mysql/mysql-slow.log
  chown mysql:mysql /var/lib/mysql
  #chown -R mysql:mysql /run/mysqld
fi

exec "$@"
