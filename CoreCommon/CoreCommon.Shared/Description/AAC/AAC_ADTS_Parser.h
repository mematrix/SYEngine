#ifndef __AAC_ADTS_PARSER_H
#define __AAC_ADTS_PARSER_H

bool ADTSHeaderParse(unsigned char* ph,int* profile,int* nch,int* srate);

#endif //__AAC_ADTS_PARSER_H