#ifndef __MPEG4_HEAD_PARSER_H
#define __MPEG4_HEAD_PARSER_H

#define MPEG4_HEAD_CODE_VOS 0xB0
#define MPEG4_HEAD_CODE_VOS_END 0xB1
#define MPEG4_HEAD_CODE_USERDATA 0xB2
#define MPEG4_HEAD_CODE_GOP 0xB3
#define MPEG4_HEAD_CODE_VSE 0xB4
#define MPEG4_HEAD_CODE_VO 0xB5
#define MPEG4_HEAD_CODE_VOP 0xB6
#define MPEG4_HEAD_CODE_SLICE 0xB7
#define MPEG4_HEAD_CODE_EXTENSION 0xB8
#define MPEG4_HEAD_CODE_FGS_VOP 0xB9
#define MPEG4_HEAD_CODE_FBA 0xBA
#define MPEG4_HEAD_CODE_FBA_PLANE 0xBB
#define MPEG4_HEAD_CODE_MESH 0xBC
#define MPEG4_HEAD_CODE_MESH_PLANE 0xBD
#define MPEG4_HEAD_CODE_STILL_TEXTURE 0xBE
#define MPEG4_HEAD_CODE_TEXTURE_SPL 0xBF
#define MPEG4_HEAD_CODE_TEXTURE_SNRL 0xC0
#define MPEG4_HEAD_CODE_TEXTURE_TILE 0xC1
#define MPEG4_HEAD_CODE_TEXTURE_SHAPE 0xC2
#define MPEG4_HEAD_CODE_STUFFING 0xC3

#define MPEG4_USERDATA_ENCODER_XVID 0x44697658 //XviD
#define MPEG4_USERDATA_ENCODER_DIVX 0x35587669 //DivX
#define MPEG4_USERDATA_ENCODER_FFMPEG 0x6376614C //Lavc

enum MPEG4V_Profile_Level
{
	MP4V_PL_Unknown,
	MP4V_PL_SimpleL1,
	MP4V_PL_SimpleL2,
	MP4V_PL_SimpleL3,
	MP4V_PL_SimpleL4,
	MP4V_PL_SimpleL5,
	MP4V_PL_SimpleL6,
	MP4V_PL_SimpleL0,
	MP4V_PL_SimpleL0b,
	MP4V_PL_SimpleScalableL0,
	MP4V_PL_SimpleScalableL1,
	MP4V_PL_SimpleScalableL2,
	MP4V_PL_CoreL1,
	MP4V_PL_CoreL2,
	MP4V_PL_MainL2,
	MP4V_PL_MainL3,
	MP4V_PL_MainL4,
	MP4V_PL_NbitL2,
	MP4V_PL_ScalableTextureL1,
	MP4V_PL_SimpleFaceAnimeL1,
	MP4V_PL_SimpleFaceAnimeL2,
	MP4V_PL_SimpleFBAL1,
	MP4V_PL_SimpleFBAL2,
	MP4V_PL_BasicAnimeTextureL1,
	MP4V_PL_BasicAnimeTextureL2,
	MP4V_PL_HybridL1,
	MP4V_PL_HybridL2,
	MP4V_PL_AdvRealTimeSimpleL1,
	MP4V_PL_AdvRealTimeSimpleL2,
	MP4V_PL_AdvRealTimeSimpleL3,
	MP4V_PL_AdvRealTimeSimpleL4,
	MP4V_PL_CoreScalableL1,
	MP4V_PL_CoreScalableL2,
	MP4V_PL_CoreScalableL3,
	MP4V_PL_AVC,
	MP4V_PL_FGS, //Fine Granularity Scalable
	MP4V_PL_ACEL1, //Advanced Coding Efficiency
	MP4V_PL_ACEL2,
	MP4V_PL_ACEL3,
	MP4V_PL_ACEL4,
	MP4V_PL_AdvCoreL1,
	MP4V_PL_AdvCoreL2,
	MP4V_PL_AdvScalableTextureL1,
	MP4V_PL_AdvScalableTextureL2,
	MP4V_PL_AdvScalableTextureL3,
	MP4V_PL_SimpleStudioL1,
	MP4V_PL_SimpleStudioL2,
	MP4V_PL_SimpleStudioL3,
	MP4V_PL_SimpleStudioL4,
	MP4V_PL_CoreStudioL1,
	MP4V_PL_CoreStudioL2,
	MP4V_PL_CoreStudioL3,
	MP4V_PL_CoreStudioL4,
	MP4V_PL_AdvSimpleL0,
	MP4V_PL_AdvSimpleL1,
	MP4V_PL_AdvSimpleL2,
	MP4V_PL_AdvSimpleL3,
	MP4V_PL_AdvSimpleL3b,
	MP4V_PL_AdvSimpleL4,
	MP4V_PL_AdvSimpleL5
};

enum MPEG4V_UserData_Encoder
{
	MP4V_Encoder_XVID,
	MP4V_Encoder_DIVX,
	MP4V_Encoder_FFmpeg,
	MP4V_Encoder_Others
};

struct MPEG4V_PrivateData
{
	MPEG4V_UserData_Encoder Encoder;
	MPEG4V_Profile_Level ProfileLevel;
	unsigned parWidth, parHeight;
	int chromaFormat; //1 = 420
	int lowDelay;
	unsigned width, height;
	unsigned interlaced;
	unsigned bitsPerPixel; //default to 8 (!= rect can to not 8)
	int quant_type; //h263 is 1
	int vop_time_increment_resolution; //unknown is 0
	int fixed_vop_time_increment; //unknown is 0
	//fps = vop_time_increment_resolution / fixed_vop_time_increment.(float)
};

unsigned FindNextMPEG4StartCode(unsigned char* pb,unsigned len);
bool ParseMPEG4VPrivateData(unsigned char*pb,unsigned len,MPEG4V_PrivateData* info);

#endif //__MPEG4_HEAD_PARSER_H