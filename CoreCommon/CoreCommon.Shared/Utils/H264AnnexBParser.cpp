/*
 - H264 BitStream AnnexB Parser -

 - Author: K.F Yang
 - Date: 2014-10-31
*/

#include "H264AnnexBParser.h"

bool H264AnnexBParser::InitFromStartCode(unsigned char* pb,unsigned buf_len)
{
	if (pb == nullptr)
		return false;
	if (buf_len == 0)
		return false;

	m_cur_sc_size = 4;
	if ((*(unsigned*)pb) != H264_ANNEXB_START_CODE_1)
	{
		if (((*(unsigned*)pb) & H264_ANNEXB_3TO4BYTES_AND) != H264_ANNEXB_START_CODE_0)
			return false;
		else
			m_cur_sc_size = 3;
	}

	if (m_nal_data)
		free(m_nal_data);

	unsigned aligend_len = ((buf_len >> 2) << 2) + 16; //4 bytes aligend.
	m_nal_data = (unsigned char*)malloc(aligend_len);

	memcpy(m_nal_data,pb,buf_len);
	m_nal_data_size = buf_len;

	m_start_pos_offset = m_next_pos_offset = 0;
	m_start_pos_offset += m_cur_sc_size;

	m_eof_flag = false;
	return ParseAndInitNaluData();
}

bool H264AnnexBParser::ReadNext()
{
	if (!m_eof_flag)
		m_start_pos_offset = m_next_pos_offset;
	else
		m_start_pos_offset = m_next_pos_offset = 0;

	unsigned flag = *(unsigned*)(m_nal_data + m_start_pos_offset);
	if (flag == H264_ANNEXB_START_CODE_1)
		m_cur_sc_size = 4;
	else
		m_cur_sc_size = 3;

	m_start_pos_offset += m_cur_sc_size;

	m_eof_flag = false;
	return ParseAndInitNaluData();
}

bool H264AnnexBParser::ParseAndInitNaluData()
{
	m_cur_type = H264_NALU_TYPE_UNSPECIFIED;
	m_cur_err_flag = m_cur_ref_frame = false;

	unsigned char nal_head = m_nal_data[m_start_pos_offset];
	m_start_pos_offset++;

	if (nal_head == 0)
		return false;

	m_cur_nal_head = nal_head;

	unsigned forbidden_zero_bit = (unsigned)((nal_head & 0x80) >> 7);
	if (forbidden_zero_bit != 0)
	{
		m_cur_err_flag = true;
		m_next_pos_offset = FindNextNaluOffset();
		return true;
	}

	unsigned nal_ref_idc = (unsigned)((nal_head & 0x60) >> 5);
	if (nal_ref_idc != 0)
		m_cur_ref_frame = true;

	unsigned nal_unit_type = (unsigned)(nal_head & 0x1F);
	if (nal_unit_type >= H264_NALU_TYPE_RESERVED0)
	{
		m_cur_err_flag = true;
		m_next_pos_offset = FindNextNaluOffset();
		return true;
	}

	m_cur_type = (H264_NALU_TYPE)nal_unit_type;

	m_next_pos_offset = FindNextNaluOffset();
	return true;
}

unsigned H264AnnexBParser::FindNextNaluOffset()
{
	unsigned tmp = m_start_pos_offset;

	while (1)
	{
		if (tmp >= m_nal_data_size)
		{
			m_eof_flag = true;
			return m_nal_data_size;
		}

		unsigned flag = *(unsigned*)(m_nal_data + tmp);
		if (flag == H264_ANNEXB_START_CODE_1 ||
			(flag & H264_ANNEXB_3TO4BYTES_AND) == H264_ANNEXB_START_CODE_0)
			return tmp;

		tmp++;
	}
}