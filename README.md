= longshore =

longshore helps configure [Progressive Technology Project](http://progressivetech.org)'s PowerBase network using docker and linux containers.

It is a set of bash helper scripts providing a thin wrapper around docker commands.

== Installing on Debian wheezy ==

 * Install apt-transport-https

 * Add jessie repository, wheezy-backports, and docker:
   deb https://get.docker.com/ubuntu docker main
   deb http://mirror.cc.columbia.edu/debian/ jessie main
   deb http://ftp.us.debian.org/debian wheezy-backports main

 * Add docker apt key
   curl https://get.docker.io/gpg | apt-key add -
   apt-key finger | grep -C2 "36A1 D786 9245 C895 0F96  6E92 D857 6A8B A88D 21E9"

 * Pin Jessie packages so they are not installed by default
   Package: *
   Pin: release n=jessie
   Pin-Priority: 200
 
 * Install linux-image-3.16.0-0 AND initramfs-tools from wheezy-backports and
   lxc-docker from jessie and lvm2 from wheezy.

 * Reboot

 * cd /srv && git clone git://git.progressivetech.org/provisioning/longshore.git

 * cp /srv/longshore/etc/longshore.conf.sample /srv/longshore/etc/longshore.conf

 * Edit /srv/longshore/etc/longshore.conf
 * /srv/longshore/bin/longshore init
