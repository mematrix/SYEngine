#include "Aka4Splitter.h"

static const char GlobalAppleMovieLangCode[][4] = {
	"eng","fra","ger","ita","dut","sve","spa","dan","por","nor",
	"heb","jpn","ara","fin","gre","ice","mlt","tur","hr ","chi",
	"urd","hin","tha","kor","lit","pol","hun","est","lav","und",
	"fo ","und","rus","chi","und","iri","alb","ron","ces","slk",
	"slv","yid","sr ","mac","bul","ukr","bel","uzb","kaz","aze",
	"aze","arm","geo","mol","kir","tgk","tuk","mon","und","pus",
	"kur","kas","snd","tib","nep","san","mar","ben","asm","guj",
	"pa ","ori","mal","kan","tam","tel","und","bur","khm","lao",
	"vie","ind","tgl","may","may","amh","tir","orm","som","swa"
};

bool MovLangId2Iso639(unsigned code, char to[4])
{
	to[0] = to[1] = to[2] = to[3] = 0;
	if (code >= 0x400 && code != 0x7fff)
	{
		for (int i = 2; i >= 0; i--) {
			to[i] = 0x60 + (code & 0x1f);
			code >>= 5;
		}
		return true;
	}

	 if (code >= ISOM_ARRAY_COUNT(GlobalAppleMovieLangCode))
		 return false;
	 if (!GlobalAppleMovieLangCode[code][0])
		 return false;
	 memcpy(to, GlobalAppleMovieLangCode[code], 4);
	 return true;
}

unsigned ReadDolbyAudioChannels(unsigned char* pb, unsigned size, bool ac3_ec3)
{
	unsigned result = 0;
	if (!ac3_ec3)
		if (size < 5)
			return 0;

	unsigned char* p = pb;
	if (!ac3_ec3)
		p += 2;

	int ac3info = 0;
	memcpy(&ac3info, p, 3);
	ac3info <<= 8;
	ac3info = ISOM_SWAP32(ac3info);

	int acmod, lfeon, bsmod;
	if (ac3_ec3)
	{
		bsmod = (ac3info >> 14) & 0x7;
		acmod = (ac3info >> 11) & 0x7;
		lfeon = (ac3info >> 10) & 0x1;
		static const unsigned nch[] = {2, 1, 2, 3, 3, 4, 4, 5};
		if (acmod < 8)
			result = nch[acmod] + lfeon;
	}else{
		bsmod = (ac3info >> 12) & 0x1F;
		acmod = (ac3info >> 9) & 0x7;
		lfeon = (ac3info >> 8) & 0x1;
	}

	return result;
}