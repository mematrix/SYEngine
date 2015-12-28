#pragma once

#include "pch.h"
#include <vector>

namespace SYEngineCore
{
	public enum class PlaylistTypes
	{
		LocalFile,
		NetworkHttp
	};

	public value struct PlaylistNetworkConfigs
	{
		double ExplicitTotalDurationSeconds;
		bool DetectDurationForParts;
		bool DownloadRetryOnFail;
		int FetchNextPartThresholdSeconds;
		int BufferBlockSizeKB;
		int BufferBlockCount;
		Platform::String^ HttpCookie;
		Platform::String^ HttpReferer;
		Platform::String^ HttpUserAgent;
		Platform::String^ UniqueId;
	};

	public ref class Playlist sealed
	{
	public:
		Playlist(PlaylistTypes type) : _type(type) {}
		virtual ~Playlist() { Clear(); }

	public:
		bool Append(Platform::String^ url, int sizeInBytes, float durationInSeconds);
		void Clear();

		property PlaylistNetworkConfigs NetworkConfigs
		{
			PlaylistNetworkConfigs get() { return _cfgs; }
			void set(PlaylistNetworkConfigs value) { _cfgs = value; }
		}

		Windows::Foundation::IAsyncOperation<Platform::String^>^ SaveAndGetFileUriAsync();

	private:
		bool SaveFile(LPWSTR uri);
		char* SerializeForLocalFile();
		char* SerializeForNetworkHttp();

	private:
		PlaylistTypes _type;
		PlaylistNetworkConfigs _cfgs;

		struct Item
		{
			char* Url;
			int SizeInBytes;
			float DurationInSeconds;
		};
		std::vector<Item> _list;
	};

	public interface class IPlaylistNetworkUpdateInfo
	{
		property Platform::String^ Url
		{
			Platform::String^ get();
			void set(Platform::String^ value);
		}

		property int Timeout
		{
			int get();
			void set(int value);
		}

		bool SetRequestHeader(Platform::String^ name, Platform::String^ value);
		Platform::String^ GetRequestHeader(Platform::String^ name);
	};
}