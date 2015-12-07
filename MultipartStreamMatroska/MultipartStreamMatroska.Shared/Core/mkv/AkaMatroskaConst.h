#ifndef __AKA_MATROSKA_CONST_H
#define __AKA_MATROSKA_CONST_H

#define __DECLARE_EID_(name, x) static const unsigned (name) = (x)

namespace AkaMatroska {
namespace Consts {

	namespace Ebml
	{
		__DECLARE_EID_(Root_Header, 0x1A45DFA3);
		__DECLARE_EID_(Element_Verson, 0x4286);
		__DECLARE_EID_(Element_ReadVersion, 0x42F7);
		__DECLARE_EID_(Element_MaxIdLength, 0x42F2);
		__DECLARE_EID_(Element_MaxSizeLength, 0x42F3);
		__DECLARE_EID_(Element_DocType, 0x4282);
		__DECLARE_EID_(Element_DocType_Version, 0x4287);
		__DECLARE_EID_(Element_DocType_ReadVersion, 0x4285);

		__DECLARE_EID_(Global_Void, 0xEC);
		__DECLARE_EID_(Global_Crc32, 0xBF);
	}

	namespace Matroska
	{
		__DECLARE_EID_(Root_Segment, 0x18538067); //Root

		__DECLARE_EID_(Master_SeekHead, 0x114D9B74);
		__DECLARE_EID_(Master_Information, 0x1549A966);
		__DECLARE_EID_(Master_Tracks, 0x1654AE6B);
		__DECLARE_EID_(Master_Cluster, 0x1F43B675);
		__DECLARE_EID_(Master_Cues, 0x1C53BB6B);
		__DECLARE_EID_(Master_Chapters, 0x1043A770);
		__DECLARE_EID_(Master_Tags, 0x1254C367);
		__DECLARE_EID_(Master_Attachments, 0x1941A469);

		namespace SeekHead
		{
			__DECLARE_EID_(Master_Seek_Entry, 0x4DBB);
			__DECLARE_EID_(Element_SeekId, 0x53AB);
			__DECLARE_EID_(Element_SeekPos, 0x53AC);
		}
		namespace Information
		{
			__DECLARE_EID_(Element_UID, 0x73A4);
			__DECLARE_EID_(Element_FileName, 0x7384);
			__DECLARE_EID_(Element_TimecodeScale, 0x2AD7B1);
			__DECLARE_EID_(Element_Duration, 0x4489);
			__DECLARE_EID_(Element_Title, 0x7BA9);
			__DECLARE_EID_(Element_MuxingApp, 0x4D80);
			__DECLARE_EID_(Element_WritingApp, 0x5741);
			__DECLARE_EID_(Element_DateUTC, 0x4461);
		}
		namespace Tracks
		{
			__DECLARE_EID_(Master_Track_Entry, 0xAE);
			__DECLARE_EID_(Element_TrackNumber, 0xD7);
			__DECLARE_EID_(Element_TrackUID, 0x73C5);
			__DECLARE_EID_(Element_TrackType, 0x83);
			__DECLARE_EID_(Element_TrackOverlay, 0x6FAB);
			__DECLARE_EID_(Element_FlagEnabled, 0xB9);
			__DECLARE_EID_(Element_FlagDefault, 0x88);
			__DECLARE_EID_(Element_FlagForced, 0x55AA);
			__DECLARE_EID_(Element_FlagLacing, 0x9C);
			__DECLARE_EID_(Element_MinCache, 0x6DE7);
			__DECLARE_EID_(Element_MaxCache, 0x6DF8);
			__DECLARE_EID_(Element_DefaultDuration, 0x23E383);
			__DECLARE_EID_(Element_DefaultDecodedFieldDuration, 0x234E7A);

			__DECLARE_EID_(Element_MaxBlockAdditionID, 0x55EE);
			__DECLARE_EID_(Element_Name, 0x536E);
			__DECLARE_EID_(Element_Language, 0x22B59C);
			__DECLARE_EID_(Element_AttachmentLink, 0x7446);

			__DECLARE_EID_(Element_CodecId, 0x86);
			__DECLARE_EID_(Element_CodecPrivate, 0x63A2);
			__DECLARE_EID_(Element_CodecName, 0x258688);
			__DECLARE_EID_(Element_CodecDecodeAll, 0xAA);
			__DECLARE_EID_(Element_CodecDelay, 0x56AA);
			__DECLARE_EID_(Element_SeekPreroll, 0x56BB);

			__DECLARE_EID_(Master_Video, 0xE0);
			__DECLARE_EID_(Master_Audio, 0xE1);
			__DECLARE_EID_(Master_Operation, 0xE2);
			__DECLARE_EID_(Master_ContentEncodings, 0x6D80);

