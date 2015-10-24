= longshore =

longshore helps configure [Progressive Technology Project](http://progressivetech.org)'s PowerBase network using docker and linux containers.

It is a set of bash helper scripts providing a thin wrapper around docker commands.

== Installing on Debian jessie ==

 * Install docker.io from jessie-backports

 * cd /srv && git clone git://git.progressivetech.org/provisioning/longshore.git

 * cp /srv/longshore/etc/longshore.conf.sample /srv/longshore/etc/longshore.conf

 * Edit /srv/longshore/etc/longshore.conf

 * /srv/longshore/bin/longshore init

== Switching from initial install to new install ==

 * Delete /etc/apt/source.list.d/docker.list
 * Shutdown all containers 
```
for id in $(docker ps -q); do docker stop "$id"; done
```
 * Remove all containers
```
for id in $(docker ps -a -q); do docker rm "$id"; done
```
 * Remove all images
```
for id in $(docker images -a -q); do docker rmi "$id"; done
```
 * Stop Docker
```
/etc/init.d/docker stop
```
 * Uninstall docker
```
apt-get remove lxc-docker
apt-get autoremove
apt-get remove $(deborphan)
```
 * Edit MF/PL puppet file to add server name - git push new puppet configs
 * Install docker.io
 * Re-run longshore init as root.
 * Build all images
```longshore image-create all```
 * Rebuild and start all containers
```
for site in $(longshore site-list $(hostname)); do longshore site-create "$site" && longshore site-start "$site"; done
```
