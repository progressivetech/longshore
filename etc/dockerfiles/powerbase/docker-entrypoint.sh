#!/bin/bash
set -e

php_version=7.4

# If the command passed to docker run/exec starts with a - or -- then
# set the command.
if [ "${1:0:1}" = '-' ]; then
  set -- "php-fpm${php_version}" "$@"
fi

if [ "$1" = "php-fpm${php_version}" ]; then
  mkdir -p /run/php
  set -- "$@" --nodaemonize --fpm-config "/etc/php/${php_version}/fpm/php-fpm.conf"
fi
# On dev servers we add more logging output 
if [ "$LONG_LIVE" = "n" ]; then
  printf "display_errors = On\n" > "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"
  printf "display_startup_errors = On\n" >> "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"
  printf "error_reporting = E_ALL\n" >> "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"
  printf "html_errors = On\n" >> "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"
  printf "log_errors = On\n" >> "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"

  # Also, we enable xdebug
  # ARG. disabled because it makes everything too slow.
  #xdebug_file="/etc/php/${php_version}/mods-available/xdebug.ini"
  #xdebug_link="/etc/php/${php_version}/fpm/conf.d/20-xdebug.ini"
  #[ -f "$xdebug_file" ] && [ ! -e "$xdebug_link" ] && ln -s  "$xdebug_link"

  #printf "xdebug.remote_enable=1\n" >> "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"
  #printf "xdebug.remote_connect_back=On\n" >> "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"
  #printf "xdebug.remote_port=9000\n" >> "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"
  #printf "xdebug.profiler_enable=1\n" >> "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"
  #printf "xdebug.remote_autostart=true\n" >> "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"
  #printf "xdebug.remote_handler=dbgp\n" >> "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"
  #printf "xdebug.remote_mode=req\n" >> "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"
  #printf "xdebug.remote_host=10.11.13.1\n" >> "/etc/php/${php_version}/fpm/conf.d/99-powerbase-dev.ini"

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
  sed -i "s/newyork\.bulk\.progressivetech\.org/smtp/" /etc/esmtprc
  sed -i "s/starttls=enabled/starttls=disabled/" /etc/esmtprc
else
  # Ensure we do not have debugging enabled.
  rm -f "/etc/php/${php_version}/fpm/conf.d/20-xdebug.ini"
fi

# Allow us to overwrite the DNS caching server at runtime.
if [ -n "$LONG_RESOLV_CONF_IP" ]; then
  printf "nameserver %s\n" "$LONG_RESOLV_CONF_IP" > /etc/resolv.conf
fi
exec "$@"
