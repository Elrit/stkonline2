#include "sv.h"

#include <ctype.h>
#include <string.h>

bool sv_eq(const Str_View sv1, const Str_View sv2) {
	if (sv1.len != sv2.len) {
		return false;
	}
	for (size_t i = 0; i < sv1.len; ++i) {
		if (sv1.data[i] != sv2.data[i]) {
			return false;
		}
	}
	return true;
}

bool sv_eq_no_case(const Str_View sv1, const Str_View sv2) {
	if (sv1.len != sv2.len) {
		return false;
	}
	for (size_t i = 0; i < sv1.len; ++i) {
		if (tolower(sv1.data[i]) != tolower(sv2.data[i])) {
			return false;
		}
	}
	return true;
}

bool sv_contains_nocase(const Str_View buf, const Str_View target) {
	if (target.len > buf.len) {
		return false;
	}
	for (size_t i = 0; i <= buf.len - target.len; ++i) {
		size_t j = 0;
		for (; j < target.len; ++j) {
			if (tolower(buf.data[i + j]) != tolower(target.data[j])) {
				break;
			}
		}
		if (j == target.len) {
			return true;
		}
	}
	return false;
}