			namespace Audio
			{
				__DECLARE_EID_(Element_SamplingFrequency, 0xB5);
				__DECLARE_EID_(Element_OutputSamplingFrequency, 0x78B5);
				__DECLARE_EID_(Element_Channels, 0x9F);
				__DECLARE_EID_(Element_BitDepth, 0x6264);
			}
			namespace Video
			{
				__DECLARE_EID_(Element_FrameRate, 0x2383E3);
				__DECLARE_EID_(Element_AspectRatio, 0x54B3);
				__DECLARE_EID_(Element_DisplayWidth, 0x54B0);
				__DECLARE_EID_(Element_DisplayHeight, 0x54BA);
				__DECLARE_EID_(Element_DisplayUnit, 0x54B2);
				__DECLARE_EID_(Element_PixelWidth, 0xB0);
				__DECLARE_EID_(Element_PixelHeight, 0xBA);
				__DECLARE_EID_(Element_AlphaMode, 0x53C0);
				__DECLARE_EID_(Element_StereoMode, 0x53B8);
				__DECLARE_EID_(Element_ColourSpace, 0x2EB524);
				__DECLARE_EID_(Element_FlagInterlaced, 0x9A);
				__DECLARE_EID_(Element_PixelCropBottom, 0x54AA);
				__DECLARE_EID_(Element_PixelCropTop, 0x54BB);
				__DECLARE_EID_(Element_PixelCropLeft, 0x54CC);
				__DECLARE_EID_(Element_PixelCropRight, 0x54DD);
			}
			namespace Operation
			{
				__DECLARE_EID_(Master_CombinePlanes, 0xE3);
				__DECLARE_EID_(Master_Plane, 0xE4);
				__DECLARE_EID_(Element_PlaneUID, 0xE5);
				__DECLARE_EID_(Element_PlaneType, 0xE6);

				__DECLARE_EID_(Master_JoinBlocks, 0xE9);
				__DECLARE_EID_(Element_JoinUID, 0xED);
			}
			namespace ContentEncodings
			{
				__DECLARE_EID_(Master_ContentEncoding, 0x6240);
				__DECLARE_EID_(Element_ContentEncodingOrder, 0x5031);
				__DECLARE_EID_(Element_ContentEncodingScope, 0x5032);
				__DECLARE_EID_(Element_ContentEncodingType, 0x5033);

				__DECLARE_EID_(Master_ContentCompression, 0x5034);
				__DECLARE_EID_(Element_ContentCompAlgo, 0x4254);
				__DECLARE_EID_(Element_ContentCompSettings, 0x4255);

				__DECLARE_EID_(Master_ContentEncryption, 0x5035);
				__DECLARE_EID_(Element_ContentEncAlgo, 0x47E1);
				__DECLARE_EID_(Element_ContentEncKeyID, 0x47E2);
				__DECLARE_EID_(Element_ContentSignature, 0x47E3);
				__DECLARE_EID_(Element_ContentSigKeyID, 0x47E4);
				__DECLARE_EID_(Element_ContentSigAlgo, 0x47E5);
				__DECLARE_EID_(Element_ContentSigHashAlgo, 0x47E6);
			}
		}
		namespace Cluster
		{
			__DECLARE_EID_(Element_SimpleBlock, 0xA3);
			__DECLARE_EID_(Element_Timecode, 0xE7);
			__DECLARE_EID_(Element_Position, 0xA7);
			__DECLARE_EID_(Element_PrevSize, 0xAB);

			__DECLARE_EID_(Master_SilentTracks, 0x5854);
			__DECLARE_EID_(Element_SilentTrackNumber, 0x58D7);

			__DECLARE_EID_(Master_BlockGroup, 0xA0);
			__DECLARE_EID_(Element_Block, 0xA1);
			__DECLARE_EID_(Element_BlockDuration, 0x9B);
			__DECLARE_EID_(Element_ReferencePriority, 0xFA);
			__DECLARE_EID_(Element_ReferenceBlock, 0xFB);
			__DECLARE_EID_(Element_CodecState, 0xA4);
			__DECLARE_EID_(Element_DiscardPadding, 0x75A2);

			__DECLARE_EID_(Master_BlockAdditions, 0x75A1);
			__DECLARE_EID_(Master_BlockMore, 0xA6);
			__DECLARE_EID_(Element_BlockAddID, 0xEE);
			__DECLARE_EID_(Element_BlockAdditional, 0xA5);
		}
		namespace Cues
		{
			__DECLARE_EID_(Master_CuePoint_Entry, 0xBB);
			__DECLARE_EID_(Element_CueTime, 0xB3);

			__DECLARE_EID_(Master_CueTrackPositions, 0xB7);
			__DECLARE_EID_(Element_CueTrack, 0xF7);
			__DECLARE_EID_(Element_CueClusterPosition, 0xF1);
			__DECLARE_EID_(Element_CueRelativePosition, 0xF0);
			__DECLARE_EID_(Element_CueDuration, 0xB2);
			__DECLARE_EID_(Element_CueBlockNumber, 0x5378);
			__DECLARE_EID_(Element_CueCodecState, 0xEA);

