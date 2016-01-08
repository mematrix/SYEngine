#pragma once

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <initguid.h>

CRITICAL_SECTION* WINAPI GetCoreMFsGlobalCS();
IMFAttributes* WINAPI GetCoreMFsGlobalSettings();

//NetworkMatroskaDNTParseSeekHead -> BOOL(UINT32)
//{DA2D0C5C-F35D-411E-84BB-07BD8B12E599}
DEFINE_GUID(kNetworkMatroskaDNTParseSeekHead,
0xda2d0c5c,0xf35d,0x411e,0x84,0xbb,0x7,0xbd,0x8b,0x12,0xe5,0x99);

//NetworkByteStreamBufferTime -> Double
//{BF18CECE-34A6-4601-BA51-EC485D137A04}
DEFINE_GUID(kNetworkByteStreamBufferTime,
0xbf18cece,0x34a6,0x4601,0xba,0x51,0xec,0x48,0x5d,0x13,0x7a,0x4);

//NetworkPacketBufferTime -> Double
//{BF18CECE-34A6-4601-BA51-EC485D137A05}
DEFINE_GUID(kNetworkPacketBufferTime,
0xbf18cece,0x34a6,0x4601,0xba,0x51,0xec,0x48,0x5d,0x13,0x7a,0x5);

//NetworkPacketIOBufSize -> UINT32
//{BF18CECE-34A6-4601-BA51-EC485D137A06}
DEFINE_GUID(kNetworkPacketIOBufSize,
0xbf18cece,0x34a6,0x4601,0xba,0x51,0xec,0x48,0x5d,0x13,0x7a,0x6);

//NetworkForceUseSyncIO
//{34CBC2F7-5F03-45A4-A3E1-B6783F7E1B9E}
DEFINE_GUID(kNetworkForceUseSyncIO,
0x34cbc2f7,0x5f03,0x45a4,0xa3,0xe1,0xb6,0x78,0x3f,0x7e,0x1b,0x9e);

//NetworkForceStartTimeBuffering
//{35CBC2F7-5F03-45A4-A3E1-B6783F7E1B9E}
DEFINE_GUID(kNetworkForceStartTimeBuffering,
0x35cbc2f7,0x5f03,0x45a4,0xa3,0xe1,0xb6,0x78,0x3f,0x7e,0x1b,0x9e);

//SubtitleExternalInterface -> IUnknown*
//{A9A3434D-CCE0-4120-9C5E-66755EA3978C}
DEFINE_GUID(kSubtitleExternalInterface,
0xa9a3434d,0xcce0,0x4120,0x9c,0x5e,0x66,0x75,0x5e,0xa3,0x97,0x8c);

//SubtitleMatroskaASS2SRT -> BOOL(UINT32)
//{597350BF-ACC1-4FB3-B2A9-E6B79A67A850}
DEFINE_GUID(kSubtitleMatroskaASS2SRT,
0x597350bf,0xacc1,0x4fb3,0xb2,0xa9,0xe6,0xb7,0x9a,0x67,0xa8,0x50);

//FileMaxAllowStreamSize -> UINT64
//{1C30BA03-013C-40BF-A11D-770CD8810D1C}
DEFINE_GUID(kFileMaxAllowStreamSize,
0x1c30ba03,0x13c,0x40bf,0xa1,0x1d,0x77,0xc,0xd8,0x81,0xd,0x1c);

//CoreUseFloat2PcmAC3DTS -> BOOL(UINT32)
//{16747CD4-FD2E-4477-8BFA-11931317ED2A}
DEFINE_GUID(kCoreUseFloat2PcmAC3DTS,
0x16747cd4,0xfd2e,0x4477,0x8b,0xfa,0x11,0x93,0x13,0x17,0xed,0x2a);

//CoreUseWin10ALACDecoder
//{1342C26D-A1A7-490C-91E5-4E389B7213F2}
DEFINE_GUID(kCoreUseWin10ALACDecoder,
0x1342c26d,0xa1a7,0x490c,0x91,0xe5,0x4e,0x38,0x9b,0x72,0x13,0xf2);

//CoreUseWin10FLACDecoder
//{1342C27D-A1A7-490C-91E5-4E389B7213F2}
DEFINE_GUID(kCoreUseWin10FLACDecoder,
0x1342c27d,0xa1a7,0x490c,0x91,0xe5,0x4e,0x38,0x9b,0x72,0x13,0xf2);

//CoreUseDShowFLACDecoder
//{2342C28D-A1A7-490C-91E5-4E389B7213F2}
DEFINE_GUID(kCoreUseDShowFLACDecoder,
0x2342c28d,0xa1a7,0x490c,0x91,0xe5,0x4e,0x38,0x9b,0x72,0x13,0xf2);

//CoreUseOnlyFFmpegDemuxer
//{9FEBE075-DC06-43FD-A32B-4A17679408D4}
DEFINE_GUID(kCoreUseOnlyFFmpegDemuxer,
0x9febe075,0xdc06,0x43fd,0xa3,0x2b,0x4a,0x17,0x67,0x94,0x8,0xd4);

//CoreUseFirstAudioStreamOnly
//{DEBA292A-6872-4C92-958E-F45F851AAD38}
DEFINE_GUID(kCoreUseFirstAudioStreamOnly,
0xdeba292a,0x6872,0x4c92,0x95,0x8e,0xf4,0x5f,0x85,0x1a,0xad,0x38);

//CoreDisable10bitH264Video
//{9165F81A-C1F8-4818-980E-E7C0A6565553}
DEFINE_GUID(kCoreDisable10bitH264Video,
0x9165f81a,0xc1f8,0x4818,0x98,0xe,0xe7,0xc0,0xa6,0x56,0x55,0x53);

//CoreDisableAllVideos
//{4FF12D8D-E9FF-4D3C-9EEB-F3D1B377ED07}
DEFINE_GUID(kCoreDisableAllVideos,
0x4ff12d8d,0xe9ff,0x4d3c,0x9e,0xeb,0xf3,0xd1,0xb3,0x77,0xed,0x7);

//CoreForceBufferSamples
//{5E65B751-D591-4B1B-B7ED-7EDFE884C04D}
DEFINE_GUID(kCoreForceBufferSamples,
0x5e65b751,0xd591,0x4b1b,0xb7,0xed,0x7e,0xdf,0xe8,0x84,0xc0,0x4d);

//CoreForceUseNetowrkMode
//{6C423A5B-1717-42F0-BEF3-374D5CD8973E}
DEFINE_GUID(kCoreForceUseNetowrkMode,
0x6c423a5b,0x1717,0x42f0,0xbe,0xf3,0x37,0x4d,0x5c,0xd8,0x97,0x3e);