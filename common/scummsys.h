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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef COMMON_SCUMMSYS_H
#define COMMON_SCUMMSYS_H


#if defined(_WIN32_WCE) && _WIN32_WCE < 300
	#define NONSTANDARD_PORT
#endif

#if defined(NONSTANDARD_PORT)

	// Ports which need to perform #includes and #defines visible in
	// virtually all the source of ScummVM should do so by providing a
	// "portdefs.h" header file (and not by directly modifying this
	// header file).
	#include <portdefs.h>
#else // defined(NONSTANDARD_PORT)

	#if defined(WIN32)

		#ifdef _MSC_VER
		// vsnprintf is already defined in Visual Studio 2008
		#if (_MSC_VER < 1500)
			#define vsnprintf _vsnprintf
		#endif
		#endif

		#if !defined(_WIN32_WCE)

		#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
		#define NOGDICAPMASKS
		#define OEMRESOURCE
		#define NONLS
		#define NOICONS
		#define NOMCX
		#define NOPROFILER
		#define NOKANJI
		#define NOSERVICE
		#define NOMETAFILE
		#define NOCOMM
		#define NOCRYPT
		#define NOIME
		#define NOATOM
		#define NOCTLMGR
		#define NOCLIPBOARD
		#define NOMEMMGR
		#define NOSYSMETRICS
		#define NOMENUS
		#define NOOPENFILE
		#define NOWH
		#define NOSOUND
		#define NODRAWTEXT

		#endif

		#if defined(ARRAYSIZE)
		// VS2005beta2 introduces new stuff in winnt.h
		#undef ARRAYSIZE
		#endif

	#endif

	#if defined(__QNXNTO__)
	#include <strings.h>	/* For strcasecmp */
	#endif

	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <stdarg.h>
	#include <assert.h>
	#include <ctype.h>
	// MSVC does not define M_PI, M_SQRT2 and other math defines by default.
	// _USE_MATH_DEFINES must be defined in order to have these defined, thus
	// we enable it here. For more information, check:
	// http://msdn.microsoft.com/en-us/library/4hwaceh6(v=VS.100).aspx
	#define _USE_MATH_DEFINES
	#include <math.h>

#endif



// Use config.h, generated by configure
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

//
// Define scumm_stricmp and scumm_strnicmp
//
#if defined(_WIN32_WCE) || defined(_MSC_VER)
	#define scumm_stricmp stricmp
	#define scumm_strnicmp _strnicmp
	#define snprintf _snprintf
#elif defined(__MINGW32__) || defined(__GP32__) || defined(__DS__)
	#define scumm_stricmp stricmp
	#define scumm_strnicmp strnicmp
#else
	#define scumm_stricmp strcasecmp
	#define scumm_strnicmp strncasecmp
#endif


// In the following we configure various targets, in particular those
// which can't use our "configure" tool and hence don't use config.h.
//
// Some #defines that occur here frequently:
// SCUMM_LITTLE_ENDIAN
//    - Define this on a little endian target
// SCUMM_BIG_ENDIAN
//    - Define this on a big endian target
// SCUMM_NEED_ALIGNMENT
//    - Define this if your system has problems reading e.g. an int32 from an odd address
// SMALL_SCREEN_DEVICE
//    - ...
// ...


//
// By default we try to use pragma push/pop to ensure various structs we use
// are "packed". If your compiler doesn't support this pragma, you are in for
// a problem. If you are lucky, there is a compiler switch, or another pragma,
// doing the same thing -- in that case, try to modify common/pack-begin.h and
// common/pack-end.h accordingly. Or maybe your port simply *always* packs
// everything, in which case you could #undefine SCUMMVM_USE_PRAGMA_PACK.
//
// If neither is possible, tough luck. Try to contact the team, maybe we can
// come up with a solution, though I wouldn't hold my breath on it :-/.
//
#define SCUMMVM_USE_PRAGMA_PACK


#if defined(HAVE_CONFIG_H)
	// All settings should have been set in config.h

#elif defined(__SYMBIAN32__)

	#define SCUMM_LITTLE_ENDIAN
	#define SCUMM_NEED_ALIGNMENT

#elif defined(_WIN32_WCE)

	#define SCUMM_LITTLE_ENDIAN

#elif defined(_MSC_VER)

	#define SCUMM_LITTLE_ENDIAN

#elif defined(__MINGW32__)

	#define SCUMM_LITTLE_ENDIAN

#elif defined(SDL_BACKEND)
	/* need this for the SDL_BYTEORDER define */
	#include <SDL_byteorder.h>

	#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	#define SCUMM_LITTLE_ENDIAN
	#elif SDL_BYTEORDER == SDL_BIG_ENDIAN
	#define SCUMM_BIG_ENDIAN
	#else
	#error Neither SDL_BIG_ENDIAN nor SDL_LIL_ENDIAN is set.
	#endif

#elif defined(__DC__)

	#define SCUMM_LITTLE_ENDIAN
	#define SCUMM_NEED_ALIGNMENT

#elif defined(__GP32__)

	#define SCUMM_LITTLE_ENDIAN
	#define SCUMM_NEED_ALIGNMENT

#elif defined(__PLAYSTATION2__)

	#define SCUMM_LITTLE_ENDIAN
	#define SCUMM_NEED_ALIGNMENT

#elif defined(__N64__)

	#define SCUMM_BIG_ENDIAN
	#define SCUMM_NEED_ALIGNMENT

