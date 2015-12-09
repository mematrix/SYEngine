#ifndef _MKV_TTA1_HELP_H
#define _MKV_TTA1_HELP_H

#pragma pack(1)
struct TTA1_FILE_HEADER
{
	unsigned fourcc; //TTA1
	unsigned short format; //1 = pcm
	unsigned short nch;
	unsigned short bits;
	unsigned rate;
	unsigned samples;
	unsigned char others[12]; //crc....
};
#pragma pack()

void BuildTTA1FileHeader(unsigned nch,unsigned bits,unsigned rate,double duration,TTA1_FILE_HEADER* head);

#endif //_MKV_TTA1_HELP_H