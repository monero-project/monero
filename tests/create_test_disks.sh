#!/bin/bash

set -e

LOOP_DEVICE_MAJOR=7
LOOP_DEVICE_MIN_ID=100

echo_err() {
    echo "$@" >&2
}

root_exec() {
    if [[ $EUID -ne 0 ]]; then
        ${ROOT_EXEC_CMD:-sudo} "$@"
    else
        "$@"
    fi
}

create_device_node() {
    local -r last_id=$(find /dev/ -name 'loop[0-9]*' -printf '%f\n' | sed 's/^loop//' | sort -r -n | head -1)
    local id=$((last_id + 1))
    if [[ "$id" -lt "$LOOP_DEVICE_MIN_ID" ]]; then
        id="$LOOP_DEVICE_MIN_ID"
    fi
    local path
    for (( i=0; i<10; i++ )); do
        path="/dev/loop$id"
        if [[ ! -e "$path" ]] && root_exec mknod "$path" b "$LOOP_DEVICE_MAJOR" "$id"; then
            echo "$path"
            return 0
        fi
        $((id++))
    done
    return 1
}

device_mountpoint() {
    local -r datadir="$1"
    local -r dev="$2"
    if [[ -z "$datadir" || -z "$dev" ]]; then
        echo_err "Usage: device_mountpoint <data dir> <device>"
        return 1
    fi
    echo "$datadir/mnt-$(basename "$dev")"
}

create_device() {
    local -r datadir="$1"
    if [[ -z "$datadir" ]]; then
        echo_err "Usage: create_device <data dir>"
        return 1
    fi
    local -r dev=$(create_device_node)
    local -r fs="$datadir/$(basename "$dev").vhd"
    local -r mountpoint=$(device_mountpoint "$datadir" "$dev")
    echo_err
    echo_err "# Device $dev"
    dd if=/dev/zero of="$fs" bs=64K count=128 >/dev/null 2>&1
    root_exec losetup "$dev" "$fs"
    root_exec mkfs.ext4 "$dev" >/dev/null 2>&1
    mkdir "$mountpoint"
    root_exec mount "$dev" "$mountpoint"
    echo "$dev"
}

# Unused by default, but helpful for local development
destroy_device() {
    local -r datadir="$1"
    local -r dev="$2"
    if [[ -z "$datadir" || -z "$dev" ]]; then
        echo_err "Usage: destroy_device <data dir> <device>"
        return 1
    fi
    echo_err "Destroying device $dev"
    root_exec umount $(device_mountpoint "$datadir" "$dev")
    root_exec losetup -d "$dev"
    root_exec rm "$dev"
}

block_device_path() {
    device_name=$(basename "$1")
    device_minor=${device_name/#loop}
    echo "/sys/dev/block/$LOOP_DEVICE_MAJOR:$device_minor"
}

tmpdir=$(mktemp --tmpdir -d monerotest.XXXXXXXX)
echo_err "Creating devices using temporary directory: $tmpdir"

dev_rot=$(create_device "$tmpdir")
bdev_rot=$(block_device_path "$dev_rot")
echo 1 | root_exec tee "$bdev_rot/queue/rotational" >/dev/null
echo MONERO_TEST_DEVICE_HDD=$(device_mountpoint "$tmpdir" "$dev_rot")

dev_ssd=$(create_device "$tmpdir")
bdev_ssd=$(block_device_path "$dev_ssd")
echo 0 | root_exec tee "$bdev_ssd/queue/rotational" >/dev/null
echo MONERO_TEST_DEVICE_SSD=$(device_mountpoint "$tmpdir" "$dev_ssd")
