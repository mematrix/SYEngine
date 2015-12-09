#ifndef _MATROSKA_CODECIDS_H
#define _MATROSKA_CODECIDS_H

enum MatroskaCodecIds
{
	MKV_Error_CodecId = 0,

	MKV_Audio_Unknown = 1,
	MKV_Audio_MS_ACM,
	MKV_Audio_APPLE_QT,
	MKV_Audio_PCM, //LE
	MKV_Audio_PCMBE,
	MKV_Audio_FLOAT, //LE
	MKV_Audio_MP1,
	MKV_Audio_MP2,
	MKV_Audio_MP3,
	MKV_Audio_AAC,
	MKV_Audio_AC3,
	MKV_Audio_EAC3,
	MKV_Audio_DTS,
	MKV_Audio_MLP,
	MKV_Audio_TRUEHD,
	MKV_Audio_ALAC,
	MKV_Audio_FLAC,
	MKV_Audio_VORBIS,
	MKV_Audio_TTA1,
	MKV_Audio_WAVPACK4,
	MKV_Audio_OPUS,
	MKV_Audio_OPUSE,
	MKV_Audio_MPCSV8,
	MKV_Audio_TAK2,
	MKV_Audio_QDMC,
	MKV_Audio_Real_14_4,
	MKV_Audio_Real_28_8,
	MKV_Audio_Real_ATRC,
	MKV_Audio_Real_COOK,
	MKV_Audio_Real_SIPR,

	MKV_Video_Unknown = 100,
	MKV_Video_RAW,
	MKV_Video_MS_VFW,
	MKV_Video_APPLE_QT,
	MKV_Video_MJPEG,
	MKV_Video_MPEG1,
	MKV_Video_MPEG2,
	MKV_Video_MPEG4_ASP,
	MKV_Video_MPEG4_AP,
	MKV_Video_MPEG4_SP,
	MKV_Video_MPEG4_MSV3,
	MKV_Video_H264,
	MKV_Video_HEVC,
	MKV_Video_SNOW,
	MKV_Video_THEORA,
	MKV_Video_PRORES,
	MKV_Video_DIRAC,
	MKV_Video_VP8,
	MKV_Video_VP9,
	MKV_Video_VP10,
	MKV_Video_Real_RV10,
	MKV_Video_Real_RV20,
	MKV_Video_Real_RV30,
	MKV_Video_Real_RV40,

	MKV_Subtitle_Unknown = 200,
	MKV_Subtitle_Text_UTF8,
	MKV_Subtitle_Text_ASCII,
	MKV_Subtitle_Text_ASS,
	MKV_Subtitle_Text_SSA,
	MKV_Subtitle_Text_USF,
	MKV_Subtitle_VOBSUB,
	MKV_Subtitle_DVBSUB,
	MKV_Subtitle_PGS,
	MKV_Subtitle_KATE
};

struct MatroskaCodecId
{
	MatroskaCodecIds Id;
	const char* Name;
};

static const MatroskaCodecId MKVAudioCodecIds[] = {
	{MKV_Audio_MS_ACM,"A_MS/ACM"},
	{MKV_Audio_APPLE_QT,"A_QUICKTIME"},
	{MKV_Audio_PCM,"A_PCM/INT/LIT"},
	{MKV_Audio_PCMBE,"A_PCM/INT/BIG"},
	{MKV_Audio_FLOAT,"A_PCM/FLOAT/IEEE"},
	{MKV_Audio_MP1,"A_MPEG/L1"},
	{MKV_Audio_MP2,"A_MPEG/L2"},
	{MKV_Audio_MP3,"A_MPEG/L3"},
	{MKV_Audio_AAC,"A_AAC"},
	{MKV_Audio_AC3,"A_AC3"},
	{MKV_Audio_EAC3,"A_EAC3"},
	{MKV_Audio_DTS,"A_DTS"},
	{MKV_Audio_MLP,"A_MLP"},
	{MKV_Audio_TRUEHD,"A_TRUEHD"},
	{MKV_Audio_ALAC,"A_ALAC"},
	{MKV_Audio_FLAC,"A_FLAC"},
	{MKV_Audio_VORBIS,"A_VORBIS"},
	{MKV_Audio_TTA1,"A_TTA1"},
	{MKV_Audio_WAVPACK4,"A_WAVPACK4"},
	{MKV_Audio_OPUS,"A_OPUS"},
	{MKV_Audio_OPUSE,"A_OPUS/EXPERIMENTAL"},
	{MKV_Audio_TAK2,"A_TAK2"},
	{MKV_Audio_Real_14_4,"A_REAL/14_4"},
	{MKV_Audio_Real_28_8,"A_REAL/28_8"},
	{MKV_Audio_Real_ATRC,"A_REAL/ATRC"},
	{MKV_Audio_Real_COOK,"A_REAL/COOK"},
	{MKV_Audio_Real_SIPR,"A_REAL/SIPR"}
};

