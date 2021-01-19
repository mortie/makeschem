#ifndef LEGACY_MAPPING_H
#define LEGACY_MAPPING_H

#include <stdlib.h>
#include <stdint.h>

struct mapping {
	char *name;
	uint8_t id;
	uint8_t data;
};

extern struct mapping legacy_mappings[];
extern size_t legacy_mappings_len;

#endif