			__DECLARE_EID_(Master_CueReference, 0xDB);
			__DECLARE_EID_(Element_CueRefTime, 0x96);
		}
		namespace Chapters
		{
			__DECLARE_EID_(Master_Edition_Entry, 0x45B9);
			__DECLARE_EID_(Element_EditionUID, 0x45BC);
			__DECLARE_EID_(Element_EditionFlagHidden, 0x45BD);
			__DECLARE_EID_(Element_EditionFlagDefault, 0x45DB);
			__DECLARE_EID_(Element_EditionFlagOrdered, 0x45DD);

			__DECLARE_EID_(Master_ChapterAtom, 0xB6);
			__DECLARE_EID_(Element_ChapterUID, 0x73C4);
			__DECLARE_EID_(Element_ChapterStringUID, 0x5654);
			__DECLARE_EID_(Element_ChapterTimeStart, 0x91);
			__DECLARE_EID_(Element_ChapterTimeEnd, 0x92);
			__DECLARE_EID_(Element_ChapterFlagHidden, 0x98);
			__DECLARE_EID_(Element_ChapterFlagEnabled, 0x4598);
			__DECLARE_EID_(Element_ChapterSegmentUID, 0x6E67);

			__DECLARE_EID_(Master_ChapterTrack, 0x8F);
			__DECLARE_EID_(Element_ChapterTrackNumber, 0x89);

			__DECLARE_EID_(Master_ChapterDisplay, 0x80);
			__DECLARE_EID_(Element_ChapString, 0x85);
			__DECLARE_EID_(Element_ChapLanguage, 0x437C);
			__DECLARE_EID_(Element_ChapCountry, 0x437E);
		}
		namespace Tags
		{
			__DECLARE_EID_(Master_Tag_Entry, 0x7373);
			__DECLARE_EID_(Master_Tag_Targets, 0x63C0);
			__DECLARE_EID_(Element_TargetTypeValue, 0x68CA);
			__DECLARE_EID_(Element_TargetType, 0x63CA);
			__DECLARE_EID_(Element_TagTrackUID, 0x63C5);
			__DECLARE_EID_(Element_TagEditionUID, 0x63C9);
			__DECLARE_EID_(Element_TagChapterUID, 0x63C4);
			__DECLARE_EID_(Element_TagAttachmentUID, 0x63C6);

			__DECLARE_EID_(Master_SimpleTag_Entry, 0x67C8);
			__DECLARE_EID_(Element_TagName, 0x45A3);
			__DECLARE_EID_(Element_TagLanguage, 0x447A);
			__DECLARE_EID_(Element_TagDefault, 0x4484);
			__DECLARE_EID_(Element_TagString, 0x4487);
			__DECLARE_EID_(Element_TagBinary, 0x4485);
		}
		namespace Attachments
		{
			__DECLARE_EID_(Master_AttachedFile_Entry, 0x61A7);
			__DECLARE_EID_(Element_FileDescription, 0x467E);
			__DECLARE_EID_(Element_FileName, 0x466E);
			__DECLARE_EID_(Element_FileMimeType, 0x4660);
			__DECLARE_EID_(Element_FileData, 0x465C);
			__DECLARE_EID_(Element_FileUID, 0x46AE);
		}
	}

	namespace Container
	{
		__DECLARE_EID_(TrackType_Video, 0x01);
		__DECLARE_EID_(TrackType_Audio, 0x02);
		__DECLARE_EID_(TrackType_Complex, 0x03);
		__DECLARE_EID_(TrackType_Logo, 0x10);
		__DECLARE_EID_(TrackType_Subtitle, 0x11);
		__DECLARE_EID_(TrackType_Button, 0x12);
		__DECLARE_EID_(TrackType_Control, 0x20);

		__DECLARE_EID_(TrackEncodingScope_AllFrames, 1);
		__DECLARE_EID_(TrackEncodingScope_CodecPrivate, 2);
		__DECLARE_EID_(TrackEncodingScope_Full, 3);

		__DECLARE_EID_(TrackEncodingType_Compression, 0);
		__DECLARE_EID_(TrackEncodingType_Encryption, 1);

		__DECLARE_EID_(TrackCompressType_ZLib, 0);
		__DECLARE_EID_(TrackCompressType_BZLib, 1);
		__DECLARE_EID_(TrackCompressType_Lzo, 2);
		__DECLARE_EID_(TrackCompressType_HeadStrip, 3);

		__DECLARE_EID_(TrackEncAlgoType_DES, 1);
		__DECLARE_EID_(TrackEncAlgoType_3DES, 2);
		__DECLARE_EID_(TrackEncAlgoType_Twofish, 3);
		__DECLARE_EID_(TrackEncAlgoType_Blowfish, 4);
		__DECLARE_EID_(TrackEncAlgoType_AES, 5);

		__DECLARE_EID_(BlockLacingType_None, 0);
		__DECLARE_EID_(BlockLacingType_Xiph, 1);
		__DECLARE_EID_(BlockLacingType_Fixed, 2);
		__DECLARE_EID_(BlockLacingType_Ebml, 3);
	}
}}

#undef __DECLARE_EID_
#endif //__AKA_MATROSKA_CONST_H