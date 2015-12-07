#ifndef __AV_DESCRIPTION_H
#define __AV_DESCRIPTION_H

#ifndef MAKE_AV_DESC_TYPE
#define MAKE_AV_DESC_TYPE(a,b,c,d) \
    ((unsigned)(unsigned char)(a) | ((unsigned)(unsigned char)(b) << 8) | \
    ((unsigned)(unsigned char)(c) << 16) | ((unsigned)(unsigned char)(d) << 24 ))
#endif

#include "AVDescription_CH_Layout.h"

//用于描述FPS、AR等。
struct VideoRational
{
	int num;
	int den;
};

//用于描述图片是隔行还逐行。
enum VideoScanMode
{
	VideoScanModeUnknown = 0,
	VideoScanModeProgressive,
	VideoScanModeInterleavedUpperFirst,
	VideoScanModeInterleavedLowerFirst,
	VideoScanModeInterleavedSingleUpper,
	VideoScanModeInterleavedSingleLower,
	VideoScanModeMixedInterlaceOrProgressive
};

//如果是隔行，是PAFF还是MBAFF。
enum VideoInterlacedScanMode
{
	VideoInterlacedScanModeUnknown = 0,
	VideoInterlacedScanModePAFF,
	VideoInterlacedScanModeMBAFF
};

struct VideoBasicDescription
{
	int width; //pixel
	int height;
	int display_width, display_height; //unknown is 0;
	VideoRational frame_rate; //fps
	VideoRational aspect_ratio; //ar
	VideoScanMode scan_mode;
	VideoInterlacedScanMode interlaced_mode;
	unsigned bitrate;
	bool compressed;
};

struct AudioBasicDescription
{
	unsigned nch;
	unsigned bits;
	unsigned srate;
	unsigned ch_layout; //AVDescription_CH_Layout
	unsigned bitrate;
	unsigned wav_block_align; //for WAV media, such PCM\WMA...
	unsigned wav_avg_bytes;
	bool compressed;
};

class IAVDescription
{
public:
	virtual int GetType() = 0; //取得内部的类型标识符（可以忽略）
	virtual bool GetProfile(void*) = 0; //取得配置文件（如果没有配置文件，返回false，可以忽略）
	virtual bool GetExtradata(void*) = 0; //取得解码需要的携带数据（如果没有，返回false，可以忽略）
	virtual unsigned GetExtradataSize() = 0; //取得携带数据大小

	virtual const unsigned char* GetPrivateData() { return nullptr; }
	virtual unsigned GetPrivateSize() { return 0; }

	virtual ~IAVDescription() {}
};

class IVideoDescription : public IAVDescription
{
public:
	virtual bool GetVideoDescription(VideoBasicDescription*) = 0; //取得视频描述
	virtual bool ExternalUpdateVideoDescription(VideoBasicDescription*) { return false; }
};

class IAudioDescription : public IAVDescription
{
public:
	virtual bool GetAudioDescription(AudioBasicDescription*) = 0; //取得音频描述
	virtual bool ExternalUpdateAudioDescription(AudioBasicDescription*) { return false; }
};

inline static void UpdateAudioDescriptionBitrate(IAudioDescription* pa,int bitrate)
{
	AudioBasicDescription desc;
	if (pa->GetAudioDescription(&desc))
	{
		desc.bitrate = bitrate;
		desc.wav_avg_bytes = bitrate / 8;
		pa->ExternalUpdateAudioDescription(&desc);
	}
}

#endif //__AV_DESCRIPTION_H