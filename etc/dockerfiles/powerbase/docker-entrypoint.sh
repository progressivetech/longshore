#!/bin/bash
set -e

# If the command passed to docker run/exec starts with a - or -- then
# set the command.
if [ "${1:0:1}" = '-' ]; then
  set -- php-fpm7.3 "$@"
fi

if [ "$1" = 'php-fpm7.3' ]; then
  mkdir -p /run/php
  set -- "$@" --nodaemonize --fpm-config /etc/php/7.3/fpm/php-fpm.conf
fi
# On dev servers we add more logging output 
if [ "$LONG_LIVE" = "n" ]; then
  printf "display_errors = On\n" > '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'
  printf "display_startup_errors = On\n" >> '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'
  printf "error_reporting = E_ALL\n" >> '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'
  printf "html_errors = On\n" >> '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'
  printf "log_errors = On\n" >> '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'

  # Also, we enable xdebug
  printf "xdebug.remote_enable=1\n" >> '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'
  printf "xdebug.remote_connect_back=On\n" >> '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'
  printf "xdebug.remote_port=9000\n" >> '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'
  printf "xdebug.profiler_enable=1\n" >> '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'
  printf "xdebug.remote_autostart=true\n" >> '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'
  printf "xdebug.remote_handler=dbgp\n" >> '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'
  printf "xdebug.remote_mode=req\n" >> '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'
  printf "xdebug.remote_host=10.11.13.1\n" >> '/etc/php/7.3/fpm/conf.d/99-powerbase-dev.ini'

  # And we ensure the civix is installed. Sometimes download.civicrm.org is not 
  # available - we don't want to hang forever, so timeout after 30 seconds.
  if [ ! -f /usr/local/bin/civix ]; then
    wget --tries=2 --timeout=30 -O /usr/local/bin/civix https://download.civicrm.org/civix/civix.phar
    chmod 755 /usr/local/bin/civix
  fi
  
  # And activate  Jamie's private x509 cert
  if [ ! -f "/usr/local/share/ca-certificates/jamie.crt" ]; then
    mv /root/jamie.crt /usr/local/share/ca-certificates/
    update-ca-certificates 
  fi

  # And relay email to the smtp server, not bulk.mayfirst.org.
  sed -i "s/bulk\.ourpowerbase\.net/smtp/" /etc/esmtprc
fi

# We use cv to enable extensions.
if [ ! -f /usr/local/bin/cv ]; then
  wget --tries=2 --timeout=30 -O /usr/local/bin/cv https://download.civicrm.org/cv/cv.phar
  chmod 755 /usr/local/bin/cv
fi

# Allow us to overwrite the DNS caching server at runtime.
if [ -n "$LONG_RESOLV_CONF_IP" ]; then
  printf "nameserver %s\n" "$LONG_RESOLV_CONF_IP" > /etc/resolv.conf
fi
exec "$@"
