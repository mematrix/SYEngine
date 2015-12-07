#include <Windows.h>
#include "DemuxerFactory.h"

#ifdef _DESKTOP_APP
#define CoreLoadLib(x) LoadLibraryW((x))
#else
#define CoreLoadLib(x) LoadPackagedLibrary((x),0)
#endif
#define CoreFreeLib(x) FreeLibrary((HMODULE)(x))

#define DEFAULT_LIB_FILE_EXT L".dll"

AVDemuxerFactory::AVDemuxerFactory() : _count_of_comps(0)
{
	_pComponents = (AVCoreDemuxer*)malloc(sizeof AVCoreDemuxer);
	memset(_pComponents,0,sizeof AVCoreDemuxer);
	
	_pCurComp = nullptr;
}

AVDemuxerFactory::~AVDemuxerFactory()
{
	for (int i = 0;i < _count_of_comps;i++)
	{
		auto p = _pComponents + i;
		if (p->mod)
			CoreFreeLib(p->mod);
	}
	free(_pComponents);
}

bool AVDemuxerFactory::NewComponent(const wchar_t* file,const char* sniff_fn,const char* create_fn,bool runtime_load)
{
	if (file == nullptr ||
		sniff_fn == nullptr ||
		create_fn == nullptr)
		return false;

	if (wcslen(file) > 128)
		return false;
	if (strlen(sniff_fn) > 64 ||
		strlen(create_fn) > 64)
		return false;

	AVCoreDemuxer core = {};
	wcscpy_s(core.file,file);
	if (wcsstr(core.file,L".") == nullptr)
		wcscat_s(core.file,DEFAULT_LIB_FILE_EXT);

	if (runtime_load)
		return NewComponentWithNoLoad(core.file,sniff_fn,create_fn);
	
	auto hm = CoreLoadLib(core.file);
	core.sniff_callback = (AVSniffFileStreamCallback)
		GetProcAddress(hm,sniff_fn);
	core.create_callback = (AVCreateDemuxerCallback)
		GetProcAddress(hm,create_fn);

	core.mod = hm;
	if (core.sniff_callback == nullptr ||
		core.create_callback == nullptr)
		return false;

	_pComponents = 
		(AVCoreDemuxer*)realloc(_pComponents,(_count_of_comps + 1) * sizeof(AVCoreDemuxer));
	if (_pComponents == nullptr)
		return false;

	auto p = _pComponents + _count_of_comps;
	memcpy(p,&core,sizeof(core));

	_count_of_comps++;
	return true;
}

bool AVDemuxerFactory::CreateFromIO(IAVMediaIO* pIo,std::shared_ptr<IAVMediaFormat>& sp)
{
	if (pIo == nullptr)
		return false;
	if (_count_of_comps == 0)
		return false;

	bool ok = false;

	for (int i = 0;i < _count_of_comps;i++)
	{
		auto p = _pComponents + i;
		if (p->runtime_load)
			if (!RuntimeLoadComponent(p))
				continue;

		if (p->sniff_callback(pIo))
		{
			pIo->Seek(0,SEEK_SET);

			auto demux = p->create_callback();
			sp = demux;

			_pCurComp = p;
			ok = true;
		}
		
		pIo->Seek(0,SEEK_SET);

		if (ok)
			break;
	}

	return ok;
}

bool AVDemuxerFactory::CreateFromSpecFile(const wchar_t* file,std::shared_ptr<IAVMediaFormat>& sp)
{
	if (file == nullptr)
		return false;

	bool ok = false;
	for (int i = 0;i < _count_of_comps;i++)
	{
		auto p = _pComponents + i;
		if (wcsicmp(p->file,file) != 0)
			continue;

		if (p->runtime_load)
			if (!RuntimeLoadComponent(p))
				return false;

		auto demux = p->create_callback();
		sp = demux;

		_pCurComp = p;
		ok = true;
		if (ok)
			break;
	}
	return ok;
}

bool AVDemuxerFactory::NewComponentWithNoLoad(const wchar_t* file,const char* sniff_fn,const char* create_fn)
{
	AVCoreDemuxer core = {};
	core.runtime_load = true;
	wcscpy_s(core.file,file);
	strcpy_s(core.sniff,sniff_fn);
	strcpy_s(core.create,create_fn);

	_pComponents = 
		(AVCoreDemuxer*)realloc(_pComponents,(_count_of_comps + 1) * sizeof(AVCoreDemuxer));
	if (_pComponents == nullptr)
		return false;

	auto p = _pComponents + _count_of_comps;
	memcpy(p,&core,sizeof(core));

	_count_of_comps++;
	return true;
}

bool AVDemuxerFactory::RuntimeLoadComponent(AVCoreDemuxer* demuxer)
{
	if (demuxer == nullptr)
		return false;
	if (demuxer->create_callback &&
		demuxer->sniff_callback)
		return true;

	auto hm = CoreLoadLib(demuxer->file);
	if (hm == nullptr)
		return false;

	demuxer->sniff_callback = (AVSniffFileStreamCallback)
		GetProcAddress(hm,demuxer->sniff);
	demuxer->create_callback = (AVCreateDemuxerCallback)
		GetProcAddress(hm,demuxer->create);

	if (demuxer->sniff_callback == nullptr ||
		demuxer->create_callback == nullptr)
		return false;

	demuxer->mod = hm;
	return true;
}