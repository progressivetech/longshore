= longshore =

longshore helps configure [Progressive Technology Project](http://progressivetech.org)'s PowerBase network using docker and linux containers.

It is a set of bash helper scripts providing a thin wrapper around docker commands.

== Installing on Debian wheezy ==

 * Add both jessie repository and wheezy-backports

 * Pin Jessie packages so they are not installed by default

 * Install linux-image-3.16.0-0 AND initramfs-tools from wheezy-backports and
   lxc-docker from jessie.

 * Be sure that lvm2 is from wheezy.
