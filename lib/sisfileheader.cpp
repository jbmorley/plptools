
#include "sisfileheader.h"

#include <stdio.h>
#include <stdlib.h>

void
SISFileHeader::fillFrom(uchar* buf, int* base)
{
	int ix = *base;
	m_uid1 = read32(buf, &ix);
	if (logLevel >= 1)
		printf("Got uid1 = %08x\n", m_uid1);
	m_uid2 = read32(buf, &ix);
	if (m_uid2 != 0x1000006d)
		{
		printf("Got bad uid2.\n");
		exit(1);
		}
	if (logLevel >= 2)
		printf("Got uid2 = %08x\n", m_uid2);
	m_uid3 = read32(buf, &ix);
	if (m_uid3 != 0x10000419)
		{
		printf("Got bad uid3.\n");
		exit(1);
		}
	if (logLevel >= 2)
		printf("Got uid3 = %08x\n", m_uid3);
	m_uid4 = read32(buf, &ix);
//	printf("Got uid4 = %08x\n", m_uid4);
	uint16 crc1 = 0;
	for (int i = 0; i < 12; i += 2)
		crc1 = updateCrc(crc1, buf[*base + i]);
	uint16 crc2 = 0;
	for (int i = 0; i < 12; i += 2)
		crc2 = updateCrc(crc2, buf[*base + i + 1]);
	if (logLevel >= 2)
		printf("Got first crc = %08x, wanted %08x\n",
			   crc2 << 16 | crc1, m_uid4);
	if ((crc2 << 16 | crc1) != m_uid4)
		{
		printf("Got bad crc.\n");
		exit(1);
		}
	m_crc = read16(buf, &ix);
	m_nlangs = read16(buf, &ix);
	if (logLevel >= 2)
		printf("Got %d languages\n", m_nlangs);
	m_nfiles = read16(buf, &ix);
	if (logLevel >= 2)
		printf("Got %d files\n", m_nfiles);
	m_nreqs = read16(buf, &ix);
	if (logLevel >= 2)
		printf("Got %d reqs\n", m_nreqs);
	m_installationLanguage = read16(buf, &ix);
	m_installationFiles = read16(buf, &ix);
	m_installationDrive = read32(buf, &ix);
	m_installerVersion = read32(buf, &ix);
	if (logLevel >= 2)
		printf("Got installer version: %08x\n", m_installerVersion);
	m_options = read16(buf, &ix);
	if (logLevel >= 2)
		printf("Got options: %04x\n", m_options);
	m_type = read16(buf, &ix);
	if (logLevel >= 2)
		printf("Got type: %0x\n", m_type);
	m_major = read16(buf, &ix);
	if (logLevel >= 2)
		printf("Got major: %d\n", m_major);
	m_minor = read16(buf, &ix);
	if (logLevel >= 2)
		printf("Got minor: %d\n", m_minor);
	m_minor = read32(buf, &ix);
	if (logLevel >= 2)
		printf("Got variant: %d\n", m_variant);
	m_languagePtr = read32(buf, &ix);
	if (logLevel >= 2)
		printf("Languages begin at %d\n", m_languagePtr);
	m_filesPtr = read32(buf, &ix);
	if (logLevel >= 2)
		printf("Files begin at %d\n", m_filesPtr);
	m_reqPtr = read32(buf, &ix);
	if (logLevel >= 2)
		printf("Requisites begin at %d\n", m_reqPtr);
	m_unknown = read32(buf, &ix);
	m_componentPtr = read32(buf, &ix);
	if (logLevel >= 2)
		printf("Components begin at %d\n", m_componentPtr);
	*base = ix;
}

