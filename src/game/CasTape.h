/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_CAS_TAPE_H
#define MG_CAS_TAPE_H


// A cassette tape as a .CAS image, the format MSX emulators (openMSX,
// blueMSX, fMSX) use: the bytes of each block as the BIOS writes them,
// every block preceded by the 8 byte header 1F A6 DE BA CC 13 7D 74 at an
// offset that is a multiple of 8 (zero padding before it). Long and short
// headers (the length of the sync tone) are not distinguished.
//
// The methods match the BIOS tape routines; false means the BIOS would
// return with carry set (error).

#include <stdint.h>
#include <stddef.h>

#include <vector>


class CasTape {
public:
	static const uint8_t	kHeader[8];

							CasTape();

			bool			Load(const char* path);
			bool			Save(const char* path) const;

			void			SetData(const std::vector<uint8_t>& data);
	const	std::vector<uint8_t>& Data() const { return fData; }
			bool			IsModified() const { return fModified; }
			void			SetModified(bool modified) { fModified = modified; }

			void			Rewind() { fPosition = 0; }
			size_t			Position() const { return fPosition; }
			void			SetPosition(size_t position);

	// TAPOON / TAPOUT / TAPOOF: writing starts at the current position and
	// replaces the rest of the tape, as on a real cassette.
			bool			OutOn();
			bool			Out(uint8_t value);
			void			OutOff();

	// TAPION / TAPIN / TAPIOF: TAPION finds the next header; TAPIN fails at
	// the end of the tape and runs over a header into the next block.
			bool			InOn();
			bool			In(uint8_t* value);
			void			InOff();

	// The next header at or after 'from' (or the size if there is none).
			size_t			FindHeader(size_t from) const;

private:
			bool			IsHeaderAt(size_t position) const;

			std::vector<uint8_t> fData;
			size_t			fPosition;
			bool			fWriting;
			bool			fReading;
			bool			fModified;
};


#endif	// MG_CAS_TAPE_H