#elif defined(__PSP__)

	#define	SCUMM_LITTLE_ENDIAN
	#define	SCUMM_NEED_ALIGNMENT

#elif defined(__amigaos4__)

	#define	SCUMM_BIG_ENDIAN
	#define	SCUMM_NEED_ALIGNMENT

#elif defined(__DS__)

	#define SCUMM_NEED_ALIGNMENT
	#define SCUMM_LITTLE_ENDIAN

#elif defined(__WII__)

	#define	SCUMM_BIG_ENDIAN
	#define	SCUMM_NEED_ALIGNMENT

#elif defined(IPHONE)

	#define	SCUMM_LITTLE_ENDIAN
	#define	SCUMM_NEED_ALIGNMENT


#else
	#error No system type defined

#endif


//
// Some more system specific settings.
// TODO/FIXME: All of these should be moved to backend specific files (such as portdefs.h)
//
#if defined(__SYMBIAN32__)

	#define SMALL_SCREEN_DEVICE

#elif defined(_WIN32_WCE)

	#if _WIN32_WCE < 300
	#define SMALL_SCREEN_DEVICE
	#endif

#elif defined(DINGUX)

	// Very BAD hack following, used to avoid triggering an assert in uClibc dingux library
	// "toupper" when pressing keyboard function keys.
	#undef toupper
	#define toupper(c) (((c & 0xFF) >= 97) && ((c & 0xFF) <= 122) ? ((c & 0xFF) - 32) : (c & 0xFF))

#elif defined(__PSP__)

	#include <malloc.h>
	#include "backends/platform/psp/memory.h"

	/* to make an efficient, inlined memcpy implementation */
	#define memcpy(dst, src, size)   psp_memcpy(dst, src, size)

#endif


//
// Fallbacks / default values for various special macros
//
#ifndef GCC_PRINTF
	#if defined(__GNUC__) || defined(__INTEL_COMPILER)
		#define GCC_PRINTF(x,y) __attribute__((__format__(__printf__, x, y)))
	#else
		#define GCC_PRINTF(x,y)
	#endif
#endif

#ifndef PACKED_STRUCT
	#if defined(__GNUC__) || defined(__INTEL_COMPILER)
		#define PACKED_STRUCT __attribute__((__packed__))
	#else
		#define PACKED_STRUCT
	#endif
#endif

#ifndef FORCEINLINE
	#if defined(_MSC_VER)
		#define FORCEINLINE __forceinline
	#elif (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
		#define FORCEINLINE inline __attribute__((__always_inline__))
	#else
		#define FORCEINLINE inline
	#endif
#endif

#ifndef PLUGIN_EXPORT
	#if defined(_MSC_VER) || defined(_WIN32_WCE) || defined(__MINGW32__)
		#define PLUGIN_EXPORT __declspec(dllexport)
	#else
		#define PLUGIN_EXPORT
	#endif
#endif

#ifndef NORETURN_PRE
	#if defined(_MSC_VER)
		#define NORETURN_PRE __declspec(noreturn)
	#else
		#define	NORETURN_PRE
	#endif
#endif

#ifndef NORETURN_POST
	#if defined(__GNUC__) || defined(__INTEL_COMPILER)
		#define NORETURN_POST __attribute__((__noreturn__))
	#else
		#define	NORETURN_POST
	#endif
#endif

#ifndef STRINGBUFLEN
  #if defined(__N64__) || defined(__DS__)
    #define STRINGBUFLEN 256
  #else
    #define STRINGBUFLEN 1024
  #endif
#endif

#ifndef LOCAL_PI
#define LOCAL_PI 3.14159265358979323846
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 256
#endif


//
// Typedef our system types
//
#if !defined(HAVE_CONFIG_H)

	#if defined(__SYMBIAN32__)

		// Enable Symbians own datatypes
		// This is done for two reasons
		// a) uint is already defined by Symbians libc component
		// b) Symbian is using its "own" datatyping, and the Scummvm port
		//    should follow this to ensure the best compability possible.
		typedef unsigned char byte;

		typedef unsigned char uint8;
		typedef signed char int8;

		typedef unsigned short int uint16;
		typedef signed short int int16;

		typedef unsigned long int uint32;
		typedef signed long int int32;

	#elif defined(__GP32__)

		// Override typenames. uint is already defined by system header files.
		typedef unsigned char byte;

		typedef unsigned char uint8;
		typedef signed char int8;

		typedef unsigned short int uint16;
		typedef signed short int int16;

		typedef unsigned long int uint32;
		typedef signed long int int32;

	#elif defined(__N64__)

		typedef unsigned char byte;

		typedef unsigned char uint8;
		typedef signed char int8;

		typedef unsigned short int uint16;
		typedef signed short int int16;

		typedef unsigned int uint32;
		typedef signed int int32;

	#elif defined(__DS__)

		// Do nothing, the SDK defines all types we need in nds/ndstypes.h,
		// which we include in our portsdef.h

	#else

		typedef unsigned char byte;
		typedef unsigned char uint8;
		typedef signed char int8;
		typedef unsigned short uint16;
		typedef signed short int16;
		typedef unsigned int uint32;
		typedef signed int int32;
		typedef unsigned int uint;

	#endif

#endif



//
// Overlay color type (FIXME: shouldn't be declared here)
//
#if defined(NEWGUI_256)
	// 256 color only on PalmOS
	typedef byte OverlayColor;
#else
	// 15/16 bit color mode everywhere else...
	typedef uint16 OverlayColor;
#endif

#include "common/forbidden.h"

#endif
