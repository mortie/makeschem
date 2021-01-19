#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mappings.h"

enum tag {
	TAG_END = 0,
	TAG_BYTE = 1,
	TAG_SHORT = 2,
	TAG_INT = 3,
	TAG_LONG = 4,
	TAG_FLOAT = 5,
	TAG_DOUBLE = 6,
	TAG_BYTE_ARRAY = 7,
	TAG_STRING = 8,
	TAG_LIST = 9,
	TAG_COMPOUND = 10,
};

int nbt_write_byte(FILE *f, int8_t val) {
	return fwrite(&val, 1, 1, f) == 1;
}

int nbt_write_short(FILE *f, int16_t val) {
	char bytes[] = {(val & 0xff00) >> 8, val & 0x00ff};
	return fwrite(bytes, 1, 2, f) == 2;
}

int nbt_write_int(FILE *f, int32_t val) {
	char bytes[] = {
		(val & 0xff000000l) >> 24, (val & 0x00ff0000l) >> 16,
		(val & 0x0000ff00l) >> 8, val & 0x000000ffl,
	};
	return fwrite(bytes, 1, 4, f) == 4;
}

int nbt_write_byte_array(FILE *f, const char *data, int32_t len) {
	if (!nbt_write_int(f, len)) return 0;
	return fwrite(data, 1, len, f) == len;
}

int nbt_start_byte_array(FILE *f, int32_t len) {
	return nbt_write_int(f, len);
}

int nbt_write_string(FILE *f, const char *str) {
	size_t len = strlen(str);
	nbt_write_short(f, len);
	return fwrite(str, 1, len, f) == len;
}

int nbt_write_end(FILE *f) {
	return nbt_write_byte(f, TAG_END);
}

int nbt_start_named_tag(FILE *f, enum tag tag, const char *str) {
	if (!nbt_write_byte(f, (int8_t)tag)) return 0;
	return nbt_write_string(f, str);
}

int nbt_start_list(FILE *f, enum tag tag, int32_t length) {
	if (!nbt_write_byte(f, tag)) return 0;
	return nbt_write_int(f, length);
}

struct mapping *mappings_lookup(const char *name) {
	size_t start = 0;
	size_t end = legacy_mappings_len - 1;
	while (1) {
		size_t mid = (end - start) / 2 + start;
		struct mapping *m = &legacy_mappings[mid];
		int cmp = strcmp(name, m->name);
		if (cmp > 0) {
			start = mid;
		} else if (cmp < 0) {
			end = mid;
		} else {
			return m;
		}

		if (start >= end) {
			fprintf(stderr, "nothing found\n");
			return NULL;
		}
	}
}

#define CHUNK_SIZE 16

struct blockchunk {
	size_t x, y, z;
	uint8_t blocks[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];
	uint8_t data[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];
};

void blockchunk_set(struct blockchunk *chunk, size_t x, size_t y, size_t z, uint8_t id, uint8_t data) {
	size_t idx = (y * CHUNK_SIZE + z) * CHUNK_SIZE + x;
	chunk->blocks[idx] = id;
	chunk->data[idx] = data;
}

uint8_t blockchunk_get_id(struct blockchunk *chunk, size_t x, size_t y, size_t z) {
	size_t idx = (y * CHUNK_SIZE + z) * CHUNK_SIZE + x;
	return chunk->blocks[idx];
}

uint8_t blockchunk_get_data(struct blockchunk *chunk, size_t x, size_t y, size_t z) {
	size_t idx = (y * CHUNK_SIZE + z) * CHUNK_SIZE + x;
	return chunk->data[idx];
}

struct blocks {
	size_t width;
	size_t height;
	size_t length;

	struct blockchunk *chunks;
	size_t chunkcount;
};

size_t blocks_find_or_create_chunk(struct blocks *blocks, size_t chunkx, size_t chunky, size_t chunkz) {
	for (size_t i = 0; i < blocks->chunkcount; ++i) {
		struct blockchunk *chunk = &blocks->chunks[i];
		if (chunk->x == chunkx && chunk->y == chunky && chunk->z == chunkz) {
			return i;
		}
	}

	size_t idx = blocks->chunkcount;
	blocks->chunkcount += 1;
	blocks->chunks = realloc(blocks->chunks, sizeof(struct blockchunk) * blocks->chunkcount);
	struct blockchunk *chunk = &blocks->chunks[idx];
	memset(chunk->blocks, 0, sizeof(chunk->blocks));
	memset(chunk->data, 0, sizeof(chunk->data));
	chunk->x = chunkx;
	chunk->y = chunky;
	chunk->z = chunkz;
	return idx;
}

