/*
 * mg-romcheck: says whether a ROM image works with the port, and why not.
 * Prints only checksums and the result, never ROM contents, so the output
 * can be shared safely.
 *
 *   mg-romcheck <rom> [...]
 */
#include "../src/game/GameData.h"

#include <stdio.h>
#include <vector>

int
main(int argc, char** argv)
{
	int bad = 0;
	for (int i = 1; i < argc; i++) {
		FILE* fp = fopen(argv[i], "rb");
		if (fp == NULL) {
			printf("%s: cannot open\n", argv[i]);
			bad++;
			continue;
		}
		std::vector<uint8_t> image;
		uint8_t buf[65536];
		size_t n;
		while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
			image.insert(image.end(), buf, buf + n);
		fclose(fp);
		RomInfo info = IdentifyRom(image);
		printf("%s\n  size %zu, CRC32 %08X, SHA1 %s\n  result: %s%s\n", argv[i],
			info.originalSize, info.crc32, info.sha1,
			info.version == kRomUnknown ? "NOT SUPPORTED"
				: info.exact ? "OK, " : "probably OK, ",
			info.version == kRomUnknown ? "" : RomVersionName(info.version));
		if (!info.note.empty())
			printf("  note: %s\n", info.note.c_str());
		if (info.version != kRomUnknown) {
			printf("  equal to the reference: %d%% of the blocks, %d of 16 banks%s\n",
				info.similarity, info.matchingBanks, info.scc ? "; SCC version" : "");
		} else
			bad++;
	}
	return bad ? 1 : 0;
}
