#!/bin/bash
set -e

# If the command passed to docker run/exec starts with a - or -- then
# set the command to php5-cgi and pass whatever follows as an argument to
# php5-cgi.
if [ "${1:0:1}" = '-' ]; then
	set -- php5-fpm "$@"
fi

if [ "$1" = 'php5-fpm' ]; then
  set -- "$@" --nodaemonize
fi
# On dev servers we add more logging output 
if [ "$LONG_LIVE" = "n" ]; then
  printf "display_errors = On\n" > '/etc/php5/fpm/conf.d/99-powerbase-dev.ini'
  printf "display_startup_errors = On\n" >> '/etc/php5/fpm/conf.d/99-powerbase-dev.ini'
  printf "error_reporting = E_ALL\n" >> '/etc/php5/fpm/conf.d/99-powerbase-dev.ini'
  printf "html_errors = On\n" >> '/etc/php5/fpm/conf.d/99-powerbase-dev.ini'
  printf "log_errors = On\n" >> '/etc/php5/fpm/conf.d/99-powerbase-dev.ini'
fi

exec "$@"
