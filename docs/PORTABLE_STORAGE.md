# Portable Storage Format

## Background

Monero makes use of a set of helper classes from a small library named
[epee](https://github.com/monero-project/monero/tree/master/contrib/epee). Part
of this library implements a networking protocol called
[Levin](https://github.com/monero-project/monero/blob/master/contrib/epee/include/net/levin_base.h),
which internally uses a storage format called [Portable
Storage](https://github.com/monero-project/monero/tree/master/contrib/epee/include/storages).
This format (amongst the rest of the
[epee](https://github.com/monero-project/monero/tree/master/contrib/epee)
library), is undocumented - or rather relies on the code itself to serve as the
documentation. Unfortunately, whilst the rest of the library is fairly
straightforward to decipher, the Portable Storage is less-so.  Hence this
document.

## Preliminaries

### String and integer encoding

#### varint

Varints are used to pack integers in an portable and space optimized way. The
lowest 2 bits store the amount of bytes required, which means the largest value
integer that can be packed into 1 byte is 63 (6 bits).

| Lowest 2 bits | Size value    | Value range                       |
|---------------|---------------|-----------------------------------|
| b00           | 1 byte        | 0 to 63                           |
| b01           | 2 bytes       | 64 to 16383                       |
| b10           | 4 bytes       | 16384 to 1073741823               |
| b11           | 8 bytes       | 1073741824 to 4611686018427387903 |

#### string

These are simply length (varint) prefixed char strings.

## Packet format

### Header

A packet starts with a header:

| Header        | Type      | Value                 |
|---------------|-----------|-----------------------|
| Signature     | 8 bytes   | 0x0111010101010201|   |
| Version       | byte      | 0x01                  |

### Section

Next we have a root object (or section as the library calls it). This is a map
of name-value pairs called [entries](#Entry). It starts with a count:

| Section       | Type      |
|---------------|-----------|
| Entry count   | varint    |


Which is followed by the section's name-value [entries](#Entry) sequentially:

### Entry

| Entry             | Type                  |
|-------------------|-----------------------|
| Name              | string<sup>1</sup>    |
| Type              | byte                  |
| Count<sup>2</sup> | varint                |
| Value(s)          | (type dependant data) |


<sup>1</sup> Note, the string used for the entry name is not prefixed with a
varint, it is prefixed with a single  byte to specify the length of the name.
This means an entry name cannot be more that 255 chars, which seems a reasonable
restriction.

<sup>2</sup> Note, this is only present if the entry type has the array flag
(see below).

#### Entry types

The types defined are:

```cpp
#define SERIALIZE_TYPE_INT64                1
#define SERIALIZE_TYPE_INT32                2
#define SERIALIZE_TYPE_INT16                3
#define SERIALIZE_TYPE_INT8                 4
#define SERIALIZE_TYPE_UINT64               5
#define SERIALIZE_TYPE_UINT32               6
#define SERIALIZE_TYPE_UINT16               7
#define SERIALIZE_TYPE_UINT8                8
#define SERIALIZE_TYPE_DUOBLE               9
#define SERIALIZE_TYPE_STRING               10
#define SERIALIZE_TYPE_BOOL                 11
#define SERIALIZE_TYPE_OBJECT               12
#define SERIALIZE_TYPE_ARRAY                13
```

The entry type can be bitwise OR'ed with a flag:

```cpp
#define SERIALIZE_FLAG_ARRAY              0x80
```

This signals there are multiple *values* for the entry. When we are dealing with
an array, the next value is a varint specifying the array length followed by
the array item values. For example:

<p style="padding-left:1em; font:italic larger serif">name, type, count,
value<sub>1</sub>, value<sub>2</sub>,..., value<sub>n</sub></p>

#### Entry values

It's important to understand that entry *values* can be encoded any way in which
an implementation chooses. For example, the integers can be in either big or
little endian byte order.

Entry values which are objects (i.e. `SERIALIZE_TYPE_OBJECT`), are stored as
[sections](#Section).

Note, I have not yet seen the type `SERIALIZE_TYPE_ARRAY` in use. My assumption
is this would be used for *untyped* arrays and so subsequent entries could be of
any type.

## Monero specifics

### Entry values

#### Strings

These are prefixed with a varint to specify the string length.

#### Integers

These are stored little endian byte order.

#### Hashes, Keys, Blobs

These are stored as strings, `SERIALIZE_TYPE_STRING`.

#### STL containers (vector, list)

These can be arrays of standard integer types, strings or
`SERIALIZE_TYPE_OBJECT`'s for structs.

#### Links to some Monero struct definitions

- [Core RPC
  definitions](https://github.com/monero-project/monero/blob/master/src/rpc/core_rpc_server_commands_defs.h)
- [CryptoNote protocol
  definitions](https://github.com/monero-project/monero/blob/master/src/cryptonote_protocol/cryptonote_protocol_defs.h)



[//]: # ( vim: set tw=80: )
