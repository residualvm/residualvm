/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef BACKEND_SDL_SYS_H
#define BACKEND_SDL_SYS_H

// The purpose of this header is to include the SDL headers in a uniform
// fashion, even on the Symbian port.
// Moreover, it contains a workaround for the fact that SDL_rwops.h uses
// a FILE pointer in one place, which conflicts with common/forbidden.h.
// The SDL 1.3 headers also include strings.h

#include "common/scummsys.h"

// Remove FILE override from common/forbidden.h, and replace
// it with an alternate slightly less unfriendly override.
#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_FILE)
#undef FILE
// Solaris has typedef __FILE FILE in several places already
#if !defined(__sun)
typedef struct { int FAKE; } FAKE_FILE;
#define FILE FAKE_FILE
#endif   // (__sun)
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_strcasecmp)
#undef strcasecmp
#define strcasecmp FAKE_strcasecmp
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_strncasecmp)
#undef strncasecmp
#define strncasecmp FAKE_strncasecmp
#endif

// ResidualVM specific for SDL2 start here:
#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_asctime)
#undef asctime
#define asctime    FAKE_asctime
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_time)
#undef time
#define time    FAKE_time
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_ctime)
#undef ctime
#define ctime    FAKE_ctime
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_mktime)
#undef mktime
#define mktime    FAKE_mktime
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_difftime)
#undef difftime
#define difftime    FAKE_difftime
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_getdate)
#undef getdate
#define getdate    FAKE_getdate
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_localtime)
#undef localtime
#define localtime    FAKE_localtime
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_gmtime)
#undef gmtime
#define gmtime    FAKE_gmtime
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_clock)
#undef clock
#define clock    FAKE_clock
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_longjmp)
#undef longjmp
#define longjmp    FAKE_longjmp
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_setjmp)
#undef setjmp
#define setjmp    FAKE_setjmp
#endif
// ResidualVM specific for SDL2 end here

#if defined(__SYMBIAN32__)
#include <esdl\SDL.h>
#else
#include <SDL.h>
#endif

// Finally forbid FILE again (if it was forbidden to start with)
#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_FILE)
#undef FILE
#define FILE	FORBIDDEN_SYMBOL_REPLACEMENT
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_strcasecmp)
#undef strcasecmp
#define strcasecmp     FORBIDDEN_SYMBOL_REPLACEMENT
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_strncasecmp)
#undef strncasecmp
#define strncasecmp    FORBIDDEN_SYMBOL_REPLACEMENT
#endif

// ResidualVM specific for SDL2 start here:
#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_asctime)
#undef asctime
#define asctime    FORBIDDEN_SYMBOL_REPLACEMENT
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_time)
#undef time
// This define conflict with variable names, so 'time' is not forbidden again
//#define time    FORBIDDEN_SYMBOL_REPLACEMENT
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_ctime)
#undef ctime
#define ctime    FORBIDDEN_SYMBOL_REPLACEMENT
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_mktime)
#undef mktime
#define mktime    FORBIDDEN_SYMBOL_REPLACEMENT
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_difftime)
#undef difftime
#define difftime    FORBIDDEN_SYMBOL_REPLACEMENT
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_getdate)
#undef getdate
#define getdate    FORBIDDEN_SYMBOL_REPLACEMENT
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_localtime)
#undef localtime
#define localtime    FORBIDDEN_SYMBOL_REPLACEMENT
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_gmtime)
#undef gmtime
#define gmtime    FORBIDDEN_SYMBOL_REPLACEMENT
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_clock)
#undef clock
#define clock    FORBIDDEN_SYMBOL_REPLACEMENT
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_longjmp)
#undef longjmp
#define longjmp    FORBIDDEN_SYMBOL_REPLACEMENT
#endif

#if !defined(FORBIDDEN_SYMBOL_ALLOW_ALL) && !defined(FORBIDDEN_SYMBOL_EXCEPTION_setjmp)
#undef setjmp
#define setjmp    FORBIDDEN_SYMBOL_REPLACEMENT
#endif
// ResidualVM specific for SDL2 end here

#endif
