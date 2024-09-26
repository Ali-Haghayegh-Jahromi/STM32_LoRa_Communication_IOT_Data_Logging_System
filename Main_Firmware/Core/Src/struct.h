#include <String.h>
#include <Stdint.h>
#include <Stdlib.h>
#include <Stdarg.h>
#include <Ctype.h>
#include <Errno.h>
#include <Math.h>

#define STRUCT_ENDIAN_NOT_SET   0
#define STRUCT_ENDIAN_BIG       1
#define STRUCT_ENDIAN_LITTLE    2
#define INIT_REPETITION(_x) int _struct_rep = 0
#define BEGIN_REPETITION(_x) do { _struct_rep--
#define END_REPETITION(_x) } while(_struct_rep > 0)
#define CLEAR_REPETITION(_x) _struct_rep = 0

extern int struct_unpack(const void *buf, const char *fmt, ...);
