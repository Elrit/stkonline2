#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "errio.h"
#include "output.h"
#include "parser.h"
#include "sv.h"
#include "types.h"

#define KB(x) ((x) * 1024ull)
#define MB(x) ((x) * 1024ull * 1024ull)

static constexpr size_t lb_buf_cap = MB(2);

static bool show_online_svrs(const char *const argv[]) {
	bool retval = false;

	bool show_all = false;

	const char *const arg = argv[2];
	if (arg) {
		if (strcmp(arg, "all")) {
			eprintf("Invalid argument\n");
			goto out;
		}
		show_all = true;
	}

	constexpr size_t buf_cap = KB(64);
	Resp_Buf buf = resp_buf_make(buf_cap);
	if (!buf.data) {
		goto out;
	}

	if (!fetch_svrs(&buf)) {
		goto cleanup;
	}

	Str_View buf_rem = sv_make(buf.data, buf.len);
	bool svr_found = false;
	while (true) {
		Svr svr = {};
		const Find_Res res = parse_svr(&buf_rem, &svr);
		if (res == find_res_err) {
			goto cleanup;
		} if (res == find_res_missing) {
			break;
		}

		if (show_all || svr.players_count) {
			print_svr(&svr);
		}

		if (!svr_found) {
			svr_found = true;
		}
	}
	if (!svr_found) {
		printf("No active servers\n");
	}

	retval = true;

cleanup:
	free(buf.data);
out:
	return retval;
}

static bool parse_size_arg(
	const char *const arg,
	size_t *const out,
	const char *const name
) {
	errno = 0;
	char *end;
	const ulonglong res = strtoull(arg, &end, 10);
	if (errno) {
		perrorf("Invalid %s", name);
		return false;
	}
	if (*end || res >= SIZE_MAX) {
		eprintf("Invalid %s\n", name);
		return false;
	}
	*out = (size_t)res;
	return true;
}

static bool show_lb(const char *const argv[]) {
	bool retval = false;

	size_t count = SIZE_MAX;
	const char *const count_arg = argv[2];
	if (count_arg && !parse_size_arg(count_arg, &count, "player count")) {
		goto out;
	}

	size_t start_idx = 0;
	const char *const start_idx_arg = count_arg ? argv[3] : nullptr;
	if (
		start_idx_arg &&
		!parse_size_arg(start_idx_arg, &start_idx, "start rank")
	) {
		goto out;
	}
	if (start_idx_arg) {
		if (start_idx == 0) {
			eprintf("Invalid start rank\n");
			goto out;
		}
		--start_idx;
	}

	Resp_Buf buf = resp_buf_make(lb_buf_cap);
	if (!buf.data) {
		goto out;
	}

	if(!fetch_lb(&buf)) {
		goto cleanup;
	}

	Str_View body = find_lb_body(sv_make(buf.data, buf.len));
	if (!body.data) {
		goto cleanup;
	}

	for (size_t i = 0; ; ++i) {
		Str_View player_body;
		const Find_Res res = find_lb_player_body(&body, &player_body);
		if (res == find_res_err) {
			goto cleanup;
		} if (res == find_res_missing) {
			break;
		}

		if (i < start_idx) {
			continue;
		}
		if (count != SIZE_MAX && i >= start_idx + count) {
			break;
		}

		Lb_Player player = {};
		if (!parse_lb_player(player_body, &player)) {
			return false;
		}

		print_lb_player(&player, i + 1);
	}

	retval = true;

cleanup:
	free(buf.data);
out:
	return retval;
}

static bool show_rank(const char *const argv[]) {
	typedef enum {
		id_type_username,
		id_type_idx
	} Id_Type;

	typedef union {
		Str_View username;
		size_t idx;
	} Id;

	bool retval = false;

	const char *const arg = argv[2];
	if (!arg) {
		eprintf("Username/rank not given\n");
		goto out;
	}

	Id_Type id_type;
	Id id = {};

	if (*arg == '@') {
		id_type = id_type_idx;

		if (!parse_size_arg(arg + 1, &id.idx, "player rank")) {
			goto out;
		}
		if (id.idx == 0) {
			eprintf("Invalid player rank\n");
			goto out;
		}
		--id.idx;
	} else {
		id_type = id_type_username;

		const size_t username_len = strlen(arg);
		if (username_len < username_min_len) {
			eprintf("Username is too short\n");
			goto out;
		} if (username_len > username_max_len) {
			eprintf("Username is too long\n");
			goto out;
		}
		id.username = sv_make(arg, username_len);
	}

	Resp_Buf buf = resp_buf_make(lb_buf_cap);
	if (!buf.data) {
		goto out;
	}

	if (!fetch_lb(&buf)) {
		goto cleanup;
	}

	Str_View body = find_lb_body(sv_make(buf.data, buf.len));
	if (!body.data) {
		goto cleanup;
	}

	bool player_found = false;
	for (size_t i = 0; ; ++i) {
		Str_View player_body;
		const Find_Res res = find_lb_player_body(&body, &player_body);
		if (res == find_res_err) {
			goto cleanup;
		} if (res == find_res_missing) {
			break;
		}

		if (id_type == id_type_idx && i != id.idx) {
			continue;
		}

		Lb_Player player = {};
		if (!parse_lb_player(player_body, &player)) {
			return false;
		}

		if (
			id_type == id_type_username &&
			!sv_contains_nocase(player.username, id.username)
		) {
			continue;
		}

		print_lb_player(&player, i + 1);
		if (!player_found) {
			player_found = true;
		}

		if (id_type == id_type_idx) {
			break;
		}
	}
	if (!player_found) {
		printf("No players found\n");
	}

	retval = true;

cleanup:
	free(buf.data);
out:
	return retval;
}

bool dispatch_action(const char *const argv[]) {
	const char *const action = argv[1];
	if (!action) {
		eprintf("No action given\n");
		return false;
	}

	if (!strcmp(action, "online")) {
		return show_online_svrs(argv);
	}
	if (!strcmp(action, "lb")) {
		return show_lb(argv);
	}
	if (!strcmp(action, "rank")) {
		return show_rank(argv);
	}

	eprintf("Invalid action\n");

	return false;
}

int main([[maybe_unused]] int argc, const char *const argv[]) {
	return !dispatch_action(argv);
}
