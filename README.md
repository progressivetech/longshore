= longshore =

longshore helps configure [Progressive Technology Project](http://progressivetech.org)'s PowerBase network using docker and linux containers.

It is a set of bash helper scripts providing a thin wrapper around docker commands.

== Installing on Debian jessie ==

 * Install docker.io from jessie-backports

 * cd /srv && git clone git://git.progressivetech.org/provisioning/longshore.git

 * cp /srv/longshore/etc/longshore.conf.sample /srv/longshore/etc/longshore.conf

 * Edit /srv/longshore/etc/longshore.conf

 * /srv/longshore/bin/longshore init
