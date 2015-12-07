#ifndef __MEDIA_AV_ERROR_H
#define __MEDIA_AV_ERROR_H

#ifndef _MAKE_AV_ERROR_VALUE
#define _MAKE_AV_ERROR_VALUE(a,b,c,d) \
    ((unsigned)(unsigned char)(a) | ((unsigned)(unsigned char)(b) << 8) | \
    ((unsigned)(unsigned char)(c) << 16) | ((unsigned)(unsigned char)(d) << 24 ))
#endif

#define AV_MEDIA_ERR unsigned

///////////////////// Common Error /////////////////////

#define AV_ERR_OK 0
#define AV_ERR_UNKNOWN 1

#define AV_COMMON_ERR_UNEXPECTED 0xFFFFFFF1
#define AV_COMMON_ERR_CHANGED_STATE 0xFFFFFFF2
#define AV_COMMON_ERR_NOT_FOUND 0xFFFFFFF3
#define AV_COMMON_ERR_NOT_SUPPORTED 0xFFFFFFF4
#define AV_COMMON_ERR_INVALID_OPERATION 0xFFFFFFF5
#define AV_COMMON_ERR_INVALID_FORMAT 0xFFFFFFF6
#define AV_COMMON_ERR_IO_ERROR 0xFFFFFFF7
#define AV_COMMON_ERR_OUTOFMEMORY 0xFFFFFFF8
#define AV_COMMON_ERR_BUF_TOO_SMALL 0xFFFFFFF9
#define AV_COMMON_ERR_INVALIDARG 0xFFFFFFFA
#define AV_COMMON_ERR_PENDING 0xFFFFFFFB
#define AV_COMMON_ERR_NOTIMPL 0xFFFFFFFC
#define AV_COMMON_ERR_ABORT 0xFFFFFFFD
#define AV_COMMON_ERR_POINTER 0xFFFFFFFE
#define AV_COMMON_ERR_FAIL 0xFFFFFFFF

#define AVE_SUCCEEDED(x) (((AV_MEDIA_ERR)(x)) == AV_ERR_OK)
#define AVE_FAILED(x) (((AV_MEDIA_ERR)(x)) != AV_ERR_OK)

///////////////////// Common Error /////////////////////

///////////////////// IO Error /////////////////////

#define AV_IO_ERR_UNKNOWN_STATE _MAKE_AV_ERROR_VALUE('I','O','E','S')
#define AV_IO_ERR_NON_MORE_DATA _MAKE_AV_ERROR_VALUE('I','O','N','D')

#define AV_IO_ERR_SIZE_INVALID _MAKE_AV_ERROR_VALUE('I','O','S','I')
#define AV_IO_ERR_DATA_INVALID _MAKE_AV_ERROR_VALUE('I','O','D','I')

#define AV_IO_ERR_SEEK_FAILED _MAKE_AV_ERROR_VALUE('I','O','S','F')
#define AV_IO_ERR_READ_FAILED _MAKE_AV_ERROR_VALUE('I','O','R','F')
#define AV_IO_ERR_WRITE_FAILED _MAKE_AV_ERROR_VALUE('I','O','W','F')

///////////////////// IO Error /////////////////////

///////////////////// Open Error /////////////////////

#define AV_OPEN_ERR_PROBE_FAILED _MAKE_AV_ERROR_VALUE('O','P','P','F')
#define AV_OPEN_ERR_HEADER_INVALID _MAKE_AV_ERROR_VALUE('O','P','H','I')
#define AV_OPEN_ERR_STREAM_INVALID _MAKE_AV_ERROR_VALUE('O','P','S','I')
#define AV_OPEN_ERR_IO_DATA_INVALID _MAKE_AV_ERROR_VALUE('O','P','D','I')

///////////////////// Open Error /////////////////////

///////////////////// Close Error /////////////////////

#define AV_CLOSE_ERR_NOT_OPENED _MAKE_AV_ERROR_VALUE('C','L','N','O')
#define AV_CLOSE_ERR_CLOSE_FAILED _MAKE_AV_ERROR_VALUE('C','L','F','E')

///////////////////// Close Error /////////////////////

///////////////////// Seek&Flush Error /////////////////////

#define AV_SEEK_ERR_NOT_OPENED _MAKE_AV_ERROR_VALUE('S','K','N','O')
#define AV_SEEK_ERR_PARSER_FAILED _MAKE_AV_ERROR_VALUE('S','K','P','F')
#define AV_SEEK_ERR_KEY_FRAME _MAKE_AV_ERROR_VALUE('S','K','K','F')
#define AV_FLUSH_ERR_NOT_OPENED AV_SEEK_ERR_NOT_OPENED

///////////////////// Seek&Flush Error /////////////////////

///////////////////// ReadPacket Error /////////////////////

#define AV_READ_PACKET_ERR_NOT_OPENED _MAKE_AV_ERROR_VALUE('R','P','N','O')
#define AV_READ_PACKET_ERR_NON_MORE _MAKE_AV_ERROR_VALUE('R','P','N','M')
#define AV_READ_PACKET_ERR_DATA_INVALID _MAKE_AV_ERROR_VALUE('R','P','D','I')
#define AV_READ_PACKET_ERR_INDEX_OUT _MAKE_AV_ERROR_VALUE('R','P','I','O')
#define AV_READ_PACKET_ERR_ALLOC_PKT _MAKE_AV_ERROR_VALUE('R','P','A','P')

///////////////////// ReadPacket Error /////////////////////

#endif //__MEDIA_AV_ERROR_H