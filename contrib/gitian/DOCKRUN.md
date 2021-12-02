Quick Gitian building with docker
=================================

*Setup instructions for a Gitian build of Monero using Docker.*

Gitian supports other container mechanisms too but if you have a Debian or
Ubuntu-based host the steps can be greatly simplified.

Preparing the Gitian builder host
---------------------------------

The procedure here will create a docker container for build preparation, as well as
for actually running the builds. The only items you must install on your own host
are docker and apt-cacher-ng. With docker installed, you should also give yourself
permission to use docker by adding yourself to the docker group.

```bash
sudo apt-get install docker.io apt-cacher-ng
sudo usermod -aG docker $USER
su $USER
```

The final `su` command is needed to start a new shell with your new group membership,
since the `usermod` command doesn't affect any existing sessions.

If you want Mac binaries included in your build, you need to obtain the MacOS SDK:

```bash
curl -O https://bitcoincore.org/depends-sources/sdks/MacOSX10.11.sdk.tar.gz
```

Other User Preparation
----------------------

The final step will be to `gpg` sign the results of your build and upload them to GitHub.
Before you can do that, you'll need
* a GitHub account.
If your GitHub account name is different from your local account name, you must
set your GitHub account name for the script to use:

```bash
export GH_USER=<github account name>
```

* PGP keys - if you don't have one already, you can use `gpg --quick-gen-key` to generate it.
* a fork of the [gitian.sigs](https://github.com/monero-project/gitian.sigs/) repo on your GitHub account.
Please follow the directions there for uploading your key first.

**Note:** Please ensure your gpg public key is available to check signatures by adding it to the [gitian.sigs/gitian-pubkeys/](https://github.com/monero-project/gitian.sigs/tree/master/gitian-pubkeys) directory in a pull request.


Building the Binaries
---------------------

The dockrun.sh script will do everything to build the binaries. Just specify the
version to build as its only argument, e.g.

```bash
./dockrun.sh v0.17.3.0
```

The build should run to completion with no errors, and will display the SHA256 checksums
of the resulting binaries. You'll be prompted to check if the sums look good, and if so
then the results will be signed, and the signatures will be pushed to GitHub.

***Note: In order to publish the signed assertions via this script, you need to have your SSH key uploaded to Github beforehand. See https://docs.github.com/articles/generating-an-ssh-key/ for more info.***

You can also look in the [gitian.sigs](https://github.com/monero-project/gitian.sigs/) repo and / or [getmonero.org release checksums](https://web.getmonero.org/downloads/hashes.txt) to see if others got the same checksum for the same version tag.  If there is ever a mismatch -- **STOP! Something is wrong**.  Contact others on IRC / GitHub to figure out what is going on.


Other Options
-------------

This script just runs the [gitian-build.py](gitian-build.py) inside a container named `gitrun`.
You can set other options for that script by setting the OPT variable when running `dockrun.sh`
e.g.

```bash
# Run build processes with 8 threads
OPT="-j 8" ./dockrun.sh v0.17.3.0
```

You can also examine the build and install logs by running a shell in the container, e.g.

```bash
# Tail running logs
docker exec -it gitrun /bin/bash
tail -F builder/var/*.log

# Inspect logs, in format install-<OS>.log and build-<OS>.log
docker exec -it gitrun /bin/bash
more builder/var/install-linux.log
more builder/var/build-linux.log
```
