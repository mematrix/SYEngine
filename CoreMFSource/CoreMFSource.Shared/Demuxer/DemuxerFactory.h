#ifndef __DEMUXER_FACTORY
#define __DEMUXER_FACTORY

#include <memory>
#include <memory.h>
#include <malloc.h>
#include <MediaAVFormat.h>

typedef bool (*AVSniffFileStreamCallback)(IAVMediaIO*);
typedef std::shared_ptr<IAVMediaFormat> (*AVCreateDemuxerCallback)();

struct AVCoreDemuxer
{
	bool runtime_load;
	wchar_t file[260];
	char sniff[64];
	char create[64];
	AVSniffFileStreamCallback sniff_callback;
	AVCreateDemuxerCallback create_callback;
	void* mod;
};

class AVDemuxerFactory final
{
	AVCoreDemuxer* _pComponents;
	int _count_of_comps;

	AVCoreDemuxer* _pCurComp;

public:
	inline static std::shared_ptr<AVDemuxerFactory> CreateFactory() { return std::make_shared<AVDemuxerFactory>(); }

	AVDemuxerFactory();
	~AVDemuxerFactory();

public:
	bool NewComponent(const wchar_t* file,const char* sniff_fn,const char* create_fn,bool runtime_load = false);
	bool CreateFromIO(IAVMediaIO* pIo,std::shared_ptr<IAVMediaFormat>& sp);
	bool CreateFromSpecFile(const wchar_t* file,std::shared_ptr<IAVMediaFormat>& sp);

	inline AVCoreDemuxer* GetCurrent() throw() { return _pCurComp; }
	inline wchar_t* GetCurrentFile() throw()
	{ return _pCurComp == nullptr ? nullptr:_pCurComp->file; }
private:
	bool NewComponentWithNoLoad(const wchar_t* file,const char* sniff_fn,const char* create_fn);
	bool RuntimeLoadComponent(AVCoreDemuxer* demuxer);
};

#endif //__DEMUXER_FACTORY