# Monero Blockchain Utilities

Copyright (c) 2014-2016, The Monero Project

## Introduction

The blockchain utilities allow one to convert an old style blockchain.bin file
to a new style database. There are two ways to upgrade an old style blockchain:
The recommended way is to run a `blockchain_export`, then `blockchain_import`.
The other way is to run `blockchain_converter`. In both cases, you will be left
with a new style blockchain.

For importing into the LMDB database, compile with `DATABASE=lmdb`

e.g.

`DATABASE=lmdb make release`

This is also the default compile setting on the master branch.

By default, the exporter will use the original in-memory database (blockchain.bin) as its source.
This default is to make migrating to an LMDB database easy, without having to recompile anything.
To change the source, adjust `SOURCE_DB` in `src/blockchain_utilities/bootstrap_file.h` according to the comments.

## Usage:

See also each utility's "--help" option.

### Export an existing in-memory database

`$ blockchain_export`

This loads the existing blockchain, for whichever database type it was compiled for, and exports it to `$MONERO_DATA_DIR/export/blockchain.raw`

### Import the exported file

`$ blockchain_import`

This imports blocks from `$MONERO_DATA_DIR/export/blockchain.raw` (exported using the `blockchain_export` tool as described above)
into the current database.

Defaults: `--batch on`, `--batch size 20000`, `--verify on`

Batch size refers to number of blocks and can be adjusted for performance based on available RAM.

Verification should only be turned off if importing from a trusted blockchain.

If you encounter an error like "resizing not supported in batch mode", you can just re-run
the `blockchain_import` command again, and it will restart from where it left off.

```bash
## use default settings to import blockchain.raw into database
$ blockchain_import

## fast import with large batch size, database mode "fastest", verification off
$ blockchain_import --batch-size 20000 --database lmdb#fastest --verify off

```

### Import options

`--input-file`
specifies input file path for importing

default: `<data-dir>/export/blockchain.raw`

`--output-file`
specifies output file path to export to

default: `<data-dir>/export/blockchain.raw`

`--block-stop`
stop at block number

`--database <database type>`

`--database <database type>#<flag(s)>`

database type: `lmdb, berkeley, memory`

flags:

The flag after the # is interpreted as a composite mode/flag if there's only
one (no comma separated arguments).

The composite mode represents multiple DB flags and support different database types:

`safe, fast, fastest`

Database-specific flags can be set instead.

LMDB flags (more than one may be specified):

`nosync, nometasync, writemap, mapasync, nordahead`

BerkeleyDB flags (takes one):

`txn_write_nosync, txn_nosync, txn_sync`

```
## Examples:
$ blockchain_import --database lmdb#fastest
$ blockchain_import --database berkeley#fastest

$ blockchain_import --database lmdb#nosync
$ blockchain_import --database lmdb#nosync,nometasync

$ blockchain_import --database berkeley#txn_nosync
```


### Blockchain converter with batching
`blockchain_converter` has also been updated and includes batching for faster writes. However, on lower RAM systems, this will be slower than using the exporter and importer utilities. The converter needs to keep the blockchain in memory for the duration of the conversion, like the original bitmonerod, thus leaving less memory available to the destination database to operate.

Due to higher resource use, it is recommended to use the importer with an exported file instead of the converter.

```bash
$ blockchain_converter --batch on --batch-size 20000
```
