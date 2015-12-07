#ifndef __MEDIA_AV_RESOURCE_H
#define __MEDIA_AV_RESOURCE_H

#define MAX_AV_RES_FILENAME_LEN 256

struct AVMediaResource
{
	char file_name[MAX_AV_RES_FILENAME_LEN]; //UTF8
	char mime_type[64];
	long long data_size;
	long long seek_pos;
};

class IAVMediaResources
{
public:
	virtual unsigned GetResourceCount() { return 0; }
	virtual AVMediaResource* GetResource(unsigned index) { return nullptr; }
};

#endif //__MEDIA_AV_RESOURCE_H