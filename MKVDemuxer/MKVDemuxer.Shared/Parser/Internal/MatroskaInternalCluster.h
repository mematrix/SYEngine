#ifndef _MATROSKA_INTERNAL_CLUSTER_H
#define _MATROSKA_INTERNAL_CLUSTER_H

#include "MatroskaEbml.h"

namespace MKV {
namespace Internal {
namespace Object {

class Cluster
{
public:
	Cluster() throw() : _timeCode(-1) {}

public:
	//BlockGroup (we want to call SetBlockDuration and SetKeyFrame).
	class Block
	{
	public:
		struct FrameInfo
		{
			bool KeyFrame;
			unsigned TrackNumber;
			int Timecode;
			unsigned Duration;
			unsigned Size;
		};
		struct AdditionalData
		{
			unsigned char* ConstDataPointer;
			unsigned Length;
		};

		bool InitBlock(EBML::EbmlHead* head,bool simpleBlock = true) throw();

		unsigned GetFrameCount() const throw() { return _listCount; }
		bool GetFrameInfo(FrameInfo* info,unsigned index = 0) throw();
		bool ReadFrame(void* pb,unsigned index = 0) throw();
		bool ReadFrameDirectPointer(unsigned char** ppbuf,unsigned* ppsize,unsigned index = 0) throw();

		bool ParseBlockAdditions(EBML::EbmlHead* head) throw()
		{ head->Skip(); return true; }
		const AdditionalData* GetAdditionalData() const throw()
		{ return &_core.MoreData; }

	public: //for BlockGroup
		void SetBlockDuration(long long duration) throw() { _core.Duration = duration; }
		void SetKeyFrame(bool keyframe) throw() { _core.Desc.fKeyFrame = keyframe; }

	private:
		struct BlockDescription
		{
			unsigned TrackNumber;
			short Timecode;
			bool fKeyFrame;

			enum BlockLacingType
			{
				NoLacing,
				XiphLacing,
				FixedLacing,
				EbmlLacing
			};
			BlockLacingType LacingType;
		};
		struct BlockCore
		{
			BlockDescription Desc;
			long long Duration;
			AdditionalData MoreData;
		};

		BlockCore _core;
	private:
		unsigned ParseBlockHeader() throw();
		unsigned ParseLacingType() throw();

		bool ParseSimpleBlock() throw();
		bool ParseBlock() throw(); //BlockGroup.

		bool InitFrameList() throw();
		bool InitEbmlFrameList() throw();
		bool InitXiphFrameList() throw();

		unsigned CalcListOffsetSize(unsigned index) throw();
		void SetListLastNumber() throw();

	private:
		unsigned _packet_offset;

	public:
		struct AutoBuffer
		{
			void* ptr;
			unsigned alloc_size;
			unsigned true_size;

			bool Alloc(unsigned size,bool non_zero = false) throw()
			{
				if (size > alloc_size)
				{
					Free();

					ptr = malloc(MKV_ALLOC_ALIGNED(size) + 4);
					alloc_size = true_size = size;
				}else{
					true_size = size;
				}

				if (ptr && !non_zero)
					memset(ptr,0,true_size);

				return ptr != nullptr;
			}

			void Free() throw()
			{
				if (ptr)
					free(ptr);

				ptr = nullptr;
				alloc_size = true_size = 0;
			}

			inline void ResetSize() throw() { true_size = 0; }
			inline unsigned Size() throw() { return true_size; }

			template<typename T = void>
			inline T* Get() const throw() { return (T*)ptr; }

			AutoBuffer() throw() : ptr(nullptr) { Free(); }
			AutoBuffer(unsigned size) throw() : ptr(nullptr) { Free(); Alloc(size); }
			~AutoBuffer() throw() { Free(); }
		};

	private:
		AutoBuffer _ioBuffer;
		AutoBuffer _listBuffer;
		unsigned _listCount;
	};

public:
	bool ParseCluster(EBML::EbmlHead& head,bool calc_cluster_pos = false,unsigned calc_head_size_offset = 0) throw();

	//已经加上段offset
	long long GetClusterPosition() const throw() { return _clusterOffsetInDataSource; }
	long long GetTimecode() const throw() { return _timeCode; }
	Block* ParseNextBlock(long long* next_cluster_size,bool no_more_data = true) throw();

private:
	bool ProcessSimpleBlock() throw();
	bool ProcessBlockGroup(bool no_more_data) throw();

private:
	EBML::EbmlHead* _head;
	long long _clusterOffsetInDataSource;

	long long _timeCode;
	Block _block;
};

}}} //MKV::Internal::Object.

#endif //_MATROSKA_INTERNAL_CLUSTER_H