/*
 - HEVC BitStream AnnexB Parser -

 - Author: K.F Yang
 - Date: 2014-11-07
*/

#ifndef __HEVC_ANNEXB_PARSER_H
#define __HEVC_ANNEXB_PARSER_H

#include <stdlib.h>
#include <memory.h>

enum HEVC_NALU_TYPE
{
	HEVC_NALU_TYPE_SLICE_TRAIL_N = 0,
	HEVC_NALU_TYPE_SLICE_TRAIL_R,
	HEVC_NALU_TYPE_SLICE_TSA_N,
	HEVC_NALU_TYPE_SLICE_TLA_R,
	HEVC_NALU_TYPE_SLICE_STSA_N,
	HEVC_NALU_TYPE_SLICE_STSA_R,
	HEVC_NALU_TYPE_SLICE_RADL_N,
	HEVC_NALU_TYPE_SLICE_RADL_R,
	HEVC_NALU_TYPE_SLICE_RASL_N,
	HEVC_NALU_TYPE_SLICE_RASL_R,
	HEVC_NALU_RESERVED_VCL_N10,
	HEVC_NALU_RESERVED_VCL_R11,
	HEVC_NALU_RESERVED_VCL_N12,
	HEVC_NALU_RESERVED_VCL_R13,
	HEVC_NALU_RESERVED_VCL_N14,
	HEVC_NALU_RESERVED_VCL_R15,
	HEVC_NALU_TYPE_SLICE_BLA_W_LP,
	HEVC_NALU_TYPE_SLICE_BLA_W_RADL,
	HEVC_NALU_TYPE_SLICE_BLA_N_LP,
	HEVC_NALU_TYPE_SLICE_IDR_W_RADL,
	HEVC_NALU_TYPE_SLICE_IDR_N_LP,
	HEVC_NALU_TYPE_SLICE_CRA,
	HEVC_NALU_TYPE_RESERVED_IRAP_VCL22,
	HEVC_NALU_TYPE_RESERVED_IRAP_VCL23,
	HEVC_NALU_TYPE_RESERVED_VCL24,
	HEVC_NALU_TYPE_RESERVED_VCL25,
	HEVC_NALU_TYPE_RESERVED_VCL26,
	HEVC_NALU_TYPE_RESERVED_VCL27,
	HEVC_NALU_TYPE_RESERVED_VCL28,
	HEVC_NALU_TYPE_RESERVED_VCL29,
	HEVC_NALU_TYPE_RESERVED_VCL30,
	HEVC_NALU_TYPE_RESERVED_VCL31,
	HEVC_NALU_TYPE_VPS,
	HEVC_NALU_TYPE_SPS,
	HEVC_NALU_TYPE_PPS,
	HEVC_NALU_TYPE_AUD,
	HEVC_NALU_TYPE_EOS,
	HEVC_NALU_TYPE_EOB,
	HEVC_NALU_TYPE_FILLER_DATA,
	HEVC_NALU_TYPE_PREFIX_SEI,
	HEVC_NALU_TYPE_SUFFIX_SEI,
	HEVC_NALU_TYPE_INVALID
};

class HEVCAnnexBParser final
{
public:
	HEVCAnnexBParser() throw() : m_nal_data(nullptr) {}
	explicit HEVCAnnexBParser(const HEVCAnnexBParser& r) throw() : m_nal_data(nullptr)
	{
		InitFromStartCode(r.GetCopiedPointer(),r.GetCopiedLength());
	}

	~HEVCAnnexBParser() throw()
	{
		if (m_nal_data)
			free(m_nal_data);
	}

public:
	bool InitFromStartCode(unsigned char* pb,unsigned buf_len) throw();

	HEVC_NALU_TYPE GetCurrentNaluType() const throw() { return m_cur_type; }
	bool IsCurrentForbidden() const throw() { return m_cur_err_flag; }
	unsigned short GetCurrentNalHead() const throw() { return m_cur_nal_head; }
	unsigned GetCurrentDataSize() const throw() { return m_next_pos_offset - m_start_pos_offset; }
	unsigned char* GetCurrentDataPointer() const throw() { return m_nal_data + m_start_pos_offset; }

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

	HEVC_NALU_TYPE m_cur_type;
	bool m_cur_err_flag;
	unsigned m_cur_sc_size;
	unsigned short m_cur_nal_head;

	unsigned m_start_pos_offset;
	unsigned m_next_pos_offset;
};

int HEVC_EBSP2RBSP(unsigned char* pb,unsigned len);

#endif //__HEVC_ANNEXB_PARSER_H