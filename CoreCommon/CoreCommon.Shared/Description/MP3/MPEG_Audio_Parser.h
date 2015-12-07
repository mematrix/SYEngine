#ifndef __MPEG_ADTS_PARSER_H
#define __MPEG_ADTS_PARSER_H

#define MPEG_AUDIO_VERSION_25 0
#define MPEG_AUDIO_VERSION_20 2
#define MPEG_AUDIO_VERSION_10 3

#define MPEG_AUDIO_L3 1
#define MPEG_AUDIO_L2 2
#define MPEG_AUDIO_L1 3

#define MPEG_AUDIO_CH_STEREO 0
#define MPEG_AUDIO_CH_JOINT_STEREO 1
#define MPEG_AUDIO_CH_DUAL_STEREO 2
#define MPEG_AUDIO_CH_MONO 3

bool MPEGAudioParseHead(unsigned char* pb,int* nch,int* srate,int* bitrate,int* mpeg_audio_level);

#endif //__MPEG_ADTS_PARSER_H