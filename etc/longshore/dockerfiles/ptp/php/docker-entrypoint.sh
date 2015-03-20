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

  # Set the symlink.
  if [ -n "$PB_URL" ]; then
    ln -s /var/www/ourpowerbase-d7-c4.5/sites/default "/var/www/ourpowerbase-d7-c4.5/sites/$PB_URL"
  fi
fi

exec "$@"
