#define _ALLOW_KEYWORD_MACROS
#ifndef __GNUC__
#define __GNUC__
#endif

#ifndef __STDC__
#define __STDC__
#endif

#ifdef _WIN32
#define __attribute__(A) /* do nothing */
#endif
//
//#pragma once
#ifdef _MSC_VER
#define __asm__(x)
#define __extension__(x)
#define __attribute__(x)
#define __builtin_va_list int
#define __extension__
#define __inline__
#define __builtin_constant_p
#define _Bool bool
//#undef __cplusplus
#endif