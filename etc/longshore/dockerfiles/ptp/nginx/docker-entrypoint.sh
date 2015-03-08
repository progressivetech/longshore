#!/bin/bash
set -e

# If the command passed to docker run/exec starts with a - or -- then
# set the command to nginx and pass whatever follows as an argument to
# nginx.
if [ "${1:0:1}" = '-' ]; then
	set -- nginx "$@"
fi

if [ "$1" = 'nginx' ]; then
  set -- "$@" -g "daemon off;"
fi

exec "$@"
