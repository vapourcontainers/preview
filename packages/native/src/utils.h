#pragma once

#include <stdint.h>
#include <stdio.h>

#include "VSHelper4.h"
#include "VSScript4.h"

#pragma pack(1)
typedef struct {
	struct {
		char magic[2];
		uint32_t file_size;
		uint16_t rev1;
		uint16_t rev2;
		uint32_t data_offset;
	} file_header;
	struct {
		uint32_t header_size;
		uint32_t width;
		uint32_t height;
		uint16_t planes;
		uint16_t bits_per_pixel;
		uint32_t compression;
		uint32_t data_size;
		uint32_t hor_res;
		uint32_t ver_res;
		uint32_t colors;
		uint32_t important_colors;
	} dib_header;
} bitmap_t;
#pragma pack()

VSNode *convert_to_rgb24(const VSAPI *vsapi, VSCore *core, VSNode *node);

ssize_t bitmap_get_size(const VSAPI *vsapi, const VSFrame *frame);
void bitmap_write(const VSAPI *vsapi, const VSFrame *frame, uint8_t *bitmap);
