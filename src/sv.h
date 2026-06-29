#pragma once

#include <stddef.h>

#define SV(cstr) (Str_View){.data = cstr, .len = sizeof(cstr) - 1}

typedef struct {
	const char *data;
	size_t len;
} Str_View;

constexpr Str_View sv_empty = {};

[[maybe_unused]] static Str_View sv_make(const char *data, size_t len) {
	return (Str_View){
		.data = data,
		.len = len
	};
}
bool sv_eq(const Str_View sv1, const Str_View sv2);
bool sv_contains_nocase(const Str_View buf, const Str_View target);
