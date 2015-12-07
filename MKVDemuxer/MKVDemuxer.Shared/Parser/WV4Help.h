#ifndef _MKV_WV4_HELP_H
#define _MKV_WV4_HELP_H

#pragma pack(1)
struct WAVPACK4_BLOCK_HEADER
{
	unsigned fourcc; //wvpk
	unsigned block_size;
	unsigned short version;
	unsigned char track_no;
	unsigned char index_no;
	unsigned total_samples;
	unsigned block_index;
	unsigned block_samples;
	unsigned flags;
	unsigned crc;

	void DefaultInit() throw()
	{
		fourcc = 0x6B707677;
		block_size = 0;
		version = 0x403;
		track_no = index_no = 0;
		total_samples = block_index = block_samples = 0;
		flags = crc = 0;
	}
};
#pragma pack()

bool MKVBlockFrameToWV4BlockHead(unsigned char* frame,unsigned frameLen,WAVPACK4_BLOCK_HEADER* wv4);

#endif //_MKV_WV4_HELP_H