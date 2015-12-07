unsigned short MakeAACAudioSpecificConfig(unsigned audioObjectType,unsigned samplingFrequency,unsigned channelConfiguration);
int ParseMKVOggXiphHeader(unsigned char* buf,unsigned size,unsigned* h1size,unsigned* h2size,unsigned* h3size);
int MemorySearch(const unsigned char* buf,unsigned bufSize,const unsigned char* search,unsigned searchSize);