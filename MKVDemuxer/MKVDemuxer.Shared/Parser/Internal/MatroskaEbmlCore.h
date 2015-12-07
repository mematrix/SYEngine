#ifndef _MATROSKA_EBML_CORE_H
#define _MATROSKA_EBML_CORE_H

#include "MatroskaDataSource.h"

#define INVALID_EBML_ID 0
#define INVALID_EBML_SIZE -1

namespace MKV {
namespace EBML {
namespace Core {

int ParseByteZeroBitsNum(unsigned char value);
int StripZeroBitInNum(unsigned char value,int num);

int GetVIntLength(unsigned char entry);

unsigned GetEbmlId(DataSource::IDataSource* dataSource);
long long GetEbmlSize(DataSource::IDataSource* dataSource);

unsigned GetEbmlBytes(unsigned long long num);
int SearchEbmlClusterStartCodeOffset(unsigned char* buf,unsigned size);

}}} //MKV::EBML::Core.

#endif //_MATROSKA_EBML_CORE_H