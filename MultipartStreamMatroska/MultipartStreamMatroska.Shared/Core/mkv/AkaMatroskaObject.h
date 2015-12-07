#ifndef __AKA_MATROSKA_OBJECT_H
#define __AKA_MATROSKA_OBJECT_H

#include "AkaMatroskaGlobal.h"

#define __STR_CHECK_AND_COPY_(src, dst) \
	if (strlen((src)) < EBML_ARRAY_COUNT((dst))) strcpy((dst), (src))

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif

namespace AkaMatroska {
namespace Objects {

	struct Header
	{
		char Title[512];
		char Application[128];
		double Duration;

		inline void SetTitle(const char* str) throw()
		{ __STR_CHECK_AND_COPY_(str, Title); }
		inline void SetAppName(const char* app) throw()
		{ __STR_CHECK_AND_COPY_(app, Application); }
	};

	struct Track
	{
		unsigned UniqueId; // = 0 is default.
		enum TrackType
		{
			TrackType_Unknown = 0,
			TrackType_Audio,
			TrackType_Video,
			TrackType_Subtitle
		};
		TrackType Type;
		struct TrackProperty
		{
			bool Enabled;
			bool Default;
			bool Forced;
		};
		TrackProperty Props;

		struct CodecInfo
		{
			char CodecId[64];
			char CodecName[64];
			unsigned char* CodecPrivate;
			unsigned CodecPrivateSize;
			unsigned CodecDelay;
			bool CodecDecodeAll;

			inline void SetCodecId(const char* cid) throw()
			{ __STR_CHECK_AND_COPY_(cid, CodecId); }
			inline void SetCodecName(const char* name) throw()
			{ __STR_CHECK_AND_COPY_(name, CodecName); }

			bool SetCodecPrivate(const unsigned char* data, unsigned size) throw()
			{
				if (CodecPrivate)
					free(CodecPrivate);
				CodecPrivate = (unsigned char*)malloc(EBML_ALLOC_ALIGNED(size));
				if (CodecPrivate == nullptr)
					return false;
				CodecPrivateSize = size;
				memcpy(CodecPrivate, data, size);
				return true;
			}
		};
		CodecInfo Codec;
		struct AudioInfo
		{
			unsigned SampleRate;
			unsigned Channels;
			unsigned BitDepth;
		};
		AudioInfo Audio;
		struct VideoInfo
		{
			unsigned Width, Height;
			unsigned DisplayWidth, DisplayHeight;
			bool Interlaced;
		};
		VideoInfo Video;

		char* Name;
		char LangId[8];
		double FrameDefaultDuration;

		bool SetName(const char* name) throw()
		{
			if (Name)
				free(Name);
			Name = (char*)malloc(strlen(name) * 2);
			if (Name == nullptr)
				return false;
			strcpy(Name, name);
			return true;
		}
		inline void SetLanguage(const char* langid = "und") throw()
		{ __STR_CHECK_AND_COPY_(langid, LangId); }

		void InternalFree() throw()
		{
			if (Name)
				free(Name);
			Name = nullptr;
			if (Codec.CodecPrivate)
				free(Codec.CodecPrivate);
			Codec.CodecPrivate = nullptr;
			Codec.CodecPrivateSize = 0;
		}
	};

	struct Chapter
	{
		double Timestamp;
		char* Title;
		char LangId[8];

		bool SetTitle(const char* title) throw()
		{
			if (Title)
				free(Title);
			Title = (char*)malloc(strlen(title) * 2);
			if (Title == nullptr)
				return false;
			strcpy(Title, title);
			return true;
		}
		inline void SetLanguage(const char* langid = "und") throw()
		{ __STR_CHECK_AND_COPY_(langid, LangId); }

		void InternalFree() throw()
		{
			if (Title)
				free(Title);
			Title = nullptr;
		}
	};

	struct Tag
	{
		char* Name;
		char* String;
		char LangId[8];

		bool SetName(const char* name) throw()
		{
			if (Name)
				free(Name);
			Name = (char*)malloc(strlen(name) * 2);
			if (Name == nullptr)
				return false;
			strcpy(Name, name);
			return true;
		}
		bool SetString(const char* str) throw()
		{
			if (String)
				free(String);
			String = (char*)malloc(strlen(str) * 2);
			if (String == nullptr)
				return false;
			strcpy(String, str);
			return true;
		}
		inline void SetLanguage(const char* langid = "und") throw()
		{ __STR_CHECK_AND_COPY_(langid, LangId); }

		void InternalFree() throw()
		{
			if (Name)
				free(Name);
			if (String)
				free(String);
			Name = String = nullptr;
		}
	};

	struct Attachment
	{
		char FileName[256];
		char FileDescription[256];
		char FileMimeType[32];
		unsigned char* Data;
		unsigned DataSize;

		void SetFileName(const char* name) throw()
		{ __STR_CHECK_AND_COPY_(name, FileName); }
		void SetFileDescription(const char* desc) throw()
		{ __STR_CHECK_AND_COPY_(desc, FileDescription); }
		void SetFileMimeType(const char* mime) throw()
		{ __STR_CHECK_AND_COPY_(mime, FileMimeType); }

		bool SetData(const unsigned char* data, unsigned size) throw()
		{
			if (Data)
				free(Data);
			Data = (unsigned char*)malloc(EBML_ALLOC_ALIGNED(size));
			if (Data == nullptr)
				return false;
			DataSize = size;
			memcpy(Data, data, size);
			return true;
		}

		void InternalFree() throw()
		{
			if (Data)
				free(Data);
			Data = nullptr;
			DataSize = 0;
		}
	};
}}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#undef __STR_CHECK_AND_COPY_
#endif //__AKA_MATROSKA_OBJECT_H