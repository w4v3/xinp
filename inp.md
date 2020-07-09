This is a description of the `.inp` file format used for backups by the Dell Backup and Recovery tool.

I only took an effort to find out about the components relevant for extracting the files, so there are still some parts of it that I don't fully understand. If any of these things look familiar to you or you have found out more, please open an issue and let me know.

## Directory structure

Inside the backup folder, there are a number of `.inp` files sharing the same prefix `Abc`. They belong together, and at least three files should be present:

* `Abc.inp` contains the path and filename of each backed up file, together with information on where to find the backup of that file
* `Abc_0000.inp` is an SQLite database containing a table with an entry for every backed up file, with an id, some sort of hashes and an offset in bytes where to find the entry of that file in `Abc.inp`, not including the header of `Abc.inp`
* multiple `Abc_xxxx.inp` files contain the actual backup

The `Abc_0000.inp` is unnecessary for extracting the backup, so in the following, only the other two filetypes are described.

## `Abc.inp`

The file consists of a header followed by an entry for each backed up file.

The header is always `0xc0` bytes long and seems to contain a cryptographic timestamp according to [this](https://www.bsi.bund.de/SharedDocs/Zertifikate_CC/PP/aktuell/PP_0113.html) protocol. Here is an example:

```
00000000: 4353 504c 4947 4854 0700 0000 1200 0000  CSPLIGHT........
00000010: 0100 0000 0100 0000 0000 0000 0000 0000  ................
00000020: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000030: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000040: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000050: 0000 0000 0000 0000 0000 0000 0500 0000  ................
00000060: 9093 8209 0300 0000 8000 0000 0000 0000  ................
00000070: ba24 b000 0000 0000 0000 0000 0000 0000  .$..............
00000080: 4201 0000 1ba3 3d25 5d91 d301 d908 0387  B.....=%].......
00000090: 1e96 d401 3d6b 0587 1e96 d401 3d6b 0587  ....=k......=k..
000000a0: 1e96 d401 2000 0000 0000 0000 7d0f 0000  .... .......}...
000000b0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
```

Each entry consists of:

* first, a sequence of `0x14` bytes, `ff ff ff ff 00 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00`
* 8 bytes encoding the size of the file in `Abc_xxxx.inp` in bytes, without the header, little endian
* 8 mystery bytes of unknown purpose
* 8 bytes encoding the offset of the entry of the backed up file in `Abc_xxxx.inp` in bytes, little endian
* 4 bytes encoding the suffix `xxxx` for the file `Abc_xxxx.inp` in which to find the backup, little endian
* `0x1c` bytes representing some sort of abbreviated filename
* 4 bytes encoding the length of the following path in *characters*, where each character takes up two bytes, little endian
* the full path, where each character is encoded with two bytes (like utf-16 little endian), ending with `2e 00 43 00`, which seems to encode the end of the path
* some sort of signature, for example

```
00000000: 0000 3401 0000 bfc3 85b5 dd94 d201 a24a  ..4............J
00000010: 87b5 dd94 d201 a24a 87b5 dd94 d201 ceb7  .......J........
00000020: f0b3 7f95 d401 2000 0000 0000 0000 6c00  ...... .......l.
00000030: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000040: 0000 0a                                  ...
```

## `Abc_xxxx.inp`

This file contains the compressed files, one after another.

Each file starts with a header, with the following rough structure:

* 4 bytes encoding the size of the header in bytes following these 4 bytes, little endian
* header contents, for example
```
00000000: 0100 04c2 8814 0000 0030 0000 0000 0000  .........0......
00000010: 004c 0000 0001 0500 0000 0000 0515 0000  .L..............
00000020: 002c c3b1 6728 c39d c39c c38c c394 c3be  .,..g(..........
00000030: c3a9 c2b1 c2b7 c3a9 0300 0001 0500 0000  ................
00000040: 0000 0515 0000 002c c3b1 6728 c39d c39c  .......,..g(....
00000050: c38c c394 c3be c3a9 c2b1 c2b7 0102 0000  ................
00000060: 0200 5800 0300 0000 0000 1400 c3bf 011f  ..X.............
00000070: 0001 0100 0000 0000 0512 0000 0000 0018  ................
00000080: 00c3 bf01 1f00 0102 0000 0000 0005 2000  .............. .
00000090: 0000 2002 0000 0000 2400 c3bf 011f 0001  .. .....$.......
000000a0: 0500 0000 0000 0515 0000 002c c3b1 6728  ...........,..g(
000000b0: c39d c39c c38c c394 c3be c3a9 c2b1 c2b7  ................
000000c0: c3a9 0300 000a                           ......
```
* 4 bytes encoding the size of the following file in bytes, little endian

The file is compressed using an [LZ77](https://en.wikipedia.org/wiki/LZ77_and_LZ78#LZ77) type compression algorithm, which can be decompressed as follows:

If a byte `b` with first nibble `0` or `1` is encountered, insert the following `b + 1` bytes literally into the output.

If the first nibble is even, greater than `0x1` and less than `0xe`, this is a reference. If this byte and the following one have the hexadecimal representation `nx yz`, there are `n / 2 + 2` bytes to be inserted from the decompressed stream, and the bytes to be inserted are `xyz` bytes ago in the output, counting from `0` at the last inserted byte.

If the first nibble is odd, greater than `0x1` and less than `0xe`, do the same as in the preceding case, subtracting `1` from the first nibble, and add `0x1000` to the distance; i.e., if the byte and the following one are of the form `nx yz`, insert `(n - 1) / 2 + 2` bytes from `1xyz` bytes ago.

If the first nibble is `0xe` or `0xf`, read two more bytes. If they have a hexadecimal representation of `[e|f]x pq yz`, insert `pq + 9` bytes, from `xyz` bytes ago, counting from `0`. If the first nibble is `0xf`, add `0x1000` to the distance.
