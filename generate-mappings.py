#!/usr/bin/env python3

import json

out = open("mappings.c", "w");

with open("legacy.json") as f:
    data = json.load(f)

out.write("#include \"mappings.h\"\n\n")
out.write("struct mapping legacy_mappings[] = {\n")

defaults = {}
mappings = []
blocks = data["blocks"]
for key in blocks:
    val = blocks[key]
    _, name = val.split(":")
    blockId, blockData = [int(x) for x in key.split(":")]
    mappings.append((name, blockId, blockData))

    parts = name.split("[")
    if len(parts) > 1 and parts[0] not in defaults:
        defaults[parts[0]] = True
        mappings.append((parts[0], blockId, blockData))

mappings.sort(key=lambda x: x[0])
for name, blockId, blockData in mappings:
    out.write("\t{\"")
    out.write(name)
    out.write("\", ")
    out.write(str(blockId));
    out.write(", ")
    out.write(str(blockData))
    out.write("},\n")

out.write("};\n\n")
out.write("size_t legacy_mappings_len = ")
out.write(str(len(mappings)))
out.write(";\n")
out.close()
