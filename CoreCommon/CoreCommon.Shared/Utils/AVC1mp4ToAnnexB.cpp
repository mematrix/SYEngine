#include <stdlib.h>
#include <memory.h>

#pragma pack(1)
struct AVCDHeader
{
	unsigned char configurationVersion;
	unsigned char AVCProfileIndication;
	unsigned char profile_compatibility;
	unsigned char AVCLevelIndication;
	unsigned char lengthSizeMinusOne;
};
#pragma pack()

unsigned MakeAVCDecoderConfigurationRecord(unsigned char* sps,unsigned sps_size,unsigned char* pps,unsigned pps_size,unsigned char* avcc)
{
	if (sps == nullptr || pps == nullptr ||
		sps_size == 0 || pps_size == 0)
		return 0;

	if (sps[0] == 0)
		return 0;

	AVCDHeader head = {};
	head.configurationVersion = 1;
	head.AVCProfileIndication = sps[1];
	head.profile_compatibility = 0;
	head.AVCLevelIndication = sps[3];
	head.lengthSizeMinusOne = 0xFF; //nal size = 4.

	memcpy(avcc,&head,5);
	avcc[5] = 0xE1; //1 sps.

	unsigned short temp = sps_size;
	avcc[6] = *(((unsigned char*)&sps_size) + 1);
	avcc[7] = *(unsigned char*)&sps_size;

	memcpy(&avcc[8],sps,sps_size);
	unsigned char* offset = &avcc[8 + sps_size];

	offset[0] = 1; //1 pps.

	temp = pps_size;
	offset[1] = *(((unsigned char*)&pps_size) + 1);
	offset[2] = *(unsigned char*)&pps_size;
	
	memcpy(&offset[3],pps,pps_size);

	return sps_size + pps_size + 8 + 3;
}

unsigned AddPPSToAVCDecoderConfigurationRecord(unsigned char* avcc,unsigned char* pps,unsigned pps_size)
{
	if (avcc == nullptr || pps == nullptr || pps_size == 0)
		return 0;

	if (avcc[5] != 0xE1) //sps need to 1.
		return 0;

	unsigned short sps_offset = 0;
	*(unsigned char*)&sps_offset = avcc[7];
	*((unsigned char*)&sps_offset + 1) = avcc[6];

	unsigned char* offset = &avcc[8 + sps_offset];

	unsigned char* count = &offset[0];
	unsigned pps_count = offset[0];
	offset++;

	for (unsigned i = 0;i < pps_count;i++)
	{
		unsigned short size = 0;
		*(unsigned char*)&size = offset[1];
		*((unsigned char*)&size + 1) = offset[0];

		offset += size + 2;
	}

	unsigned short temp = pps_size;
	offset[0] = *(((unsigned char*)&pps_size) + 1);
	offset[1] = *(unsigned char*)&pps_size;

	memcpy(&offset[2],pps,pps_size);
	*count = *count + 1;

	return pps_size + 2;
}

unsigned AVCDecoderConfigurationRecord2AnnexB(unsigned char* src,unsigned char** dst,unsigned* profile,unsigned* level,unsigned* nal_size,unsigned max_annexb_size)
{
	decltype(src) ps = src;
	if (ps[0] > 1) //configurationVersion
		return 0;

	int pf = ps[1]; //AVCProfileIndication
	if (pf <= 0)
		return 0;
	if (profile)
		*profile = (unsigned)pf;

	//ps[2] = profile_compatibility;

	int lv = ps[3]; //AVCLevelIndication
	if (level)
		*level = (unsigned)lv;

	int nsize = (ps[4] & 0x03) + 1; //lengthSizeMinusOne
	if (nsize < 2)
		return 0;
	if (nal_size)
		*nal_size = nsize;

	int sps_count = ps[5] & 0x1F; //numOfSequenceParameterSets
	if (sps_count <= 0)
		return 0;

	unsigned char* p = &ps[6];

	unsigned annexb_size = max_annexb_size > 128 ? max_annexb_size:1024;
	auto pdst = (unsigned char*)
		malloc(annexb_size);

	unsigned char* sps = pdst;
	unsigned sps_size = 0;
	for (int i = 0;i < sps_count;i++)
	{
		unsigned short len = *(unsigned short*)p; //sequenceParameterSetLength
		len = ((len & 0xFF00) >> 8) | ((len & 0x00FF) << 8);
		p += 2;

		if (len == 0)
			continue;

		if (((unsigned)len + 3) > (annexb_size - sps_size))
			break;

		sps[0] = 0;
		sps[1] = 0;
		sps[2] = 1;

		memcpy(&sps[3],p,len);
		p += len;

		sps_size = sps_size + 3 + len;
		sps = sps + 3 + len;
	}

	int pps_count = *p & 0x1F; //numOfPictureParameterSets
	if (pps_count <= 0)
		return 0;
	p++;

	unsigned char* pps = sps;
	unsigned pps_size = 0;
	for (int i = 0;i < pps_count;i++)
	{
		unsigned short len = *(unsigned short*)p; //pictureParameterSetLength
		len = ((len & 0xFF00) >> 8) | ((len & 0x00FF) << 8);
		p += 2;

		if (len == 0)
			continue;

		if (((unsigned)len + 3) > (annexb_size - (pps_size + sps_size)))
			break;

		pps[0] = 0;
		pps[1] = 0;
		pps[2] = 1;

		memcpy(&pps[3],p,len);
		p += len;

		pps_size = pps_size + 3 + len;
		pps = pps + 3 + len;
	}

	*dst = pdst;
	return sps_size + pps_size;
}