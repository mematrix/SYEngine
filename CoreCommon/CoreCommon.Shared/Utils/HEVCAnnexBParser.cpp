/*
 - HEVC BitStream AnnexB Parser -

 - Author: K.F Yang
 - Date: 2014-11-07
*/

#include "HEVCSpec.h"
#include "HEVCAnnexBParser.h"

bool HEVCAnnexBParser::InitFromStartCode(unsigned char* pb,unsigned buf_len) throw()
{
	if (pb == nullptr)
		return false;
	if (buf_len == 0)
		return false;

	m_cur_sc_size = 4;
	if ((*(unsigned*)pb) != HEVC_ANNEXB_START_CODE_1)
	{
		if (((*(unsigned*)pb) & HEVC_ANNEXB_3TO4BYTES_AND) != HEVC_ANNEXB_START_CODE_0)
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

bool HEVCAnnexBParser::ReadNext()
{
	if (!m_eof_flag)
		m_start_pos_offset = m_next_pos_offset;
	else
		m_start_pos_offset = m_next_pos_offset = 0;

	unsigned flag = *(unsigned*)(m_nal_data + m_start_pos_offset);
	if (flag == HEVC_ANNEXB_START_CODE_1)
		m_cur_sc_size = 4;
	else
		m_cur_sc_size = 3;

	m_start_pos_offset += m_cur_sc_size;

	m_eof_flag = false;
	return ParseAndInitNaluData();
}

bool HEVCAnnexBParser::ParseAndInitNaluData()
{
	m_cur_type = HEVC_NALU_TYPE_INVALID;
	m_cur_err_flag = false;
	
	unsigned char nal_head = m_nal_data[m_start_pos_offset];

	unsigned short m_cur_nal_head = *(unsigned short*)(m_nal_data + m_start_pos_offset);
	m_start_pos_offset += 2; //HEVC的NAL头大小为16bit

	if (m_cur_nal_head == 0)
		return false;

	unsigned forbidden_zero_bit = (unsigned)((nal_head & 0x80) >> 7);
	if (forbidden_zero_bit != 0)
	{
		m_cur_err_flag = true;
		m_next_pos_offset = FindNextNaluOffset();
		return true;
	}

	unsigned nal_unit_type = (unsigned)((nal_head & 0x7E) >> 1);
	if (nal_unit_type >= HEVC_NALU_TYPE_INVALID)
	{
		m_cur_err_flag = true;
		m_next_pos_offset = FindNextNaluOffset();
		return true;
	}

	m_cur_type = (HEVC_NALU_TYPE)nal_unit_type;

	m_next_pos_offset = FindNextNaluOffset();
	return true;
}

unsigned HEVCAnnexBParser::FindNextNaluOffset()
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
		if (flag == HEVC_ANNEXB_START_CODE_1 ||
			(flag & HEVC_ANNEXB_3TO4BYTES_AND) == HEVC_ANNEXB_START_CODE_0)
			return tmp;

		tmp++;
	}
}

/////////// HEVC_EBSP2RBSP ///////////
int HEVC_EBSP2RBSP(unsigned char* pb,unsigned len)
{
	if (pb == nullptr ||
		len == 0)
		return -1;

	unsigned aligned_size = len / 4 * 4 + 16;
	auto p = (unsigned char*)malloc(aligned_size * 2);
	auto buf = p + aligned_size;

	memcpy(p + 2,pb,len);
	*p = 0xFF;

	unsigned char* src = p + 2;
	unsigned char* dst = buf;

	unsigned skip_count = 0;
	for (unsigned i = 0;i < len;i++)
	{
		if (src[i] == 0x03 && src[i - 1] == 0 && src[i - 2] == 0)
		{
			skip_count++;
			continue;
		}

		*dst = src[i];
		dst++;
	}
	memcpy(pb,buf,len - skip_count);

	free(p);
	return (int)(len - skip_count);
}
/////////// HEVC_EBSP2RBSP ///////////