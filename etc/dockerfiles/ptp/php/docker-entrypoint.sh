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

  # Set the symlink if we are building a PB site. 
  if [ -n "$PB_URL" ] && [ -n "$PB_PLATFORM" ]; then
    source="/var/www/html/${PB_PLATFORM}/sites/default"
    if [ ! -d "$source" ]; then
      printf "Failed to find platform (%s)\n" "$source"
      exit 1
    fi
    ln -s "$source" "/var/www/${PB_PLATFORM}/sites/$PB_URL"
  fi
fi

exec "$@"
