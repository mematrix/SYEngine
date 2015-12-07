#ifndef __FLAC_PARSER_H
#define __FLAC_PARSER_H

struct FLACParser
{
	unsigned nch;
	unsigned rate;
	unsigned bps;

	unsigned max_blocksize;
	unsigned max_framesize;

	void Parse(unsigned char* buf, unsigned size) throw();
};

#endif //__FLAC_PARSER_H