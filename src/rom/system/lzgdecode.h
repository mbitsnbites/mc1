/* -*- mode: c; tab-width: 4; indent-tabs-mode: nil; -*- */

/*
* This file is part of liblzg.
*
* Copyright (c) 2010 Marcus Geelnard
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would
*    be appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source
*    distribution.
*/

#ifndef LZGDECODE_H_
#define LZGDECODE_H_

typedef unsigned int lzg_uint32_t;

lzg_uint32_t LZG_DecodedSize(const unsigned char *in, lzg_uint32_t insize);

lzg_uint32_t LZG_Decode(const unsigned char *in,
                        lzg_uint32_t insize,
                        unsigned char *out,
                        lzg_uint32_t outsize);

#endif /* LZGDECODE_H_ */

