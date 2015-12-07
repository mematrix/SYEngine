/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ABitReader.h"

#define INIT_BASE_READER(x) \
	ABitReader((const uint8_t*)x,sizeof(decltype(*x)))

namespace stagefright {

ABitReader::ABitReader(const uint8_t *data, size_t size)
{
    mData = data;
    mSize = size;
    mReservoir = 0;
    mNumBitsLeft = 0;
}

ABitReader::ABitReader(const char* data) : INIT_BASE_READER(data) {}
ABitReader::ABitReader(const unsigned char* data) : INIT_BASE_READER(data) {}
ABitReader::ABitReader(const int* data) : INIT_BASE_READER(data) {}
ABitReader::ABitReader(const unsigned* data) : INIT_BASE_READER(data) {}
ABitReader::ABitReader(const short* data) : INIT_BASE_READER(data) {}
ABitReader::ABitReader(const unsigned short* data)  : INIT_BASE_READER(data) {}
ABitReader::ABitReader(const long long* data) : INIT_BASE_READER(data) {}
ABitReader::ABitReader(const unsigned long long* data)  : INIT_BASE_READER(data) {}

void ABitReader::fillReservoir() {
    if (mSize == 0)
		return;

    mReservoir = 0;
    size_t i;
    for (i = 0; mSize > 0 && i < 4; ++i) {
        mReservoir = (mReservoir << 8) | *mData;

        ++mData;
        --mSize;
    }

    mNumBitsLeft = 8 * i;
    mReservoir <<= 32 - mNumBitsLeft;
}

uint32_t ABitReader::getBits(size_t n) {
	if (n > 32)
		return -1;

    uint32_t result = 0;
    while (n > 0) {
        if (mNumBitsLeft == 0) {
            fillReservoir();
        }

        size_t m = n;
        if (m > mNumBitsLeft) {
            m = mNumBitsLeft;
        }

        result = (result << m) | (mReservoir >> (32 - m));
        mReservoir <<= m;
        mNumBitsLeft -= m;

        n -= m;
    }

    return result;
}

void ABitReader::skipBits(size_t n) {
    while (n > 32) {
        getBits(32);
        n -= 32;
    }

    if (n > 0) {
        getBits(n);
    }
}

size_t ABitReader::numBitsLeft() const {
    return mSize * 8 + mNumBitsLeft;
}

const uint8_t *ABitReader::data() const {
    return mData - (mNumBitsLeft + 7) / 8;
}

}  // namespace stagefright
