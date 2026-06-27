#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "errio.h"

#define BASE_URL "https://online.supertuxkart.net"

static void perror_curl(const CURLcode err, const char *const msg) {
	eprintf("%s: %s\n", msg, curl_easy_strerror(err));
}

Resp_Buf resp_buf_make(const size_t cap) {
	Resp_Buf buf = {
		.data = malloc(cap),
		.cap = cap
	};
	if (!buf.data) {
		perror("malloc failed");
	}
	return buf;
}

static size_t recv_resp(
	const char *const chunk,
	[[maybe_unused]] const size_t size,
	const size_t chunk_len,
	void *const user_data
) {
	Resp_Buf *const buf = (Resp_Buf *)user_data;

	const size_t new_len = buf->len + chunk_len;
	if (new_len > buf->cap) {
		eprintf("Server response is too large\n");
		return 0;
	}
	memcpy(buf->data + buf->len, chunk, chunk_len);
	buf->len = new_len;

	return chunk_len;
}

static bool fetch(
	Resp_Buf *const buf,
	const char *const url
) {
	bool retval = false;

	CURL *const curl = curl_easy_init();
	if (!curl) {
		eprintf("curl_easy_init failed\n");
		goto out;
	}

	#define SETOPT(opt, val) curl_easy_setopt(curl, opt, val)

	CURLcode err = 0;
	if (
		(err = SETOPT(CURLOPT_WRITEFUNCTION,  recv_resp  )) ||
		(err = SETOPT(CURLOPT_WRITEDATA,      buf        )) ||
		(err = SETOPT(CURLOPT_URL,            url        )) ||
		(err = SETOPT(CURLOPT_USERAGENT,      "stkonline")) ||
		(err = SETOPT(CURLOPT_FOLLOWLOCATION, 1L         ))
	) {
		perror_curl(err, "curl_easy_setopt");
		goto cleanup;
	}

	#undef SETOPT

	err = curl_easy_perform(curl);
	if (err) {
		perror_curl(err, "curl_easy_perform");
		goto cleanup;
	}

	long status;
	err = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
	if (err) {
		perror_curl(err, "curl_easy_getinfo");
		goto cleanup;
	}
	if (status != 200) {
		eprintf("Server returned %ld\n", status);
		goto cleanup;
	}
	
	retval = true;

cleanup:
	curl_easy_cleanup(curl);
out:
	return retval;
}

bool fetch_svrs(Resp_Buf *const buf) {
	return fetch(buf, BASE_URL"/api/v2/server/get-all");
}

bool fetch_lb(Resp_Buf *const buf) {
	return fetch(buf, BASE_URL"/rankings.php");
}
