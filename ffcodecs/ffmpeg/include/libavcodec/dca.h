/*
 * DCA compatible decoder
 * Copyright (C) 2004 Gildas Bazin
 * Copyright (C) 2004 Benjamin Zores
 * Copyright (C) 2006 Benjamin Larsson
 * Copyright (C) 2007 Konstantin Shishkov
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVCODEC_DCA_H
#define AVCODEC_DCA_H

#include <stdint.h>
#include "avcodec.h"

#define DCA_PRIM_CHANNELS_MAX  (7)
#define DCA_ABITS_MAX         (32)      /* Should be 28 */
#define DCA_SUBSUBFRAMES_MAX   (4)
#define DCA_SUBFRAMES_MAX     (16)
#define DCA_BLOCKS_MAX        (16)
#define DCA_LFE_MAX            (3)
#define DCA_CHSETS_MAX         (4)
#define DCA_CHSET_CHANS_MAX    (8)

#define DCA_PRIM_CHANNELS_MAX  (7)
#define DCA_ABITS_MAX         (32)      /* Should be 28 */
#define DCA_SUBSUBFRAMES_MAX   (4)
#define DCA_SUBFRAMES_MAX     (16)
#define DCA_BLOCKS_MAX        (16)
#define DCA_LFE_MAX            (3)
#define DCA_XLL_FBANDS_MAX     (4)
#define DCA_XLL_SEGMENTS_MAX  (16)
#define DCA_XLL_CHSETS_MAX    (16)
#define DCA_XLL_CHANNELS_MAX  (16)
#define DCA_XLL_AORDER_MAX    (15)

/* Arbitrary limit; not sure what the maximum really is, but much larger. */
#define DCA_XLL_DMIX_NCOEFFS_MAX (18)

#define DCA_MAX_FRAME_SIZE       16384
#define DCA_MAX_EXSS_HEADER_SIZE  4096

#define DCA_BUFFER_PADDING_SIZE   1024

enum DCAExtensionMask {
    DCA_EXT_CORE       = 0x001, ///< core in core substream
    DCA_EXT_XXCH       = 0x002, ///< XXCh channels extension in core substream
    DCA_EXT_X96        = 0x004, ///< 96/24 extension in core substream
    DCA_EXT_XCH        = 0x008, ///< XCh channel extension in core substream
    DCA_EXT_EXSS_CORE  = 0x010, ///< core in ExSS (extension substream)
    DCA_EXT_EXSS_XBR   = 0x020, ///< extended bitrate extension in ExSS
    DCA_EXT_EXSS_XXCH  = 0x040, ///< XXCh channels extension in ExSS
    DCA_EXT_EXSS_X96   = 0x080, ///< 96/24 extension in ExSS
    DCA_EXT_EXSS_LBR   = 0x100, ///< low bitrate component in ExSS
    DCA_EXT_EXSS_XLL   = 0x200, ///< lossless extension in ExSS
};

/**
 * Convert bitstream to one representation based on sync marker
 */
int avpriv_dca_convert_bitstream(const uint8_t *src, int src_size, uint8_t *dst,
                             int max_size);

#endif /* AVCODEC_DCA_H */
