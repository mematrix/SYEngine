#include "BD_LPCM_Parser.h"

bool BDAudioHeaderParse(void* ph,int* nch,int* srate,int* bits,unsigned int* ch_layout)
{
	unsigned int head = BE_TO_LE32((*(unsigned int*)ph));
	
	switch ((head & 0xF000) >> 12)
	{
	case 1:
		*nch = 1;
		*ch_layout = AUDIO_CH_LAYOUT_FRONT_CENTER;
		break;
	case 3:
		*nch = 2;
		*ch_layout = AUDIO_CH_LAYOUT_FRONT_LEFT|AUDIO_CH_LAYOUT_FRONT_RIGHT;
		break;
	case 4:
		*nch = 3;
		*ch_layout = AUDIO_CH_LAYOUT_FRONT_LEFT|AUDIO_CH_LAYOUT_FRONT_RIGHT|AUDIO_CH_LAYOUT_FRONT_CENTER;
		break;
	case 5:
		*nch = 3;
		*ch_layout = AUDIO_CH_LAYOUT_FRONT_LEFT|AUDIO_CH_LAYOUT_FRONT_RIGHT|AUDIO_CH_LAYOUT_BACK_CENTER;
		break;
	case 6:
		*nch = 4;
		*ch_layout = AUDIO_CH_LAYOUT_FRONT_LEFT|AUDIO_CH_LAYOUT_FRONT_RIGHT
			|AUDIO_CH_LAYOUT_FRONT_CENTER|AUDIO_CH_LAYOUT_BACK_CENTER;
		break;
	case 7:
		*nch = 4;
		*ch_layout = AUDIO_CH_LAYOUT_FRONT_LEFT|AUDIO_CH_LAYOUT_FRONT_RIGHT
			|AUDIO_CH_LAYOUT_BACK_LEFT|AUDIO_CH_LAYOUT_BACK_RIGHT;
		break;
	case 8:
		*nch = 5;
		*ch_layout = AUDIO_CH_LAYOUT_FRONT_LEFT|AUDIO_CH_LAYOUT_FRONT_RIGHT|AUDIO_CH_LAYOUT_FRONT_CENTER
			|AUDIO_CH_LAYOUT_BACK_LEFT|AUDIO_CH_LAYOUT_BACK_RIGHT;
		break;
	case 9:
		*nch = 6;
		*ch_layout = AUDIO_CH_LAYOUT_FRONT_LEFT|AUDIO_CH_LAYOUT_FRONT_RIGHT|AUDIO_CH_LAYOUT_FRONT_CENTER
			|AUDIO_CH_LAYOUT_BACK_LEFT|AUDIO_CH_LAYOUT_BACK_RIGHT|AUDIO_CH_LAYOUT_LOW_FREQUENCY;
		break;
	case 10:
		*nch = 7;
		*ch_layout = AUDIO_CH_LAYOUT_FRONT_LEFT|AUDIO_CH_LAYOUT_FRONT_RIGHT|AUDIO_CH_LAYOUT_FRONT_CENTER
			|AUDIO_CH_LAYOUT_BACK_LEFT|AUDIO_CH_LAYOUT_BACK_RIGHT
			|AUDIO_CH_LAYOUT_SIDE_LEFT|AUDIO_CH_LAYOUT_SIDE_RIGHT;
		break;
	case 11:
		*nch = 8;
		*ch_layout = AUDIO_CH_LAYOUT_FRONT_LEFT|AUDIO_CH_LAYOUT_FRONT_RIGHT|AUDIO_CH_LAYOUT_FRONT_CENTER
			|AUDIO_CH_LAYOUT_BACK_LEFT|AUDIO_CH_LAYOUT_BACK_RIGHT
			|AUDIO_CH_LAYOUT_SIDE_LEFT|AUDIO_CH_LAYOUT_SIDE_RIGHT|AUDIO_CH_LAYOUT_LOW_FREQUENCY;
		break;
	default:
		return false;
	}

	switch ((head >> 6) & 3)
	{
	case 1:
		*bits = 16;
		break;
	case 2:
	case 3:
		*bits = 24;
		break;
	default:
		return false;
	}

	switch ((head >> 8) & 15)
	{
	case 1:
		*srate = 48000;
		break;
	case 4:
		*srate = 96000;
		break;
	case 5:
		*srate = 192000;
		break;
	default:
		return false;
	}

	return true;
}