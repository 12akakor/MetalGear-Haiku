/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "CasTape.h"

#include <stdio.h>
#include <string.h>


const uint8_t CasTape::kHeader[8] = {
	0x1f, 0xa6, 0xde, 0xba, 0xcc, 0x13, 0x7d, 0x74
};


CasTape::CasTape()
	:
	fPosition(0),
	fWriting(false),
	fReading(false),
	fModified(false)
{
}


bool
CasTape::Load(const char* path)
{
	FILE* file = fopen(path, "rb");
	if (file == NULL)
		return false;
	std::vector<uint8_t> data;
	uint8_t buffer[4096];
	size_t count;
	while ((count = fread(buffer, 1, sizeof(buffer), file)) > 0)
		data.insert(data.end(), buffer, buffer + count);
	bool ok = ferror(file) == 0;
	fclose(file);
	if (!ok)
		return false;
	SetData(data);
	return true;
}


bool
CasTape::Save(const char* path) const
{
	FILE* file = fopen(path, "wb");
	if (file == NULL)
		return false;
	bool ok = fwrite(fData.data(), 1, fData.size(), file) == fData.size();
	if (fclose(file) != 0)
		ok = false;
	return ok;
}


void
CasTape::SetData(const std::vector<uint8_t>& data)
{
	fData = data;
	fPosition = 0;
	fWriting = false;
	fReading = false;
	fModified = false;
}


void
CasTape::SetPosition(size_t position)
{
	fPosition = position < fData.size() ? position : fData.size();
}


bool
CasTape::IsHeaderAt(size_t position) const
{
	return (position & 7) == 0 && position + sizeof(kHeader) <= fData.size()
		&& memcmp(fData.data() + position, kHeader, sizeof(kHeader)) == 0;
}


size_t
CasTape::FindHeader(size_t from) const
{
	for (size_t position = (from + 7) & ~static_cast<size_t>(7);
			position + sizeof(kHeader) <= fData.size(); position += 8) {
		if (IsHeaderAt(position))
			return position;
	}
	return fData.size();
}


bool
CasTape::OutOn()
{
	fReading = false;
	fData.resize(fPosition);
	while ((fData.size() & 7) != 0)
		fData.push_back(0);
	fData.insert(fData.end(), kHeader, kHeader + sizeof(kHeader));
	fPosition = fData.size();
	fWriting = true;
	fModified = true;
	return true;
}


bool
CasTape::Out(uint8_t value)
{
	if (!fWriting)
		return false;
	fData.push_back(value);
	fPosition = fData.size();
	return true;
}


void
CasTape::OutOff()
{
	fWriting = false;
}


bool
CasTape::InOn()
{
	fWriting = false;
	size_t header = FindHeader(fPosition);
	if (header >= fData.size()) {
		fPosition = fData.size();
		fReading = false;
		return false;
	}
	fPosition = header + sizeof(kHeader);
	fReading = true;
	return true;
}


bool
CasTape::In(uint8_t* value)
{
	if (!fReading)
		return false;
	if (IsHeaderAt(fPosition))
		fPosition += sizeof(kHeader);
	if (fPosition >= fData.size())
		return false;
	*value = fData[fPosition++];
	return true;
}


void
CasTape::InOff()
{
	fReading = false;
}
