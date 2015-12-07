#ifndef __MEDIA_PACKET_H
#define __MEDIA_PACKET_H

static const double PACKET_NO_PTS = -1.0;

#pragma region Packet Flags
#define MEDIA_PACKET_BUFFER_NONE_FLAG 1 //空包flag
#define MEDIA_PACKET_END_OF_STREAM_FLAG 2 //如果有这个flag，这个包【肯定】是流的最后一个包
#define MEDIA_PACKET_CAN_TO_SKIP_FLAG 4 //可以skip不处理这个包（一般不用）
#define MEDIA_PACKET_SIDE_DATA_FLAG 8 //side_data可用
#define MEDIA_PACKET_RAW_DATA_FLAG 16 //如果包内部的数据不是压缩数据（比如读WAV文件的PCM包），这个flag明确表示
#define MEDIA_PACKET_KEY_FRAME_FLAG 32 //即便不设置这个，也可以是KeyFrame，这个flag可选，有flag表示明确是key-frame
#define MEDIA_PACKET_FIXED_BUFFER_FLAG 64 //设置这个flag，表示packet的buf是固定的，Free时不释放buf
#pragma endregion

#pragma region Packet Structures
struct AVMediaBuffer
{
	unsigned char* buf;
	unsigned size;
};

struct AVMediaPacket
{
	unsigned flag;
	AVMediaBuffer data;
	double pts;
	double dts; //-1 is unknown.
	double duration; //0 is unknown.
	long long position; //0 is unknown.
	int stream_index;
	int samples; //if flag MEDIA_PACKET_RAW_DATA_FLAG
	AVMediaBuffer side_data;
};

struct AVMediaInfoPacket
{
	unsigned mask;
	unsigned flag;
	double pts;
	double dts;
	double duration;
	long long position;
	int stream_index;
	int samples;
};

struct AVMediaPacketList
{
	AVMediaPacket packet;
	struct AVMediaPacketList* next;
};
#pragma endregion

#pragma region Packet Functions
unsigned AllocMediaPacket(AVMediaPacket* packet,unsigned size);
unsigned AllocMediaPacketAndCopy(AVMediaPacket* packet,void* pd,unsigned size);
bool CopyMediaPacket(AVMediaPacket* old_packet,AVMediaPacket* new_packet);
bool ShrinkMediaPacket(AVMediaPacket* packet,unsigned new_data_size);
bool GrowMediaPacket(AVMediaPacket* packet,unsigned add_size);
void FreeMediaPacket(AVMediaPacket* packet);
#pragma endregion

#endif //__MEDIA_PACKET_H