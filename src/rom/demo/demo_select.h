// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2020 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

#ifndef DEMO_SELECT_H_
#define DEMO_SELECT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DEMO_NONE 0
#define DEMO_MANDELBROT 1
#define DEMO_RAYTRACE 2
#define DEMO_RETRO 3
#define DEMO_STARS 4

// Global demo selection, controlled by the sub-demos.
extern int g_demo_select;

#ifdef __cplusplus
}
#endif

#endif  // DEMO_SELECT_H_