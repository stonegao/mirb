#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef DEBUG
	#define RT_ASSERT(condition) do { \
		if(!(condition)) { \
			__asm__("int3\n"); \
			assert(0); \
		} \
	} while(0)
#else
	#define RT_ASSERT(condition) assert(condition)
#endif

#ifdef WINDOWS
    #include <windows.h>
    #include <excpt.h>
    #include "BeaEngine.h"
#else
	#define __stdcall __attribute__((__stdcall__))
	#define __cdecl __attribute__((__cdecl__))

	#include <sys/mman.h>
#endif

static inline void __attribute__((noreturn)) __builtin_unreachable(void)
{
	while(1);
}

#include "vendor/kvec.h"
#include "vendor/khash.h"
