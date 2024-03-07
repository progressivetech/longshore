# longshore

longshore helps configure [Progressive Technology Project](http://progressivetech.org)'s PowerBase network using docker and linux containers.

It is a set of bash helper scripts providing a thin wrapper around docker commands.

## Installing on Debian Buster

 * Install docker.io and debootstrap from debian 

 * Create `longshore` user.

 * mkdir /srv && chown longshore /srv

 * As longshore user...

 * cd /srv && git clone git://git.progressivetech.org/provisioning/longshore.git

 * cp /srv/longshore/etc/longshore.conf.sample /srv/longshore/etc/longshore.conf

 * Edit /srv/longshore/etc/longshore.conf

 * As root: chown root /srv && /srv/longshore/bin/longshore init

## Setting up overlay2 and docker ==

Create a 100GB logical volume and format it as ext4.

Mount in /var/lib/docker.

Here is an example /etc/docker/daemon.json file:

    {
      "exec-opts": [ "native.cgroupdriver=cgroupfs" ],
      "storage-driver": "overlay2"
    }

