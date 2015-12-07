#ifndef __BD_LPCM_PARSER_H
#define __BD_LPCM_PARSER_H

#include "AVDescription_CH_Layout.h"

#ifndef BE_TO_LE32
#define BE_TO_LE32(x) ((((x) & 0xFF000000) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | (((x) & 0x000000FF) << 24))
#endif

bool BDAudioHeaderParse(void* ph,int* nch,int* srate,int* bits,unsigned int* ch_layout);

#endif //__BD_LPCM_PARSER_H