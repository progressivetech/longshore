#!/bin/bash
set -e

sql_init_file=/root/temp.sql

if [ "$1" = 'mysqld' ]; then
  if [ -f "/root/.my.cnf" ]; then
    # initialize root password.
    password=$(grep password /root/.my.cnf | cut -d= -f2)
    if [ -z "$password" ]; then
      echo "Failed to extract password from /root/my.cnf"
      exit 1
    fi
    cat > "$sql_init_file" <<-EOSQL
      DELETE FROM mysql.user WHERE user = 'root' ;
      CREATE USER 'root'@'%' IDENTIFIED BY "$password" ;
      GRANT ALL ON *.* TO 'root'@'%' WITH GRANT OPTION ;
      DROP DATABASE IF EXISTS test ;
EOSQL
    
    chown mysql:mysql "$sql_init_file"

    echo 'FLUSH PRIVILEGES ;' >> "$sql_init_file"
    
    set -- "$@" --init-file="$sql_init_file"
  fi
fi

exec "$@"
