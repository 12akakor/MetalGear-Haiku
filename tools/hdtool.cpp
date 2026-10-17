/*
 * mg-hdtool: helper for drawing HD graphics for the Metal Gear port.
 *
 *   mg-hdtool slice <export dir> <room number> <painted room.png> <hd dir> [-force]
 *       Cuts a painted (enlarged) room into HD tiles: every 8x8 cell of the
 *       original becomes hd/tiles/<key>.png at the painted scale. Tiles
 *       that already exist are kept unless -force is given.
 *   mg-hdtool coverage <export dir> <hd dir>
 *       Shows how many tiles have an HD version, and which rooms to paint
 *       next to cover the most missing tiles.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb/stb_image_write.h"


static bool
exists(const std::string& path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}


static std::vector<std::string>
room_keys(const std::string& exportDir, int room)
{
	char name[64];
	snprintf(name, sizeof(name), "/rooms/room_%03d.keys", room);
	std::ifstream in(exportDir + name);
	std::vector<std::string> keys;
	std::string k;
	while (in >> k)
		keys.push_back(k);
	return keys;
}


static int
slice(const std::string& exportDir, int room, const char* painted, const std::string& hdDir,
	bool force)
{
	std::vector<std::string> keys = room_keys(exportDir, room);
	if (keys.size() != 32 * 24) {
		fprintf(stderr, "no key file for room %d in %s\n", room, exportDir.c_str());
		return 1;
	}
	int w, h, n;
	unsigned char* image = stbi_load(painted, &w, &h, &n, 4);
	if (image == NULL) {
		fprintf(stderr, "cannot read %s\n", painted);
		return 1;
	}
	if (w % 256 != 0 || h * 256 != w * 192) {
		fprintf(stderr, "%s is %dx%d; it must be the room enlarged by a whole factor"
			" (1024x768 for 4x, 2048x1536 for 8x)\n", painted, w, h);
		stbi_image_free(image);
		return 1;
	}
	int scale = w / 256, cell = 8 * scale;
	mkdir(hdDir.c_str(), 0755);
	mkdir((hdDir + "/tiles").c_str(), 0755);
	int written = 0, kept = 0;
	std::set<std::string> done;
	for (int ty = 0; ty < 24; ty++) {
		for (int tx = 0; tx < 32; tx++) {
			const std::string& key = keys[ty * 32 + tx];
			if (key == "-" || done.count(key))
				continue;
			done.insert(key);
			std::string path = hdDir + "/tiles/" + key + ".png";
			if (!force && exists(path)) {
				kept++;
				continue;
			}
			std::vector<unsigned char> tile((size_t)cell * cell * 4);
			for (int y = 0; y < cell; y++)
				memcpy(&tile[(size_t)y * cell * 4],
					&image[((size_t)(ty * cell + y) * w + tx * cell) * 4], (size_t)cell * 4);
			stbi_write_png(path.c_str(), cell, cell, 4, tile.data(), cell * 4);
			written++;
		}
	}
	stbi_image_free(image);
	printf("room %d: %d HD tiles written (%dx), %d already present\n", room, written, scale, kept);
	return 0;
}


static int
coverage(const std::string& exportDir, const std::string& hdDir)
{
	std::ifstream csv(exportDir + "/tiles.csv");
	std::string line;
	std::getline(csv, line);
	std::set<std::string> all, missing;
	while (std::getline(csv, line)) {
		std::string key = line.substr(0, line.find(','));
		all.insert(key);
		if (!exists(hdDir + "/tiles/" + key + ".png"))
			missing.insert(key);
	}
	printf("%zu of %zu tiles have an HD version (%.0f%%)\n", all.size() - missing.size(),
		all.size(), all.empty() ? 0.0 : 100.0 * (all.size() - missing.size()) / all.size());
	/* Greedy: rooms that cover the most missing tiles. */
	std::map<int, std::set<std::string> > roomMissing;
	for (int r = 0; r < 251; r++) {
		for (auto& k : room_keys(exportDir, r)) {
			if (missing.count(k))
				roomMissing[r].insert(k);
		}
	}
	printf("rooms to paint next (new tiles covered):\n");
	std::set<std::string> covered;
	for (int i = 0; i < 15 && covered.size() < missing.size(); i++) {
		int best = -1;
		size_t bestCount = 0;
		for (auto& rm : roomMissing) {
			size_t c = 0;
			for (auto& k : rm.second)
				c += covered.count(k) == 0;
			if (c > bestCount) {
				bestCount = c;
				best = rm.first;
			}
		}
		if (best < 0)
			break;
		for (auto& k : roomMissing[best])
			covered.insert(k);
		printf("  room %3d: %zu\n", best, bestCount);
	}
	return 0;
}


int
main(int argc, char** argv)
{
	if (argc >= 6 && strcmp(argv[1], "slice") == 0)
		return slice(argv[2], atoi(argv[3]), argv[4], argv[5],
			argc > 6 && strcmp(argv[6], "-force") == 0);
	if (argc >= 4 && strcmp(argv[1], "coverage") == 0)
		return coverage(argv[2], argv[3]);
	fprintf(stderr,
		"usage:\n"
		"  mg-hdtool slice <export dir> <room> <painted room.png> <hd dir> [-force]\n"
		"  mg-hdtool coverage <export dir> <hd dir>\n");
	return 2;
}
