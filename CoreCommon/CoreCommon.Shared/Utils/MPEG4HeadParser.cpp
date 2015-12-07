#include "stagefright/ABitReader.h"
#include "MPEG4HeadParser.h"
#include <stdio.h>

unsigned FindNextMPEG4StartCode(unsigned char* pb,unsigned len)
{
	if (pb == nullptr || len < 4)
		return 0;

	for (unsigned i = 0;i < (len - 4);i++)
		if (pb[i] == 0 && pb[i + 1] == 0 &&
			pb[i + 2] == 1)
			return i + 3;

	return 0;
}

void ParseMPEG4VUserData(unsigned char* pb,unsigned len,MPEG4V_PrivateData* info)
{
	info->Encoder = MPEG4V_UserData_Encoder::MP4V_Encoder_Others;
	if (*pb != MPEG4_HEAD_CODE_USERDATA)
		return;
	if (len < 7)
		return;

	unsigned flag = *(unsigned*)(pb + 1);
	if (flag == MPEG4_USERDATA_ENCODER_XVID)
		info->Encoder = MPEG4V_UserData_Encoder::MP4V_Encoder_XVID;
	else if (flag == MPEG4_USERDATA_ENCODER_DIVX)
		info->Encoder = MPEG4V_UserData_Encoder::MP4V_Encoder_DIVX;
	else if (flag == MPEG4_USERDATA_ENCODER_FFMPEG)
		info->Encoder = MPEG4V_UserData_Encoder::MP4V_Encoder_FFmpeg;
}

void ParseMPEG4VObjectLayer(unsigned char* pb,unsigned len,MPEG4V_PrivateData* info)
{
	info->bitsPerPixel = 8;
	unsigned end = FindNextMPEG4StartCode(pb,len);
	if (end == 0)
		end = len;
	else
		end -= 3;

	stagefright::ABitReader br(pb + 1,end - 1);
	unsigned video_object_layer_verid = 1; //default.

	br.skipBits(1); //random_accessible_vol
	br.skipBits(8); //video_object_type_indication
	
	if (br.getBits(1)) //is_object_layer_identifier
	{
		video_object_layer_verid = br.getBits(4);
		br.skipBits(3); //video_object_layer_priority
	}

	unsigned aspect_ratio_info = br.getBits(4);
	if (aspect_ratio_info == 0x0F)
	{
		info->parWidth = br.getBits(8);
		info->parHeight = br.getBits(8);
	}else{
		switch (aspect_ratio_info)
		{
		case 0:
		case 1:
			info->parWidth = info->parHeight = 1;
			break;
		case 2:
			info->parWidth = 12;
			info->parHeight = 11;
			break;
		case 3:
			info->parWidth = 10;
			info->parHeight = 11;
			break;
		case 4:
			info->parWidth = 16;
			info->parHeight = 11;
			break;
		case 5:
			info->parWidth = 40;
			info->parHeight = 33;
			break;
		}
	}

	if (br.getBits(1)) //vol_control_parameters
	{
		info->chromaFormat = br.getBits(2);
		info->lowDelay = (int)br.getBits(1);
		if (br.getBits(1)) //vbv_parameters
			br.skipBits(79);
	}

	unsigned video_object_layer_shape = br.getBits(2);
	if (video_object_layer_shape == 3 && video_object_layer_verid != 1)
		br.skipBits(4); //video_object_layer_shape_extension

	br.skipBits(1); //mark
	info->vop_time_increment_resolution = br.getBits(16);
	br.skipBits(1); //mark

	int timeSize = 0;
	int p2w = 1;
	for (;timeSize <= 16;timeSize++)
	{
		if (info->vop_time_increment_resolution < p2w)
			break;
		p2w <<= 1;
	}

	if (br.getBits(1)) //fixed_vop_rate
		info->fixed_vop_time_increment = br.getBits(timeSize);

	if (video_object_layer_shape != 2)
	{
		if (video_object_layer_shape == 0) //rect
		{
			br.skipBits(1); //mark
			info->width = br.getBits(13);
			br.skipBits(1); //mark
			info->height = br.getBits(13);
			br.skipBits(1); //mark
		}

		info->interlaced = br.getBits(1);
		br.skipBits(1); //obmc_disable
		unsigned sprite_enable = br.getBits(video_object_layer_verid == 1 ? 1:2);
		if (sprite_enable == 1 || sprite_enable == 2)
		{
			if (sprite_enable == 1)
			{
				br.skipBits(13); //sprite_width
				br.skipBits(1); //mark
				br.skipBits(13); //sprite_height
				br.skipBits(1); //mark
				br.skipBits(13); //sprite_top_coordinate
				br.skipBits(1); //mark
				br.skipBits(13); //sprite_left_coordinate
				br.skipBits(1); //mark
			}

			br.skipBits(6); //no_of_sprite_warping_points
			br.skipBits(2); //sprite_warping_accuracy
			br.skipBits(1); //sprite_brightness_change
			if (sprite_enable == 1)
				br.skipBits(1); //low_latency_sprite_enable
		}

		if (video_object_layer_verid != 1 && video_object_layer_shape != 0)
		{
			br.skipBits(1); //sadct_disable
			if (br.getBits(1)) //bits_per_pixel_not_8_bit
			{
				br.skipBits(4); //quant_precision
				info->bitsPerPixel = br.getBits(4);
			}
		}

		if (video_object_layer_shape == 3)
			br.skipBits(3);
		info->quant_type = br.getBits(1);
	}
}

bool ParseMPEG4VPrivateData(unsigned char*pb,unsigned len,MPEG4V_PrivateData* info)
{
	if (pb == nullptr || len < 8 || info == nullptr)
		return false;

	decltype(pb) p = pb;
	unsigned size = len;
	for (int i = 0;i < 100;i++)
	{
		unsigned offset = FindNextMPEG4StartCode(p,size);
		if (offset == 0)
			break;

		p += offset;
		size -= offset;

		if (*p == MPEG4_HEAD_CODE_VOS)
			printf("TODO...\n");
		else if (*p == MPEG4_HEAD_CODE_USERDATA)
			ParseMPEG4VUserData(p,size,info);
		else if ((*p >= 0x20) && (*p <= 0x2F))
			ParseMPEG4VObjectLayer(p,size,info);
	}

	return true;
}