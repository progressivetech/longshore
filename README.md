= longshore =

longshore helps configure [Progressive Technology Project](http://progressivetech.org)'s PowerBase network using docker and linux containers.

It is a set of bash helper scripts providing a thin wrapper around docker commands.

== Installing on Debian jessie ==

 * Install docker.io from jessie-backports

 * cd /srv && git clone git://git.progressivetech.org/provisioning/longshore.git

 * cp /srv/longshore/etc/longshore.conf.sample /srv/longshore/etc/longshore.conf

 * Edit /srv/longshore/etc/longshore.conf

 * /srv/longshore/bin/longshore init

== Setting up LVM and docker ==

These directions are geared toward Debian stretch.

There is docker [documentation on setting up lvm](http://54.71.194.30:4111/engine/userguide/storagedriver/device-mapper-driver/)

However, that documentation assumes a dedicated logical volume. Below are directions for using a logical volume in use already. It is essentially the same, however, it doesn't include the option to auto-expand when necessary.

Also note: previous versions of docker had the dm.datadev and dm.metadatadev options. New versions have replaced those options with dm.thinpooldev.

You must install the package thin-provisioning-tools:

    apt-get install thin-provisioning-tools

Next, create a logical volume for docker data and for docker meta data.

    lvcreate --wipesignatures y --size 50GB -n docker vg_turkey0
    lvcreate --wipesignatures y --size 2GB -n dockermeta vg_turkey0

Next, convert the docker volume to a thinpool, with the dockermeta volume serving as the storage for meta data.

    lvconvert -y --zero n -c 512K --thinpool vg_turkey0/docker --poolmetadata vg_turkey0/dockermeta

Done. Don't configure to auto-extend.

Here is an example /etc/docker/daemon.json file:

    {
      "exec-opts": [ "native.cgroupdriver=cgroupfs" ],
      "storage-driver": "devicemapper",
      "storage-opts": [
      "dm.thinpooldev=/dev/mapper/vg_turkey0-docker",
          "dm.use_deferred_removal=true",
          "dm.use_deferred_deletion=true"
       ]

    }

