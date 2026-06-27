#pragma once

#include <stddef.h>

typedef struct {
	char *data;
	size_t cap;
	size_t len;
} Resp_Buf;

Resp_Buf resp_buf_make(const size_t cap);
bool fetch_svrs(Resp_Buf *const buf);
bool fetch_lb(Resp_Buf *const buf);
