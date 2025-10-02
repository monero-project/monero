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

## String and Integer Encoding

### Integers

With few exceptions, integers serialized in epee portable storage format are serialized
as little-endian.

### Varints

Varints are used to pack integers in an portable and space optimized way. Varints are stored as little-endian integers, with the lowest 2 bits storing the amount of bytes required, which means the largest value integer that can be packed into 1 byte is 63
(6 bits).

#### Byte Sizes

| Lowest 2 bits | Size value    | Value range                       |
|---------------|---------------|-----------------------------------|
| b00           | 1 byte        | 0 to 63                           |
| b01           | 2 bytes       | 64 to 16383                       |
| b10           | 4 bytes       | 16384 to 1073741823               |
| b11           | 8 bytes       | 1073741824 to 4611686018427387903 |

#### Representations of Example Values
|        Value         | Byte Representation (hex) |
|----------------------|---------------------------|
|                    0 | 00                        |
|                    7 | 1c                        |
|                  101 | 95 01                     |
|               17,000 | A2 09 01 00               |
|        7,942,319,744 | 03 BA 98 65 07 00 00 00   |

### Strings

These are simply length (varint) prefixed char strings without a null
terminator (though one can always add one if desired). There is no
specific encoding enforced, and in fact, many times binary blobs are
stored as these strings. This type should not be confused with the keys
in sections, as those are restricted to a maximum length of 255 and
do not use varints to encode the length.

    "Howdy" => 14 48 6F 77 64 79

### Section Keys

These are similar to strings except that they are length limited to 255
bytes, and use a single byte at the front of the string to describe the
length (as opposed to a varint).

    "Howdy" => 05 48 6F 77 64 79

## Binary Format Specification

### Header

The format must always start with the following header:

| Field            | Type     | Value      |
|------------------|----------|------------|
| Signature Part A | UInt32   | 0x01011101 |
| Signature Part B | UInt32   | 0x01020101 |
| Version          | UInt8    | 0x01       |

In total, the 9 byte header will look like this (in hex): `01 11 01 01 01 01 02 01 01`

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
| Name              | section key           |
| Type              | byte                  |
| Count<sup>1</sup> | varint                |
| Value(s)          | (type dependant data) |

<sup>1</sup> Note, this is only present if the entry type has the array flag
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
#define SERIALIZE_TYPE_DOUBLE               9
#define SERIALIZE_TYPE_STRING               10
#define SERIALIZE_TYPE_BOOL                 11
#define SERIALIZE_TYPE_OBJECT               12
```

The entry type can be bitwise OR'ed with a flag:

```cpp
#define SERIALIZE_FLAG_ARRAY              0x80
```

This signals there are multiple *values* for the entry. Since only one bit is
reserved for specifying an array, we can not directly represent nested arrays.
However, you can place each of the inner arrays inside of a section, and make
the outer array type `SERIALIZE_TYPE_OBJECT | SERIALIZE_FLAG_ARRAY`. Immediately following the type code byte is a varint specifying the length of the array.
Finally, the all the elements are serialized in sequence with no padding and
without any type information. For example:

<p style="padding-left:1em; font:italic larger serif">type, count,
value<sub>1</sub>, value<sub>2</sub>,..., value<sub>n</sub></p>

#### Entry values

It's important to understand that entry *values* can be encoded any way in which
an implementation chooses. For example, the integers can be in either big or
little endian byte order.

Entry values which are objects (i.e. `SERIALIZE_TYPE_OBJECT`), are stored as
[sections](#Section).

### Overall example

Let's put it all together and see what an entire object would look like serialized. To represent our data, let's create a JSON object (since it's a format
that most will be familiar with):

```json
{
  "short_quote": "Give me liberty or give me death",
  "long_quote": "Monero is more than just a technology. It's also what the technology stands for.",
  "signed_32bit_int": 20140418,
  "array_of_bools": [true, false, true, true],
  "nested_section": {
    "double": -6.9,
    "unsigned_64bit_int": 11111111111111111111
  }
}
```

This object would translate into the following bytes when serialized into epee portable storage format. The bytes are represented in hex, with comments and whitespace added for readability.

```
01 11 01 01 01 01 02 01                         // Signature
01                                              // Version
14                                              // Varint number of section entries (5)
0b                                              // Length of next section key (11)
73 68 6f 72 74 5f 71 75 6f 74 65                // Section key ("short_quote")
0a                                              // Type code (STRING)
80                                              // Varint length of string (32)
47 69 76 65 20 6d 65 20 6c 69 62 65 72 74 79 20 // STRING value ("Give me liberty ")
6f 72 20 67 69 76 65 20 6d 65 20 64 65 61 74 68 // STRING value cont. ("or give me death")
0a                                              // Length of next section key (10)
6c 6f 6e 67 5f 71 75 6f 74 65                   // Section key ("long_quote")
0a                                              // Type code (STRING)
41 01                                           // Varint length of string (80). Note it's 2 bytes
4d 6f 6e 65 72 6f 20 69 73 20 6d 6f 72 65 20 74 // STRING value ("Monero is more t")
68 61 6e 20 6a 75 73 74 20 61 20 74 65 63 68 6e // STRING value cont. ("han just a techn")
6f 6c 6f 67 79 2e 20 49 74 27 73 20 61 6c 73 6f // STRING value cont. ("ology. It's also")
20 77 68 61 74 20 74 68 65 20 74 65 63 68 6e 6f // STRING value cont. (" what the techno")
6c 6f 67 79 20 73 74 61 6e 64 73 20 66 6f 72 2e // STRING value cont. ("logy stands for.")
10                                              // Length of next section key (16)
73 69 67 6e 65 64 5f 33 32 62 69 74 5f 69 6e 74 // Section key ("signed_32bit_int")
02                                              // type code (INT32)
82 51 33 01                                     // INT32 value (20140418)
0e                                              // Length of next section key (14)
61 72 72 61 79 5f 6f 66 5f 62 6f 6f 6c 73       // Section key ("array_of_bools")
8b                                              // Type code (BOOL | FLAG_ARRAY)
10                                              // Varint number of array elements (4)
01 00 01 01                                     // Array BOOL values [true, false, true, true]
0e                                              // Length of next section key (14)
6e 65 73 74 65 64 5f 73 65 63 74 69 6f 6e       // Section key ("nested_section")
0c                                              // Type code (OBJECT)
08                                              // Varint number of inner section entries (2)
06                                              // Length of first inner section key (6)
64 6f 75 62 6c 65                               // Section key ("double")
09                                              // Type code (DOUBLE)
9a 99 99 99 99 99 1b c0                         // DOUBLE value (-6.9)
12                                              // Length of second inner section key (18)
75 6e 73 69 67 6e 65 64 5f 36 34 62 69 74 5f 69 // Section key ("unsigned_64bit_i")
6e 74                                           // Section key  cont ("nt")
05                                              // Type code (UINT64)
c7 71 ac b5 af 98 32 9a                         // UINT64 value (11111111111111111111)

```

## Monero specifics

### Entry values

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
