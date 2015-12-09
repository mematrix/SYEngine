#ifndef __ISOM_TYPES_H_
#define __ISOM_TYPES_H_

#include "IsomGlobal.h"

#define MK_ISOM_TYPE(name, x) \
	static const unsigned name = ISOM_FCC(x);
#define MK_ITUNES_TYPE(name, a,b,c,d) \
	static const unsigned name = ISOM_MKTAG(a,b,c,d);

namespace Isom {
namespace Types {

MK_ISOM_TYPE(isom_box_container_root, 'root')
MK_ISOM_TYPE(isom_box_container_moov, 'moov')
MK_ISOM_TYPE(isom_box_container_trak, 'trak')
MK_ISOM_TYPE(isom_box_container_tref, 'tref')
MK_ISOM_TYPE(isom_box_container_mdia, 'mdia')
MK_ISOM_TYPE(isom_box_container_minf, 'minf')
MK_ISOM_TYPE(isom_box_container_stbl, 'stbl')
MK_ISOM_TYPE(isom_box_container_dinf, 'dinf')
MK_ISOM_TYPE(isom_box_container_edts, 'edts')
MK_ISOM_TYPE(isom_box_container_udta, 'udta')
MK_ISOM_TYPE(isom_box_container_meta, 'meta')
MK_ISOM_TYPE(isom_box_container_ilst, 'ilst')
MK_ISOM_TYPE(isom_box_container_esds, 'esds')
MK_ISOM_TYPE(isom_box_container_stsd, 'stsd')
MK_ISOM_TYPE(isom_box_container_dref, 'dref')

MK_ISOM_TYPE(isom_box_content_ftyp, 'ftyp')
MK_ISOM_TYPE(isom_box_content_mdat, 'mdat')
MK_ISOM_TYPE(isom_box_content_free, 'free')
MK_ISOM_TYPE(isom_box_content_wide, 'wide')
MK_ISOM_TYPE(isom_box_content_junk, 'junk')
MK_ISOM_TYPE(isom_box_content_skip, 'skip')
MK_ISOM_TYPE(isom_box_content_uuid, 'uuid')
MK_ISOM_TYPE(isom_box_content_pnot, 'pnot')

MK_ISOM_TYPE(isom_box_content_mvhd, 'mvhd')
MK_ISOM_TYPE(isom_box_content_tkhd, 'tkhd')
MK_ISOM_TYPE(isom_box_content_mdhd, 'mdhd')
MK_ISOM_TYPE(isom_box_content_hdlr, 'hdlr')
MK_ISOM_TYPE(isom_box_content_vmhd, 'vmhd')
MK_ISOM_TYPE(isom_box_content_smhd, 'smhd')
MK_ISOM_TYPE(isom_box_content_nmhd, 'nmhd')
MK_ISOM_TYPE(isom_box_content_hmhd, 'hmhd')
MK_ISOM_TYPE(isom_box_content_url, 'url ')

MK_ISOM_TYPE(isom_box_content_stts, 'stts')
MK_ISOM_TYPE(isom_box_content_stss, 'stss')
MK_ISOM_TYPE(isom_box_content_stsz, 'stsz')
MK_ISOM_TYPE(isom_box_content_stco, 'stco')
MK_ISOM_TYPE(isom_box_content_stsc, 'stsc')
MK_ISOM_TYPE(isom_box_content_ctts, 'ctts')
MK_ISOM_TYPE(isom_box_content_co64, 'co64')
MK_ISOM_TYPE(isom_box_content_stz2, 'stz2')
MK_ISOM_TYPE(isom_box_content_stps, 'stps')
MK_ISOM_TYPE(isom_box_content_elst, 'elst')
MK_ISOM_TYPE(isom_box_content_chpl, 'chpl')

MK_ISOM_TYPE(isom_box_other_cmov, 'cmov')
MK_ISOM_TYPE(isom_box_other_cmvd, 'cmvd')
MK_ISOM_TYPE(isom_box_other_dcom, 'dcom')
MK_ISOM_TYPE(isom_box_other_zlib, 'zlib')

MK_ISOM_TYPE(isom_box_other_wave, 'wave')
MK_ISOM_TYPE(isom_box_other_data, 'data')
MK_ISOM_TYPE(isom_box_other_covr, 'covr')
MK_ISOM_TYPE(isom_box_other_colr, 'colr')
MK_ISOM_TYPE(isom_box_other_enda, 'enda')
MK_ISOM_TYPE(isom_box_other_strf, 'strf')
MK_ISOM_TYPE(isom_box_other_pasp, 'pasp')
MK_ISOM_TYPE(isom_box_other_chan, 'chan')
MK_ISOM_TYPE(isom_box_other_tmcd, 'tmcd')

MK_ISOM_TYPE(isom_box_codec_avc1, 'avc1')
MK_ISOM_TYPE(isom_box_codec_hvc1, 'hvc1')
MK_ISOM_TYPE(isom_box_codec_hev1, 'hev1')
MK_ISOM_TYPE(isom_box_codec_mp4v, 'mp4v')
MK_ISOM_TYPE(isom_box_codec_divx, 'DIVX')
MK_ISOM_TYPE(isom_box_codec_xvid, 'XVID')
MK_ISOM_TYPE(isom_box_codec_3iv1, '3IV1')
MK_ISOM_TYPE(isom_box_codec_3iv2, '3IV2')
MK_ISOM_TYPE(isom_box_codec_3ivd, '3IVD')
MK_ISOM_TYPE(isom_box_codec_div3, 'DIV3')
MK_ISOM_TYPE(isom_box_codec_cvid, 'cvid')
MK_ISOM_TYPE(isom_box_codec_H263, 'H263')
MK_ISOM_TYPE(isom_box_codec_h263, 'h263')
MK_ISOM_TYPE(isom_box_codec_s263, 's263')
MK_ISOM_TYPE(isom_box_codec_m1v, 'm1v ')
MK_ISOM_TYPE(isom_box_codec_m1v1, 'm1v1')
MK_ISOM_TYPE(isom_box_codec_mpeg, 'mpeg')
MK_ISOM_TYPE(isom_box_codec_mp1v, 'mp1v')
MK_ISOM_TYPE(isom_box_codec_m2v1, 'm2v1')
MK_ISOM_TYPE(isom_box_codec_mp2v, 'mp2v')
MK_ISOM_TYPE(isom_box_codec_vp31, 'VP31')
MK_ISOM_TYPE(isom_box_codec_ffv1, 'FFV1')
MK_ISOM_TYPE(isom_box_codec_drac, 'drac')
MK_ISOM_TYPE(isom_box_codec_mjp2, 'mjp2')
MK_ISOM_TYPE(isom_box_codec_svc1, 'svc1')
MK_ISOM_TYPE(isom_box_codec_svc2, 'svc2')
MK_ISOM_TYPE(isom_box_codec_avs2, 'avs2')
MK_ISOM_TYPE(isom_box_codec_vc1, 'vc-1')

MK_ISOM_TYPE(isom_box_codec_mp4a, 'mp4a')
MK_ISOM_TYPE(isom_box_codec_mp3, '.mp3')
MK_ISOM_TYPE(isom_box_codec_samr, 'samr')
MK_ISOM_TYPE(isom_box_codec_sawb, 'sawb')
MK_ISOM_TYPE(isom_box_codec_sawp, 'sawp')
MK_ISOM_TYPE(isom_box_codec_alac, 'alac')
MK_ISOM_TYPE(isom_box_codec_ac3, 'ac-3')
MK_ISOM_TYPE(isom_box_codec_sac3, 'sac3')
MK_ISOM_TYPE(isom_box_codec_eac3, 'ec-3')
MK_ISOM_TYPE(isom_box_codec_alaw, 'alaw')
MK_ISOM_TYPE(isom_box_codec_ulaw, 'ulaw')
MK_ISOM_TYPE(isom_box_codec_spex, 'spex')
MK_ISOM_TYPE(isom_box_codec_g719, 'g719')
MK_ISOM_TYPE(isom_box_codec_g726, 'g726')
MK_ISOM_TYPE(isom_box_codec_sevc, 'sevc')
MK_ISOM_TYPE(isom_box_codec_lpcm, 'lpcm')
MK_ISOM_TYPE(isom_box_codec_twos, 'twos')
MK_ISOM_TYPE(isom_box_codec_mlpa, 'mlpa')
MK_ISOM_TYPE(isom_box_codec_raw, 'raw ')
MK_ISOM_TYPE(isom_box_codec_Opus, 'Opus')

MK_ISOM_TYPE(isom_box_codec_mp4s, 'mp4s')

MK_ISOM_TYPE(isom_type_name_vide, 'vide')
MK_ISOM_TYPE(isom_type_name_soun, 'soun')
MK_ISOM_TYPE(isom_type_name_subp, 'subp')

MK_ISOM_TYPE(isom_type_name_name, 'name')
MK_ISOM_TYPE(isom_type_name_data, 'data')
MK_ISOM_TYPE(isom_type_name_mean, 'mean')
MK_ISOM_TYPE(isom_type_name_hdlr, 'hdlr')
MK_ISOM_TYPE(isom_type_name_rap, 'rap ')

namespace iTunes {
	MK_ITUNES_TYPE(apple_tag_title, 0xa9,'n','a','m');
	MK_ITUNES_TYPE(apple_tag_sub_title, 0xa9,'s','t','3');
	MK_ITUNES_TYPE(apple_tag_album_artist, 'a','A','R','T');
	MK_ITUNES_TYPE(apple_tag_album, 0xa9,'a','l','b');
	MK_ITUNES_TYPE(apple_tag_original_artist, 0xa9,'o','p','e');
	MK_ITUNES_TYPE(apple_tag_artist0, 0xa9,'a','u','t');
	MK_ITUNES_TYPE(apple_tag_artist1, 0xa9,'A','R','T');
	MK_ITUNES_TYPE(apple_tag_date, 0xa9,'d','a','y');
	MK_ITUNES_TYPE(apple_tag_genre0, 0xa9,'g','e','n');
	MK_ITUNES_TYPE(apple_tag_genre1, 'g','n','r','e');
	MK_ITUNES_TYPE(apple_tag_comment0, 0xa9,'c','m','t');
	MK_ITUNES_TYPE(apple_tag_comment1, 0xa9,'i','n','f');
	MK_ITUNES_TYPE(apple_tag_copyright0, 0xa9,'c','p','y');
	MK_ITUNES_TYPE(apple_tag_copyright1, 'c','p','r','t');
	MK_ITUNES_TYPE(apple_tag_composer0, 0xa9,'c','o','m');
	MK_ITUNES_TYPE(apple_tag_composer1, 0xa9,'w','r','t');
	MK_ITUNES_TYPE(apple_tag_producer0, 0xa9,'P','R','D');
	MK_ITUNES_TYPE(apple_tag_producer1, 0xa9,'p','r','d');
	MK_ITUNES_TYPE(apple_tag_performers, 0xa9,'p','r','f');
	MK_ITUNES_TYPE(apple_tag_director, 0xa9,'d','i','r');
	MK_ITUNES_TYPE(apple_tag_disclaimer, 0xa9,'d','i','s');
	MK_ITUNES_TYPE(apple_tag_chapter, 0xa9,'c','h','p');
	MK_ITUNES_TYPE(apple_tag_track0, 0xa9,'t','r','k');
	MK_ITUNES_TYPE(apple_tag_track1, 't','r','k','n');
	MK_ITUNES_TYPE(apple_tag_lyrics, 0xa9,'l','y','r');
	MK_ITUNES_TYPE(apple_tag_disc, 'd','i','s','k');
	MK_ITUNES_TYPE(apple_tag_rating, 'r','t','n','g');
	MK_ITUNES_TYPE(apple_tag_link, 0xa9,'u','r','l');
	MK_ITUNES_TYPE(apple_tag_podcast, 'p','c','s','t');
	MK_ITUNES_TYPE(apple_tag_description, 'd','e','s','c');
	MK_ITUNES_TYPE(apple_tag_category, 'c','a','t','g');
	MK_ITUNES_TYPE(apple_tag_keywords, 'k','e','y','w');
	MK_ITUNES_TYPE(apple_tag_original_source, 0xa9,'s','r','c');
	MK_ITUNES_TYPE(apple_tag_original_format, 0xa9,'f','m','t');
	MK_ITUNES_TYPE(apple_tag_host_computer, 0xa9,'h','s','t');
	MK_ITUNES_TYPE(apple_tag_encoder0, 0xa9,'t','o','o');
	MK_ITUNES_TYPE(apple_tag_encoder1, 0xa9,'e','n','c');
	MK_ITUNES_TYPE(apple_tag_encoder2, 0xa9,'s','w','r');
	MK_ITUNES_TYPE(apple_tag_cover, 'c','o','v','r');
	MK_ITUNES_TYPE(apple_tag_sort_title, 's','o','n','m');
	MK_ITUNES_TYPE(apple_tag_sort_artist, 's','o','a','r');
	MK_ITUNES_TYPE(apple_tag_sort_album_title, 's','o','a','l');
	MK_ITUNES_TYPE(apple_tag_sort_album_artist, 's','o','a','a');
}
}}

#undef MK_ITUNES_TYPE
#undef MK_ISOM_TYPE

#endif //__ISOM_TYPES_H_