ssize_t blocks_find_chunk(struct blocks *blocks, size_t chunkx, size_t chunky, size_t chunkz) {
	for (size_t i = 0; i < blocks->chunkcount; ++i) {
		struct blockchunk *chunk = &blocks->chunks[i];
		if (chunk->x == chunkx && chunk->y == chunky && chunk->z == chunkz) {
			return i;
		}
	}

	return -1;
}

void blocks_set(struct blocks *blocks, size_t x, size_t y, size_t z, uint8_t id, uint8_t data) {
	size_t chunkx = x / CHUNK_SIZE;
	size_t chunky = y / CHUNK_SIZE;
	size_t chunkz = z / CHUNK_SIZE;
	size_t relx = x % CHUNK_SIZE;
	size_t rely = y % CHUNK_SIZE;
	size_t relz = z % CHUNK_SIZE;
	size_t chunkidx = blocks_find_or_create_chunk(blocks, chunkx, chunky, chunkz);
	blockchunk_set(&blocks->chunks[chunkidx], relx, rely, relz, id, data);
	if (x >= blocks->width) blocks->width = x + 1;
	if (y >= blocks->height) blocks->height = y + 1;
	if (z >= blocks->length) blocks->length = z + 1;
}

uint8_t blocks_get_id(struct blocks *blocks, size_t x, size_t y, size_t z) {
	size_t chunkx = x / CHUNK_SIZE;
	size_t chunky = y / CHUNK_SIZE;
	size_t chunkz = z / CHUNK_SIZE;
	size_t relx = x % CHUNK_SIZE;
	size_t rely = y % CHUNK_SIZE;
	size_t relz = z % CHUNK_SIZE;
	ssize_t chunkidx = blocks_find_chunk(blocks, chunkx, chunky, chunkz);
	if (chunkidx < 0) {
		return 0;
	}

	return blockchunk_get_id(&blocks->chunks[chunkidx], relx, rely, relz);
}

uint8_t blocks_get_data(struct blocks *blocks, size_t x, size_t y, size_t z) {
	size_t chunkx = x / CHUNK_SIZE;
	size_t chunky = y / CHUNK_SIZE;
	size_t chunkz = z / CHUNK_SIZE;
	size_t relx = x % CHUNK_SIZE;
	size_t rely = y % CHUNK_SIZE;
	size_t relz = z % CHUNK_SIZE;
	ssize_t chunkidx = blocks_find_chunk(blocks, chunkx, chunky, chunkz);
	if (chunkidx < 0) {
		return 0;
	}

	return blockchunk_get_data(&blocks->chunks[chunkidx], relx, rely, relz);
}

void blocks_init(struct blocks *blocks) {
	blocks->chunks = NULL;
	blocks->chunkcount = 0;
	blocks->width = blocks->length = blocks->height = 1;
}

int scdef_is_space(char ch) {
	return ch == '\0' || ch == ' ' || ch == '\t' || ch == '\n';
}

void scdef_skip_space(char *line, size_t *idx) {
	while (line[*idx] != '\0' && scdef_is_space(line[*idx])) *idx += 1;
}

void scdef_skip_to_space(char *line, size_t *idx) {
	while (line[*idx] != '\0' && !scdef_is_space(line[*idx])) *idx += 1;
}

size_t scdef_read_num(char *line, size_t *idx) {
	char *end;
	char *start = &line[*idx];
	long num = strtol(start, &end, 10);

	if (num < 0) {
		fprintf(stderr, "Error: Expected positive number\n");
		fprintf(stderr, "  Here: ... %s", start);
		fprintf(stderr, "  Line: %s", line);
		num = 0;
	} else if (end == start) {
		fprintf(stderr, "Error: Expected number\n");
		fprintf(stderr, "  Here: ... %s", start);
		fprintf(stderr, "  Line: %s", line);
		num = 0;
	} else if (end[0] != '\0' && !scdef_is_space(end[0])) {
		fprintf(stderr, "Error: Expected space after number\n");
		fprintf(stderr, "  Here: ... %s", end);
		fprintf(stderr, "  Line: %s", line);
	}

	scdef_skip_to_space(line, idx);
	return num;
}

