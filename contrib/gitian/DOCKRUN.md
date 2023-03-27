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

You'll also need to clone the monero repository and navigate to the `contrib/gitian` directory:

```bash
git clone https://github.com/monero-project/monero.git
cd monero/contrib/gitian
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
VERSION=v0.18.2.2
./dockrun.sh $VERSION
```

The build should run to completion with no errors, and will display the SHA256 checksums
of the resulting binaries. You'll be prompted to check if the sums look good, and if so
then the results will be signed, and the signatures will be pushed to GitHub.

***Note: In order to publish the signed assertions via this script, you need to have your SSH key uploaded to GitHub beforehand. See https://docs.github.com/articles/generating-an-ssh-key/ for more info.***

You can also look in the [gitian.sigs](https://github.com/monero-project/gitian.sigs/) repo and / or [getmonero.org release checksums](https://web.getmonero.org/downloads/hashes.txt) to see if others got the same checksum for the same version tag.  If there is ever a mismatch -- **STOP! Something is wrong**.  Contact others on IRC / GitHub to figure out what is going on.


Other Options
-------------

This script just runs the [gitian-build.py](gitian-build.py) inside a container named `gitrun`.
You can set other options for that script by setting the OPT variable when running `dockrun.sh`
e.g.

```bash
# Run build processes with 8 threads
OPT="-j 8" ./dockrun.sh $VERSION
```

Post-build
----------

You can examine the build and install logs by running a shell in the container, e.g.

```bash
# Tail running logs
docker exec -it gitrun /bin/bash
tail -F builder/var/install.log
tail -F builder/var/build.log

# Inspect logs, in format install-<OS>.log and build-<OS>.log
docker exec -it gitrun /bin/bash
more builder/var/install-linux.log
more builder/var/build-linux.log
```

You can find the compiled archives inside of the container at the following directory:

```bash
docker exec -it gitrun /bin/bash
ls -la out/$VERSION/
```

To copy the compiled archives to the local host out of the Docker container, you can run the following:

```bash
mkdir out
docker cp gitrun:/home/ubuntu/out/$VERSION out
```
