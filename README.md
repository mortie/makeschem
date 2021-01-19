# makeschem

`makeschem` is a program to convert the text-based `scdef` file format to
a `.schematic` file supported by tools like WorldEdit.

The generated schematic file is uncompressed. Many tools need a gzip-compressed
schematic file, so you should pass the generated file through gzip.

## The scdef file format

The schematics definition file format (`scdef`) is a very simple text-based
file format, meant to be easy to generate from other tools.
It's line-based, where each line consists of 3 whitespace-separated numbers,
followed by a string. The numbers represent x/y/z, and the name represents
the block.

Here's an example file:

```
0 0 0 redstone_torch
9 0 0 redstone_torch
9 0 9 redstone_torch
0 0 9 redstone_torch
```

After generating a schematic from that definition and loading it into a world,
we see four redstone torches at X/Z coordinates (0,0), (9,0), (9,9) and (0,9):

![Image of schematic](https://raw.githubusercontent.com/mortie/makeschem/main/screenshot.jpg)

All unspecified blocks are set to air.

To see which block names are available, you can look at
[mappings.c](https://github.com/mortie/makeschem/blob/main/mappings.c).
The format is `{<block name>, <block ID>, <block metadata>}`, and the file is
generated from the file [legacy.json](https://github.com/mortie/makeschem/blob/main/mappings.c).

## TODO

* ~~Read the JSON file used by WorldEdit (`legacy.json`) to map strings to legacy
  block IDs~~
* Automatically gzip the resulting schematic
