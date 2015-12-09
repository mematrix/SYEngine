#include "TTA1Help.h"
#include <memory.h>

void BuildTTA1FileHeader(unsigned nch,unsigned bits,unsigned rate,double duration,TTA1_FILE_HEADER* head)
{
	memset(head,0,sizeof(*head));
	head->fourcc = 0x31415454;
	head->format = 1;
	head->nch = nch;
	head->bits = bits;
	head->rate = rate;
	head->samples = (unsigned)(duration * (double)rate);
}