static const MatroskaCodecId MKVVideoCodecIds[] = {
	{MKV_Video_RAW,"V_UNCOMPRESSED"},
	{MKV_Video_MS_VFW,"V_MS/VFW/FOURCC"},
	{MKV_Video_APPLE_QT,"V_QUICKTIME"},
	{MKV_Video_MJPEG,"V_MJPEG"},
	{MKV_Video_MPEG1,"V_MPEG1"},
	{MKV_Video_MPEG2,"V_MPEG2"},
	{MKV_Video_MPEG4_ASP,"V_MPEG4/ISO/ASP"},
	{MKV_Video_MPEG4_AP,"V_MPEG4/ISO/AP"},
	{MKV_Video_MPEG4_SP,"V_MPEG4/ISO/SP"},
	{MKV_Video_MPEG4_MSV3,"V_MPEG4/MS/V3"},
	{MKV_Video_H264,"V_MPEG4/ISO/AVC"},
	{MKV_Video_HEVC,"V_MPEGH/ISO/HEVC"},
	{MKV_Video_SNOW,"V_SNOW"},
	{MKV_Video_THEORA,"V_THEORA"},
	{MKV_Video_PRORES,"V_PRORES"},
	{MKV_Video_DIRAC,"V_DIRAC"},
	{MKV_Video_VP8,"V_VP8"},
	{MKV_Video_VP9,"V_VP9"},
	{MKV_Video_VP10,"V_VP10"},
	{MKV_Video_Real_RV10,"V_REAL/RV10"},
	{MKV_Video_Real_RV20,"V_REAL/RV20"},
	{MKV_Video_Real_RV30,"V_REAL/RV30"},
	{MKV_Video_Real_RV40,"V_REAL/RV40"}
};

static const MatroskaCodecId MKVSubtitleCodecIds[] = {
	{MKV_Subtitle_Text_UTF8,"S_TEXT/UTF8"},
	{MKV_Subtitle_Text_ASCII,"S_TEXT/ASCII"},
	{MKV_Subtitle_Text_ASS,"S_TEXT/ASS"},
	{MKV_Subtitle_Text_SSA,"S_TEXT/SSA"},
	{MKV_Subtitle_Text_USF,"S_TEXT/USF"},
	{MKV_Subtitle_Text_ASS,"S_ASS"},
	{MKV_Subtitle_Text_SSA,"S_SSA"},
	{MKV_Subtitle_VOBSUB,"S_VOBSUB"},
	{MKV_Subtitle_DVBSUB,"S_DVBSUB"},
	{MKV_Subtitle_KATE,"S_KATE"},
	{MKV_Subtitle_PGS,"S_HDMV/PGS"},
};

MatroskaCodecIds MatroskaFindAudioCodecId(const char* name);
MatroskaCodecIds MatroskaFindVideoCodecId(const char* name);
MatroskaCodecIds MatroskaFindSubtitleCodecId(const char* name);
MatroskaCodecIds MatroskaFindCodecId(const char* name);

#endif //_MATROSKA_CODECIDS_H