#pragma once

#include <mfapi.h>
#include <initguid.h>

DEFINE_GUID(MF_MT_CORE_DEMUX_MIMETYPE,
0x607f6619, 0xd9b5, 0x4089, 0x83, 0xc5, 0xd5, 0x6a, 0x6c, 0x35, 0x75, 0x6f);
DEFINE_GUID(MF_MT_CORE_DEMUX_FRAMERATE,
0xd872bf70, 0xb2bb, 0x4032, 0xae, 0x5d, 0x81, 0xd3, 0xb6, 0xf3, 0xde, 0x64);

DEFINE_GUID(MF_MT_CORE_DEMUX_PARSER_POINTER,
0x96055856, 0xf076, 0x4b19, 0xa7, 0xbd, 0x86, 0xdf, 0x18, 0xed, 0x50, 0x4c);
DEFINE_GUID(MF_MT_CORE_DEMUX_STREAM_POINTER,
0x96055857, 0xf076, 0x4b19, 0xa7, 0xbd, 0x86, 0xdf, 0x18, 0xed, 0x50, 0x4c);

/* ---------------------------------------
               Audio Codecs
 --------------------------------------- */

DEFINE_MEDIATYPE_GUID(MFAudioFormat_ALAC,FCC('alac'));
DEFINE_MEDIATYPE_GUID(MFAudioFormat_FLAC,0x0000F1AC);

/* ---------------------------------------
               Video Codecs
 --------------------------------------- */

//H264 and HEVC with MP4
DEFINE_MEDIATYPE_GUID(MFVideoFormat_AVC1,FCC('avc1'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_HVC1,FCC('hvc1'));

//On2 VP6
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VP6,FCC('VP60'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VP6F,FCC('VP6F'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VP6A,FCC('VP6A'));

//Google VP8 and VP9
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VP8,FCC('VP80'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VP9,FCC('VP90'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VP10,FCC('VP10'));