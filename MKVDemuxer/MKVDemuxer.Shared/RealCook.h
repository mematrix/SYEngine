#ifndef __REAL_COOK_H
#define __REAL_COOK_H

#pragma pack(1)
struct RealAudioPrivate //48bytes
{
	unsigned fourcc;
	unsigned short version;
	unsigned short unk1;
	unsigned char unk2[12];
	unsigned short unk3; //22bytes
	unsigned short flavor;
	unsigned coded_frame_size;
	unsigned unk4[3]; //12bytes
	unsigned short sub_packet_h;
	unsigned short frame_size;
	unsigned short sub_packet_size;
	unsigned short unk5;
};

struct RealAudioPrivateV4
{
	RealAudioPrivate header;
	unsigned short sample_rate;
	unsigned short unknown;
	unsigned short sample_size;
	unsigned short channels;
};

struct RealAudioPrivateV5
{
	RealAudioPrivate header;
	unsigned unk1;
	unsigned short unk2;
	unsigned short sample_rate;
	unsigned short unk3;
	unsigned short sample_size;
	unsigned short channels;
};
#pragma pack()

#endif //__REAL_COOK_H