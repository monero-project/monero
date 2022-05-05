#!/bin/sh

if [ $# -ne 1 ]; then
	echo "usage: $0 <version>"
	exit 1
fi
VERSION=$1

DOCKER=`command -v docker`
CACHER=`command -v apt-cacher-ng`

if [ -z "$DOCKER" -o -z "$CACHER" ]; then
	echo "$0: you must first install docker.io and apt-cacher-ng"
	echo "  e.g. sudo apt-get install docker.io apt-cacher-ng"
	exit 1
fi

GH_USER=${GH_USER-$USER}

TAG=gitrun-bionic
TAG2=base-bionic-amd64
IMAGE=`docker images | grep $TAG`

WORKDIR=/home/ubuntu

if [ -z "$IMAGE" ]; then
GID=`getent group docker`
mkdir -p docker
cd docker

# container for running gitian-build.py
cat <<EOF > ${TAG}.Dockerfile
FROM ubuntu:bionic

ENV DEBIAN_FRONTEND=noninteractive
RUN echo 'Acquire::http { Proxy "http://172.17.0.1:3142"; };' > /etc/apt/apt.conf.d/50cacher
RUN echo "$GID" >> /etc/group
RUN apt-get update && apt-get --no-install-recommends -y install lsb-release ruby git make wget docker.io python3 curl

RUN useradd -ms /bin/bash -U ubuntu -G docker
USER ubuntu:docker
WORKDIR $WORKDIR

RUN	git clone https://github.com/monero-project/gitian.sigs.git sigs; \
  git clone https://github.com/devrandom/gitian-builder.git builder; \
  cd builder; git checkout c0f77ca018cb5332bfd595e0aff0468f77542c23; mkdir -p inputs var; cd inputs; \
  git clone https://github.com/monero-project/monero

CMD ["sleep", "infinity"]
EOF

docker build --pull -f ${TAG}.Dockerfile -t $TAG .

cd ..
docker run -v /var/run/docker.sock:/var/run/docker.sock -d --name gitrun $TAG

fi

IMAGE=`docker images | grep $TAG2`
if [ -z "$IMAGE" ]; then
mkdir -p docker
cd docker

# container for actually running each build
cat <<EOF > ${TAG2}.Dockerfile
FROM ubuntu:bionic

ENV DEBIAN_FRONTEND=noninteractive
RUN echo 'Acquire::http { Proxy "http://172.17.0.1:3142"; };' > /etc/apt/apt.conf.d/50cacher
RUN apt-get update && apt-get --no-install-recommends -y install build-essential git language-pack-en \
  wget lsb-release curl gcc-7 g++-7 gcc g++ binutils-gold pkg-config autoconf libtool automake faketime \
  bsdmainutils ca-certificates python cmake gperf

RUN useradd -ms /bin/bash -U ubuntu
USER ubuntu:ubuntu
WORKDIR $WORKDIR

CMD ["sleep", "infinity"]
EOF

docker build --pull -f ${TAG2}.Dockerfile -t $TAG2 .

cd ..

fi

RUNNING=`docker ps | grep gitrun`
if [ -z "$RUNNING" ]; then
  BUILT=`docker ps -a | grep gitrun`
  if [ -z "$BUILT" ]; then
    docker run -v /var/run/docker.sock:/var/run/docker.sock -d --name gitrun $TAG
  else
    docker start gitrun
  fi
fi
docker cp gitian-build.py gitrun:$WORKDIR/
docker exec -t gitrun ./gitian-build.py -d -b -D -n $OPT $GH_USER $VERSION
RC=$?
if [ $RC != 0 ]; then
	exit $RC
fi
echo "\nBuild Results:\n"
docker exec gitrun sh -c "sha256sum out/$VERSION/*"
echo "\nIf these results look correct, type \"sign\" to sign them, otherwise ^C to stop now."
read check
if [ "$check" != "sign" ]; then
	echo "Not signing, bye."
	exit 1
fi

if [ ! -d sigs ]; then
	git clone https://github.com/monero-project/gitian.sigs.git sigs
	cd sigs
	git remote add $GH_USER git@github.com:$GH_USER/gitian.sigs.git
	cd ..
fi

DIRS=`docker exec gitrun sh -c "echo sigs/$VERSION-*"`
for i in $DIRS; do
	docker cp gitrun:$WORKDIR/$i sigs
	gpg --detach-sign $i/$GH_USER/*.assert
done

cd sigs
git checkout -b $VERSION
git add $VERSION-*
git commit -S -m "Add $GH_USER $VERSION"
git push --set-upstream $GH_USER $VERSION
