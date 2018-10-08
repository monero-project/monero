Gitian building
================

*Setup instructions for a Gitian build of Monero using a VM or physical system.*

Gitian is the deterministic build process that is used to build the Bitcoin
Core executables. It provides a way to be reasonably sure that the
executables are really built from the git source. It also makes sure that
the same, tested dependencies are used and statically built into the executable.

Multiple developers build the source code by following a specific descriptor
("recipe"), cryptographically sign the result, and upload the resulting signature.
These results are compared and only if they match, the build is accepted and provided
for download.

More independent Gitian builders are needed, which is why this guide exists.
It is preferred you follow these steps yourself instead of using someone else's
VM image to avoid 'contaminating' the build.

Table of Contents
------------------

Please note that these instructions have been forked from bitcoin's gitian build
instructions. Please also consult their documentation, when running into problems.
The signing is left as inherited from bitcoin at the moment, since building currently
still fails with libiconv.

- [Preparing the Gitian builder host](#preparing-the-gitian-builder-host)
- [Getting and building the inputs](#getting-and-building-the-inputs)
- [Building Binaries](#building-bitcoin-core)
- [Signing externally](#signing-externally)
- [Uploading signatures](#uploading-signatures)

Preparing the Gitian builder host
---------------------------------

The first step is to prepare the host environment that will be used to perform the Gitian builds.
This guide explains how to set up the environment, and how to start the builds.

Gitian builds are for now executed on Ubuntu 18.04 "Bionic Beaver". Please run Ubuntu in either a VM, or on your physical machine.
You need to be logged in as the `gitianuser` in order to build gitian builds. If this user does not exist yet on your system, 
create him. 

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
The instructions below use the automated script [gitian-build.py](https://github.com/betcoin/bitcoin/blob/master/contrib/gitian-build.py) which only works in Ubuntu. For manual steps and instructions for fully offline signing, see [this guide](./gitian-building/gitian-building-manual.md).

MacOS code signing
------------------
In order to sign builds for MacOS, you need to download the free SDK and extract a file. The steps are described [here](./gitian-building/gitian-building-mac-os-sdk.md). Alternatively, you can skip the OSX build by adding `--os=lw` below.

Initial Gitian Setup
--------------------
The `gitian-build.py` script will checkout different release tags, so it's best to copy it:

```bash
cp monero/contrib/gitian/gitian-build.py .
```

You only need to do this once:

```
./gitian-build.py --setup fluffypony 0.0.20
```

Where `fluffypony` is your Github name and `0.0.20` is the most recent tag (without `v`). 

In order to sign gitian builds on your host machine, which has your PGP key, fork the gitian.sigs repository and clone it on your host machine:

```
git clone git@github.com:bitcoin-core/gitian.sigs.git
git remote add satoshi git@github.com:satoshi/gitian.sigs.git
```

Build Binaries
-----------------------------
Windows and OSX have code signed binaries, but those won't be available until a few developers have gitian signed the non-codesigned binaries.

To build the most recent tag:

 `./gitian-build.py --detach-sign --no-commit -b fluffypony 0.0.20`

To speed up the build, use `-j 5 -m 5000` as the first arguments, where `5` is the number of CPU's you allocated to the VM plus one, and 5000 is a little bit less than then the MB's of RAM you allocated. If there is memory corruption on your machine, try to tweak these values.

If all went well, this produces a number of (uncommited) `.assert` files in the gitian.sigs repository.

You need to copy these uncommited changes to your host machine, where you can sign them:

```
export NAME=satoshi
gpg --output $VERSION-linux/$NAME/bitcoin-linux-0.16-build.assert.sig --detach-sign 0.16.0rc1-linux/$NAME/bitcoin-linux-0.16-build.assert 
gpg --output $VERSION-osx-unsigned/$NAME/bitcoin-osx-0.16-build.assert.sig --detach-sign 0.16.0rc1-osx-unsigned/$NAME/bitcoin-osx-0.16-build.assert 
gpg --output $VERSION-win-unsigned/$NAME/bitcoin-win-0.16-build.assert.sig --detach-sign 0.16.0rc1-win-unsigned/$NAME/bitcoin-win-0.16-build.assert 
```

Make a PR (both the `.assert` and `.assert.sig` files) to the
[bitcoin-core/gitian.sigs](https://github.com/bitcoin-core/gitian.sigs/) repository:

```
git checkout -b 0.0.20-not-codesigned
git commit -S -a -m "Add $NAME 0.0.20 non-code signed signatures"
git push --set-upstream $NAME 0.0.20
```

You can also mail the files to Wladimir (laanwj@gmail.com) and he will commit them.

```bash
    gpg --detach-sign ${VERSION}-linux/${SIGNER}/bitcoin-linux-*-build.assert
    gpg --detach-sign ${VERSION}-win-unsigned/${SIGNER}/bitcoin-win-*-build.assert
    gpg --detach-sign ${VERSION}-osx-unsigned/${SIGNER}/bitcoin-osx-*-build.assert
```

You may have other .assert files as well (e.g. `signed` ones), in which case you should sign them too. You can see all of them by doing `ls ${VERSION}-*/${SIGNER}`.

This will create the `.sig` files that can be committed together with the `.assert` files to assert your
Gitian build.


 `./gitian-build.py --detach-sign -s satoshi 0.16.0rc1 --nocommit`

Make another pull request for these.