void scdef_read_line(struct blocks *blocks, char *line) {
	size_t idx = 0;
	scdef_skip_space(line, &idx);
	size_t x = scdef_read_num(line, &idx);
	scdef_skip_space(line, &idx);
	size_t y = scdef_read_num(line, &idx);
	scdef_skip_space(line, &idx);
	size_t z = scdef_read_num(line, &idx);
	scdef_skip_space(line, &idx);

	char *name = &line[idx];
	scdef_skip_to_space(line, &idx);
	line[idx] = '\0';

	struct mapping *m = mappings_lookup(name);
	if (m == NULL) {
		fprintf(stderr, "(%zu,%zu,%zu): Unknown block name: '%s'\n", x, y, z, name);
	} else {
		blocks_set(blocks, x, y, z, m->id, m->data);
		fprintf(stderr, "(%zu,%zu,%zu): %u:%u\n", x, y, z, m->id, m->data);
	}
}

void scdef_read(struct blocks *blocks, FILE *in) {
	char line[1024];
	while (fgets(line, sizeof(line), in)) {
		scdef_read_line(blocks, line);
	}
}

int schem_write(struct blocks *blocks, FILE *out) {
	if (!nbt_start_named_tag(out, TAG_COMPOUND, "Schematic")) return 0;

	if (!nbt_start_named_tag(out, TAG_SHORT, "Width")) return 0;
	if (!nbt_write_short(out, blocks->width)) return 0;
	if (!nbt_start_named_tag(out, TAG_SHORT, "Height")) return 0;
	if (!nbt_write_short(out, blocks->height)) return 0;
	if (!nbt_start_named_tag(out, TAG_SHORT, "Length")) return 0;
	if (!nbt_write_short(out, blocks->length)) return 0;
	if (!nbt_start_named_tag(out, TAG_STRING, "Materials")) return 0;
	if (!nbt_write_string(out, "Alpha")) return 0;

	if (!nbt_start_named_tag(out, TAG_BYTE_ARRAY, "Blocks")) return 0;
	if (!nbt_start_byte_array(out, blocks->width * blocks->length * blocks->height)) return 0;
	for (size_t y = 0; y < blocks->height; ++y) {
		for (size_t z = 0; z < blocks->length; ++z) {
			for (size_t x = 0; x < blocks->width; ++x) {
				uint8_t id = blocks_get_id(blocks, x, y, z);
				nbt_write_byte(out, id);
			}
		}
	}

	if (!nbt_start_named_tag(out, TAG_BYTE_ARRAY, "Data")) return 0;
	if (!nbt_start_byte_array(out, blocks->width * blocks->length * blocks->height)) return 0;
	for (size_t y = 0; y < blocks->height; ++y) {
		for (size_t z = 0; z < blocks->length; ++z) {
			for (size_t x = 0; x < blocks->width; ++x) {
				uint8_t data = blocks_get_data(blocks, x, y, z);
				nbt_write_byte(out, data);
			}
		}
	}

	return nbt_write_end(out);
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Usage: %s <input|-> [output|-]\n", argv[0]);
		return EXIT_FAILURE;
	}

	FILE *in = stdin;
	if (strcmp(argv[1], "-") != 0) {
		in = fopen(argv[1], "r");
		if (in == NULL) {
			perror(argv[1]);
			return EXIT_FAILURE;
		}
	}

	FILE *out = stdout;
	if (strcmp(argv[2], "-") != 0) {
		out = fopen(argv[2], "w");
		if (out == NULL) {
			perror(argv[2]);
			return EXIT_FAILURE;
		}
	}

	struct blocks blocks;
	blocks_init(&blocks);
	scdef_read(&blocks, in);
	if (!schem_write(&blocks, out)) {
		if (ferror(out)) {
			perror("write");
		} else {
			fprintf(stderr, "Error: Write failed.\n");
		}
		free(blocks.chunks);
		return EXIT_FAILURE;
	}

	free(blocks.chunks);
	return EXIT_SUCCESS;
}
