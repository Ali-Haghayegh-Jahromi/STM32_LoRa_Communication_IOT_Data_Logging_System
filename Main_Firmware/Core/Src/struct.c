#include "struct.h"

int struct_get_endian(void)
{
	int i = 0x00000001;
	if (((char *)&i)[0]) {
		return STRUCT_ENDIAN_LITTLE;
	} else {
		return STRUCT_ENDIAN_BIG;
	}
}


static int myendian = STRUCT_ENDIAN_NOT_SET;


static void unpack_int32_t(const unsigned char **bp, int32_t *dst, int endian)
{
    uint32_t val;
    if (endian == myendian) {
        val = *((*bp)++);
        val |= (uint32_t)(*((*bp)++)) << 8;
        val |= (uint32_t)(*((*bp)++)) << 16;
        val |= (uint32_t)(*((*bp)++)) << 24;
    } else {
        val = *((*bp)++) << 24;
        val |= (uint32_t)(*((*bp)++)) << 16;
        val |= (uint32_t)(*((*bp)++)) << 8;
        val |= (uint32_t)(*((*bp)++));
    }
    if (val <= 0x7fffffffU) {
        *dst = val;
    } else {
        *dst = -1 - (int32_t)(0xffffffffU - val);
    }
}

static int unpack_va_list(const unsigned char *buf,int offset,const char *fmt,va_list args)
{
	INIT_REPETITION();
	const char *p;
	const unsigned char *bp;
	int *ep = &myendian;
	int endian;

	int32_t *l;

	if (STRUCT_ENDIAN_NOT_SET == myendian) 
		myendian = struct_get_endian();
	
	bp = buf + offset;
	for (p = fmt; *p != '\0'; p++) 
	{
		switch (*p) 
		{	
			case '!': /* network (= big-endian) */
				endian = STRUCT_ENDIAN_BIG;
				ep = &endian;
				break;
			case 'i': /* fall through */
			case 'l':
				BEGIN_REPETITION();
						l = va_arg(args, int32_t*);
						unpack_int32_t(&bp, l, *ep);
				END_REPETITION();
				break;
		}

		if (!isdigit((int)*p)) 
		{
				CLEAR_REPETITION();
		}
	}
	return (bp - buf);
}

int struct_unpack(const void *buf, const char *fmt, ...)
{
    va_list args;
    int unpacked_len = 0;

    va_start(args, fmt);
    unpacked_len = unpack_va_list((const unsigned char*)buf, 0, fmt, args);
    va_end(args);

    return unpacked_len;
}
