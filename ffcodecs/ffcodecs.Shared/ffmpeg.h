#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __FFMPEG__H__
#define __FFMPEG__H__

#ifndef __cplusplus
#error C++ Only.
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/avstring.h>
#include <libavutil/buffer.h>
#include <libavutil/cpu.h>
#include <libavutil/crc.h>
#include <libavutil/dict.h>
#include <libavutil/fifo.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/md5.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
#include <libavutil/parseutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/time.h>
#include <libavutil/timecode.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#ifdef _MSC_VER
#pragma comment(lib,"libavcodec.lib")
#pragma comment(lib,"libavformat.lib")
#pragma comment(lib,"libavutil.lib")
#pragma comment(lib,"libswscale.lib")
#pragma comment(lib,"libswresample.lib")
#endif

#endif