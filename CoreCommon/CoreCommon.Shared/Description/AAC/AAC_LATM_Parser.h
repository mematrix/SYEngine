#ifndef __AAC_LATM_PARSER_H
#define __AAC_LATM_PARSER_H

bool LATMHeaderParse(unsigned char* ph,int* profile,int* nch,int* srate,int* he_lc_core);

#endif //__AAC_LATM_PARSER_H