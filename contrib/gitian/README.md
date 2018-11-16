Gitian building
================

*Setup instructions for a Gitian build of Monero using a VM or physical system.*

Gitian is the deterministic build process that is used to build the Monero CLI
executables. It provides a way to be reasonably sure that the
executables are really built from the git source. It also makes sure that
the same, tested dependencies are used and statically built into the executable.

Multiple developers build the source code by following a specific descriptor
("recipe"), cryptographically sign the result, and upload the resulting signature.
These results are compared and only if they match, the build is accepted and provided
for download.

More independent Gitian builders are needed, which is why this guide exists.
It is preferred you follow these steps yourself instead of using someone else's
VM image to avoid 'contaminating' the build.

Preparing the Gitian builder host
---------------------------------

The first step is to prepare the host environment that will be used to perform the Gitian builds.
This guide explains how to set up the environment, and how to start the builds.

Gitian builds are for now executed on Ubuntu 18.04 "Bionic Beaver". A solution is being worked on to run 
it in docker in the future. Please run Ubuntu in either a VM, or on your physical machine.
You need to be logged in as the `gitianuser` in order to build gitian builds. If this user does not exist yet on your system, 
create it. 

Note that a version of `lxc-execute` higher or equal to 2.1.1 is required.
You can check the version with `lxc-execute --version`.

First we need to set up dependencies. Type/paste the following in the terminal:

```bash
sudo apt-get install git ruby apt-cacher-ng qemu-utils debootstrap lxc python-cheetah parted kpartx bridge-utils make ubuntu-archive-keyring curl firewalld
```

Then set up LXC and the rest with the following, which is a complex jumble of settings and workarounds:

```bash
sudo -s
# the version of lxc-start in Debian needs to run as root, so make sure
# that the build script can execute it without providing a password
echo "%sudo ALL=NOPASSWD: /usr/bin/lxc-start" > /etc/sudoers.d/gitian-lxc
echo "%sudo ALL=NOPASSWD: /usr/bin/lxc-execute" >> /etc/sudoers.d/gitian-lxc
# make /etc/rc.local script that sets up bridge between guest and host
echo '#!/bin/sh -e' > /etc/rc.local
echo 'brctl addbr br0' >> /etc/rc.local
echo 'ip addr add 10.0.3.1/24 broadcast 10.0.3.255 dev br0' >> /etc/rc.local
echo 'ip link set br0 up' >> /etc/rc.local
echo 'firewall-cmd --zone=trusted --add-interface=br0' >> /etc/rc.local
echo 'exit 0' >> /etc/rc.local
chmod +x /etc/rc.local
# make sure that USE_LXC is always set when logging in as gitianuser,
# and configure LXC IP addresses
echo 'export USE_LXC=1' >> /home/gitianuser/.profile
echo 'export GITIAN_HOST_IP=10.0.3.1' >> /home/gitianuser/.profile
echo 'export LXC_GUEST_IP=10.0.3.5' >> /home/gitianuser/.profile
reboot
```

This setup is required to enable networking in the container.


Manual and Building
-------------------
The instructions below use the automated script [gitian-build.py](https://github.com/betcoin/bitcoin/blob/master/contrib/gitian-build.py) which only works in Ubuntu. 
It calls all available descriptors. Help for the build steps taken can be accessed with `./gitian-build.py --help`.

Initial Gitian Setup
--------------------
The `gitian-build.py` script will checkout different release tags, so it's best to copy it:

```bash
cp monero/contrib/gitian/gitian-build.py .
```

Setup the required environment, you only need to do this once:

```
./gitian-build.py --setup fluffypony v0.14.0
```

Where `fluffypony` is your Github name and `v0.14.0` is the version tag you want to build.

While gitian and this build script does provide a way for you to sign the build directly, it is recommended to sign in a seperate step. 
This script is only there for convenience. Seperate steps for building can still be taken.
In order to sign gitian builds on your host machine, which has your PGP key, 
fork the gitian.sigs repository and clone it on your host machine, 
or pass the signed assert file back to your build machine.

```
git clone git@github.com:monero-project/gitian.sigs.git
git remote add fluffypony git@github.com:fluffypony/gitian.sigs.git
```

Build Binaries
-----------------------------
To build the most recent tag:

 `./gitian-build.py --detach-sign --no-commit -b fluffypony v0.14.0`

To speed up the build, use `-j 5 -m 5000` as the first arguments, where `5` is the number of CPU's you allocated to the VM plus one, and 5000 is a little bit less than then the MB's of RAM you allocated. If there is memory corruption on your machine, try to tweak these values.

If all went well, this produces a number of (uncommited) `.assert` files in the gitian.sigs repository.

If you do detached, offline signing, you need to copy these uncommited changes to your host machine, where you can sign them. For example:

```
export NAME=fluffypony
export VERSION=v0.14.0
gpg --output $VERSION-linux/$NAME/monero-linux-$VERSION-build.assert.sig --detach-sign $VERSION-linux/$NAME/monero-linux-$VERSION-build.assert
gpg --output $VERSION-osx-unsigned/$NAME/monero-osx-$VERSION-build.assert.sig --detach-sign $VERSION-osx-unsigned/$NAME/monero-osx-$VERSION-build.assert
gpg --output $VERSION-win-unsigned/$NAME/monero-win-$VERSION-build.assert.sig --detach-sign $VERSION-win-unsigned/$NAME/monero-win-$VERSION-build.assert
```

Make a pull request (both the `.assert` and `.assert.sig` files) to the
[monero-project/gitian.sigs](https://github.com/monero-project/gitian.sigs/) repository:

```
git checkout -b v0.14.0
git commit -S -a -m "Add $NAME v0.14.0"
git push --set-upstream $NAME v0.14.0
```

```bash
    gpg --detach-sign ${VERSION}-linux/${SIGNER}/monero-linux-*-build.assert
    gpg --detach-sign ${VERSION}-win-unsigned/${SIGNER}/monero-win-*-build.assert
    gpg --detach-sign ${VERSION}-osx-unsigned/${SIGNER}/monero-osx-*-build.assert
```

