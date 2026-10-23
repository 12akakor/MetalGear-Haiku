/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

/*
 * Enlarges a PNG with xBR (the filter of the game), for tools/hd/hd_auto.py.
 *
 *   xbr_png <in.png> <out.png> <factor 2-6>
 */
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "../src/platform/VideoScaler.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb/stb_image_write.h"


int
main(int argc, char** argv)
{
	if (argc != 4) {
		fprintf(stderr, "usage: xbr_png <in.png> <out.png> <factor 2-6>\n");
		return 2;
	}
	int factor = atoi(argv[3]);
	if (factor < 2 || factor > 6) {
		fprintf(stderr, "factor must be 2-6\n");
		return 2;
	}
	int width, height, channels;
	unsigned char* data = stbi_load(argv[1], &width, &height, &channels, 3);
	if (data == NULL) {
		fprintf(stderr, "cannot read %s\n", argv[1]);
		return 1;
	}
	std::vector<uint32_t> in(static_cast<size_t>(width) * height);
	for (size_t i = 0; i < in.size(); i++) {
		in[i] = 0xff000000u | (static_cast<uint32_t>(data[i * 3]) << 16)
			| (static_cast<uint32_t>(data[i * 3 + 1]) << 8) | data[i * 3 + 2];
	}
	stbi_image_free(data);
	std::vector<uint32_t> out(in.size() * factor * factor);
	XbrScaler::Scale(in.data(), width, height, factor, out.data());
	std::vector<unsigned char> rgb(out.size() * 3);
	for (size_t i = 0; i < out.size(); i++) {
		rgb[i * 3] = static_cast<unsigned char>(out[i] >> 16);
		rgb[i * 3 + 1] = static_cast<unsigned char>(out[i] >> 8);
		rgb[i * 3 + 2] = static_cast<unsigned char>(out[i]);
	}
	if (!stbi_write_png(argv[2], width * factor, height * factor, 3, rgb.data(),
			width * factor * 3)) {
		fprintf(stderr, "cannot write %s\n", argv[2]);
		return 1;
	}
	return 0;
}
