#ifndef __MEDIA_MEMORY_H
#define __MEDIA_MEMORY_H

#include "MediaPacket.h"

void* AllocMediaMemory(unsigned size);
void* ReallocMediaMemory(void* p,unsigned size);
void FreeMediaMemory(void* p);

bool InitMediaBuffer(AVMediaBuffer* buffer,unsigned size);
bool AdjustMediaBufferSize(AVMediaBuffer* buffer,unsigned new_size);
bool FreeMediaBuffer(AVMediaBuffer* buffer);
void CopyMediaMemory(AVMediaBuffer* buffer,void* copy_to,unsigned copy_to_size);

#endif //__MEDIA_MEMORY_H