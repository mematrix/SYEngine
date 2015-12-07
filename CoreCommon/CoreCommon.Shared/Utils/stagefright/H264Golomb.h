/*
 * Exp-golomb code Parser
 * Author: K.F Yang
*/

#ifndef H264GOLOMB_H_
#define H264GOLOMB_H_

#include "ABitReader.h"

namespace stagefright {
namespace H264Golomb {

static inline unsigned H264U(stagefright::ABitReader& br,unsigned bits)
{
	return br.getBits(bits);
}

static unsigned H264UE(stagefright::ABitReader& br)
{
	unsigned numZero = 0;
	while (br.getBits(1) == 0)
		++numZero;

	unsigned x = br.getBits(numZero);
	return x + (1U << numZero) - 1;
}

static int H264SE(stagefright::ABitReader& br)
{
	unsigned u = H264UE(br);
	if ((u & 0x01) == 0)
	{
		u >>= 1;
		int ret = 0 - u;
		return ret;
	}

	return (u + 1) >> 1;
}

}}

#endif