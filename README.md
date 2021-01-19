# makeschem

`makeschem` is a program to convert the text-based `scdef` file format to
a `.schematic` file supported by tools like WorldEdit.

The generated schematic file is uncompressed. Many tools need a gzip-compressed
schematic file, so you should pass the generated file through gzip.

## The scdef file format

The schematics definition (`scdef`) file format is a very simple text-based
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

All unspecified blocks are set to air. Currently, `torch` is the only supported block,
and it represents a switched-off redstone torch facing north.

## TODO

* ~~Read the JSON file used by WorldEdit (`legacy.json`) to map strings to legacy
  block IDs~~
* Automatically gzip the resulting schematic
