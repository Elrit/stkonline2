#include "sv_num_conv.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "errio.h"
#include "types.h"

#define ABS(x) ((x) >= 0 ? (x) : -(x))

bool sv_to_unsigned(
	const Str_View buf,
	const ulonglong limit,
	ulonglong *const out
) {
	errno = 0;
	*out = strtoull(buf.data, nullptr, 10);
	if (errno) {
		perrorf(
			"Failed to convert an attribute to a number '%.*s'\n",
			(int)buf.len,
			buf.data
		);
		return false;
	}

	if (*out > limit) {
		eprintf(
			"Attribute value is too large '%.*s'\n",
			(int)buf.len,
			buf.data
		);
		return false;
	}

	return true;
}

bool sv_to_signed(
	Str_View buf,
	const ulonglong limit,
	longlong *const out
) {
	const int sign = *buf.data == '-' ? -1 : 1;
	if (sign == -1) {
		++buf.data;
		--buf.len;
	}

	ulonglong abs_val;
	if (!sv_to_unsigned(buf, limit, &abs_val)) {
		return false;
	}
	*out = (longlong)abs_val * sign;

	return true;
}

bool sv_to_uint(const Str_View buf, uint *const out) {
	ulonglong res;
	if (!sv_to_unsigned(buf, UINT_MAX, &res)) {
		return false;
	}
	*out = (uint)res;
	return true;
}

bool sv_to_ulong(const Str_View buf, ulong *const out) {
	ulonglong res;
	if (!sv_to_unsigned(buf, ULONG_MAX, &res)) {
		return false;
	}
	*out = (ulong)res;
	return true;
}

bool sv_to_long(const Str_View buf, long *const out) {
	longlong res;
	if (!sv_to_signed(buf, LONG_MAX, &res)) {
		return false;
	}
	*out = (long)res;
	return true;
}

bool sv_to_size(const Str_View buf, size_t *const out) {
	ulonglong res;
	if (!sv_to_unsigned(buf, SIZE_MAX, &res)) {
		return false;
	}
	*out = (size_t)res;
	return true;
}

bool sv_to_udecimal(
	const Str_View buf,
	const ulonglong limit,
	ulonglong *const out
) {
	*out = 0;
	bool dot_found = false;
	ulonglong val = 0;

	for (size_t i = 0; i < buf.len; ++i) {
		const char c = buf.data[i];

		if (c == '.') {
			if (dot_found) {
				eprintf(
					"Too many decimal points in an attribute '%.*s\n",
					(int)buf.len,
					buf.data
				);
				return false;
			}
			if (buf.len - i - 1 != decimal_digits_count) {
				eprintf(
					"Attribute has an invalid number of decimal digits '%.*s'\n",
					(int)buf.len,
					buf.data
				);
				return false;
			}

			dot_found = true;
			continue;
		}
		if (!isdigit(c)) {
			eprintf(
				"Invalid character in a decimal attribute '%.s'\n",
				(int)buf.len,
				buf.data
			);
			return false;
		}
	
		const ulonglong digit = (ulonglong)(c - '0');
		if (val > (limit / 10) - digit || val > (ULLONG_MAX / 10) - digit) {
			eprintf(
				"Decimal attribute value is too large '%.*s'\n",
				(int)buf.len,
				buf.data
			);
			return false;
		}
		val = val * 10 + digit;
	}

	*out = val;	
	return true;
}

bool sv_to_decimal(
	Str_View buf,
	const ulonglong limit, // <-limit; limit>
	longlong *const out
) {
	const int sign = *buf.data == '-' ? -1 : 1;
	if (sign == -1) {
		++buf.data;
		--buf.len;
	}

	ulonglong abs_val;
	if (!sv_to_udecimal(buf, limit, &abs_val)) {
		return false;
	}
	*out = (longlong)abs_val * sign;

	return true;
}

bool sv_to_long_decimal(const Str_View buf, long *const out) {
	longlong res;
	if(!sv_to_decimal(buf, LONG_MAX, &res)) {
		return false;
	}
	*out = (long)res;
	return true;
}

bool sv_to_uint_decimal(const Str_View buf, uint *const out) {
	ulonglong res;
	if(!sv_to_udecimal(buf, UINT_MAX, &res)) {
		return false;
	}
	*out = (uint)res;
	return true;
}

bool sv_to_ulong_decimal(const Str_View buf, ulong *const out) {
	ulonglong res;
	if(!sv_to_udecimal(buf, ULONG_MAX, &res)) {
		return false;
	}
	*out = (ulong)res;
	return true;
}
