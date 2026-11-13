# StageX release builds

## Building

### 0. Requirements

- x86_64 Linux environment
- 50GB of free space

### 1. Install docker or podman

**Docker**

Installation instructions: https://docs.docker.com/engine/install/

If you have an existing installation, make sure it supports buildx:

```bash
docker buildx version
```

**Podman**

Installation instructions: https://podman.io/docs/installation#installing-on-linux

You may need to add: `unqualified-search-registries = ["docker.io", "quay.io"]` to `/etc/containers/registries.conf`

### 2. Run a build

**Docker**

```bash
make stagex-build
```

**Podman**

```
CONTAINER_ENGINE=podman make stagex-build
```

### 3. Get hashes

```bash
make stagex-hashes
```

## Debugging

If you made changes to the source for testing purposes, you can start a build with:

```bash
FORCE_DIRTY_WORKTREE=1 make stagex-build
```

To interactively debug a build:

```bash
INTERACTIVE=1 make stagex-build
```

To build for a specific target:

```bash
HOSTS="arm64-apple-darwin" make stagex-build
```

To limit the number of build threads:

```bash
JOBS=N make stagex-build
```

If something went wrong, and you want to start over:

```bash
rm -rf stagex
```
