#pragma once

#include "int_aliases.h"
#include "sv.h"

bool sv_to_unsigned(
	const Str_View buf,
	const ulonglong limit,
	ulonglong *const out
);
bool sv_to_signed(
	Str_View buf,
	const ulonglong limit,
	longlong *const out
);
bool sv_to_long(const Str_View buf, long *const out);
bool sv_to_uint(const Str_View buf, uint *const out);
bool sv_to_ulong(const Str_View buf, ulong *const out);
bool sv_to_size(const Str_View buf, size_t *const out);

bool sv_to_udecimal(
	const Str_View buf,
	const ulonglong limit,
	ulonglong *const out
);
bool sv_to_decimal(
	Str_View buf,
	const ulonglong limit, // <-limit; limit>
	longlong *const out
);
bool sv_to_long_decimal(const Str_View buf, long *const out);
bool sv_to_uint_decimal(const Str_View buf, uint *const out);
bool sv_to_ulong_decimal(const Str_View buf, ulong *const out);
