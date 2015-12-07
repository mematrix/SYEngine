/*
 - H264 BitStream AnnexB Parser -

 - Author: K.F Yang
 - Date: 2014-10-31
*/

#ifndef __H264_ANNEXB_PARSER_H
#define __H264_ANNEXB_PARSER_H

#include <malloc.h>
#include <memory.h>
#include "H264Spec.h"

enum H264_NALU_TYPE
{
	H264_NALU_TYPE_UNSPECIFIED = 0,
	H264_NALU_TYPE_SLICE_NON_IDR,
	H264_NALU_TYPE_SLICE_DPA,
	H264_NALU_TYPE_SLICE_DPB,
	H264_NALU_TYPE_SLICE_DPC,
	H264_NALU_TYPE_SLICE_IDR,
	H264_NALU_TYPE_SLICE_SEI,
	H264_NALU_TYPE_SLICE_SPS,
	H264_NALU_TYPE_SLICE_PPS,
	H264_NALU_TYPE_SLICE_AUD,
	H264_NALU_TYPE_END_OF_SEQUENCE,
	H264_NALU_TYPE_END_OF_STREAM,
	H264_NALU_TYPE_FILLER,
	H264_NALU_TYPE_SPS_EXT,
	H264_NALU_TYPE_RESERVED0
};

class H264AnnexBParser final
{
public:
	H264AnnexBParser() throw() : m_nal_data(nullptr) {}
	explicit H264AnnexBParser(const H264AnnexBParser& r) throw() : m_nal_data(nullptr)
	{
		InitFromStartCode(r.GetCopiedPointer(),r.GetCopiedLength());
	}

	~H264AnnexBParser() throw()
	{
		if (m_nal_data)
			free(m_nal_data);
	}

public:
	bool InitFromStartCode(unsigned char* pb,unsigned buf_len) throw();

	H264_NALU_TYPE GetCurrentNaluType() const throw() { return m_cur_type; }
	bool IsCurrentRefFrame() const throw() { return m_cur_ref_frame; }
	bool IsCurrentForbidden() const throw() { return m_cur_err_flag; }
	unsigned GetCurrentNalHead() const throw() { return m_cur_nal_head; }
	unsigned GetCurrentDataSize() const throw() { return m_next_pos_offset - m_start_pos_offset; }
	unsigned char* GetCurrentDataPointer() const throw() { return m_nal_data + m_start_pos_offset; }

	unsigned GetCurrentOffset() const throw() { return m_start_pos_offset - m_cur_sc_size - NALU_HEADER_BYTES; }

	bool ReadNext();
	bool IsParseEOF() const throw() { return m_eof_flag; }

	unsigned char* GetCopiedPointer() const throw()
	{
		return m_nal_data;
	}
	unsigned GetCopiedLength() const throw()
	{
		return m_nal_data_size;
	}

private:
	bool ParseAndInitNaluData();
	unsigned FindNextNaluOffset();

private:
	unsigned char* m_nal_data;
	unsigned m_nal_data_size;

	bool m_eof_flag;

	H264_NALU_TYPE m_cur_type;
	bool m_cur_ref_frame;
	bool m_cur_err_flag;
	unsigned m_cur_sc_size;
	unsigned m_cur_nal_head;

	unsigned m_start_pos_offset;
	unsigned m_next_pos_offset;
};

#endif //__H264_ANNEXB_PARSER_H