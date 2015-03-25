#!/bin/bash
set -e

# If the command passed to docker run/exec starts with a - or -- then
# set the command to nginx and pass whatever follows as an argument to
# nginx.
if [ "${1:0:1}" = '-' ]; then
	set -- nginx "$@"
fi

if [ "$1" = 'nginx' ]; then
  # Note: nginx will still create /var/log/nginx/error.log even thought
  # it won't be used for errrors. See http://trac.nginx.org/nginx/ticket/147.
  set -- "$@" -g "daemon off; error_log /dev/stderr;"
fi

exec "$@"
