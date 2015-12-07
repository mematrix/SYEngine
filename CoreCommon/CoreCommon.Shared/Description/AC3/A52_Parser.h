#ifndef __A52_PARSER_H
#define __A52_PARSER_H

bool AC3ParseFrameInfo(unsigned char* pb,int* srate,int* brate,int* flags);
int GetAC3ChannelsFromFlags(int flags);

#endif //__A52_PARSER_H