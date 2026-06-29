#include "parser.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "errio.h"
#include "sv_num_conv.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ARR_CAP(arr) (sizeof(arr) / sizeof(*(arr)))

static constexpr size_t max_attr_name_len = 64;
static constexpr size_t max_attr_val_len = 256;

static const char *sv_memmem(const Str_View buf, const Str_View delim) {
	return memmem(buf.data, buf.len, delim.data, delim.len);
}

// returns ptr to first char after delim
static const char *memmem_after(
	const void *const buf,
	const size_t buf_size,
	const void *const delim,
	const size_t delim_size
) {
	if (!buf || !delim || !delim_size || buf_size <= delim_size) {
		return nullptr;
	}
	const char *p = memmem(buf, buf_size - 1, delim, delim_size);
	return p ? p + delim_size : nullptr;
}

static const char *sv_memmem_after(const Str_View buf, const Str_View delim) {
	return memmem_after(buf.data, buf.len, delim.data, delim.len);
}

static const char *memmem_rev(
	const void *const buf,
	const size_t buf_size,
	const void *const delim,
	const size_t delim_size
) {
	if (!buf || !delim || !delim_size || buf_size <= delim_size) {
		return nullptr;
	}
	for (const void *c = buf + buf_size - delim_size; c >= buf; --c) {
		if (!memcmp(c, delim, delim_size)) {
			return c;
		}
	}
	return nullptr;
}

static const char *sv_memmem_rev(const Str_View buf, const Str_View delim) {
	return memmem_rev(buf.data, buf.len, delim.data, delim.len);
}

// also checks whether there exists a server to be parsed
static Find_Res find_svr_body(const Str_View buf, Str_View *const body) {
	constexpr Str_View start_delim = SV("<server>");
	const char *const start = sv_memmem_after(buf, start_delim);
	if (!start) {
		return find_res_missing;
	}

	constexpr Str_View end_delim = SV("</server>");
	size_t buf_len_rem = buf.len - (size_t)(start - buf.data);
	const char *const end = sv_memmem(sv_make(start, buf_len_rem), end_delim);
	if (!end) {
		eprintf("Server body is missing the end\n");
		return find_res_err;
	}

	body->data = start;
	body->len = (size_t)(end - start);

	return find_res_ok;
}

static Str_View find_svr_attrs_body(const Str_View svr_buf) {
	constexpr Str_View start_delim = SV("<server-info");
	const char *const start = sv_memmem_after(svr_buf, start_delim);
	if (!start) {
		eprintf("Failed to find the server attributes body\n");
		return sv_empty;
	}

	constexpr Str_View end_delim = SV("/>");
	const size_t svr_buf_len_rem =
		svr_buf.len - (size_t)(start - svr_buf.data);
	const char *const end = sv_memmem(
		sv_make(start, svr_buf_len_rem),
		end_delim
	);
	if (!end) {
		eprintf("Invalid server attributes body\n");
		return sv_empty;
	}

	const size_t len = (size_t)(end - start);
	return sv_make(start, len);
}

// this sucks 🥸
static Find_Res get_next_attr(
	Str_View *const it,
	Str_View *const name,
	Str_View *const val,
	const bool print_err
) {
	constexpr size_t min_len = 4;
	if (!it->data || it->len < min_len) {
		return find_res_missing;
	}

	const char *const space = memchr(it->data, ' ', it->len - min_len);
	if (!space) {
		return find_res_missing;
	}

	const char *const name_start = space + 1;

	size_t it_len_rem = it->len - (size_t)(name_start - it->data);
	const char *const eq = memchr(name_start, '=', it_len_rem);
	if (!eq) {
		if (print_err) {
			eprintf("Attribute is missing the '='\n");
		}
		return find_res_err;
	}
	const char *const name_end = eq;

	name->data = name_start;
	name->len = (size_t)(name_end - name_start);
	if (name->len > max_attr_name_len) {
		if (print_err) {
			eprintf("Attribute name is too long (%zu)\n", name->len);
		}
		return find_res_err;
	}

	it_len_rem -= (size_t)(eq - name_start);
	if (it_len_rem < 2) {
		if (print_err) {
			eprintf("Attribute ends too early\n");
		}
		return find_res_err;
	}

	if (eq[1] != '"') {
		if (print_err) {
			eprintf("Attribute is missing the starting '\"'\n");
		}
		return find_res_err;
	}
	const char *const val_start = eq + 2;

	const char *const val_end = memchr(val_start, '"', it_len_rem);
	if (!val_end) {
		if (print_err) {
			eprintf("Attribute is missing the ending '\"'\n");
		}
		return find_res_err;
	}

	val->data = val_start;
	val->len = (size_t)(val_end - val->data);

	if (val->len > max_attr_val_len) {
		if (print_err) {
			eprintf("Attribute value is too long (%zu)\n", val->len);
		}
		return find_res_err;
	}

	const char *const new_data = val_end + 1;
	it->len -= (size_t)(new_data - it->data);
	it->data = new_data;

	return find_res_ok;
}

static Str_View get_attr_val_sv(
	const Str_View buf,
	const Str_View target_name,
	const bool print_err
) {	
	Str_View it = buf;
	while (true) {
		Str_View name, val;
		const Find_Res res = get_next_attr(&it, &name, &val, print_err);
		if (res < find_res_ok) {
			if (print_err) {
				eprintf(
					"Failed to get the attribute '%.*s'\n",
					(int)target_name.len,
					target_name.data
				);
			}
			return sv_empty;
		}

		if (sv_eq(name, target_name)) {
			return val;
		}
	}
}

static Str_View get_svr_attr_val(const Str_View buf, const Str_View name) {
	return get_attr_val_sv(buf, name, true);
}

// does not print an error if an attribute is not found
static Str_View get_opt_attr_val(const Str_View buf, const Str_View name) {
	return get_attr_val_sv(buf, name, false);
}

static Str_View parse_svr_attrs(const Str_View svr_body, Svr *const svr) {
	const Str_View body = find_svr_attrs_body(svr_body);
	if (!body.data) {
		return sv_empty;
	}

	constexpr Str_View name_key         = SV("name");
	constexpr Str_View curr_track_key   = SV("current_track");
	constexpr Str_View country_code_key = SV("country_code");
	constexpr Str_View max_players_key  = SV("max_players");
	constexpr Str_View bots_count_key   = SV("current_ai");
	constexpr Str_View mode_key         = SV("game_mode");
	constexpr Str_View difficulty_key   = SV("difficulty");
	constexpr Str_View dist_km_key      = SV("distance");
	constexpr Str_View priv_key         = SV("password");

	svr->name = get_svr_attr_val(svr_body, name_key);
	if (!svr->name.data) {
		return sv_empty;
	}

	svr->curr_track = get_svr_attr_val(svr_body, curr_track_key);
	if (!svr->curr_track.data) {
		return sv_empty;
	}

	svr->game_started = svr->curr_track.len > 0;

	svr->country_code = get_svr_attr_val(svr_body, country_code_key);
	if (!svr->country_code.data) {
		return sv_empty;
	}

	const Str_View priv_sv = get_svr_attr_val(svr_body, priv_key);
	if (!priv_sv.data) {
		return sv_empty;
	}
	svr->priv = priv_sv.len > 0 && *priv_sv.data == '1';
	
	const Str_View mode_sv = get_svr_attr_val(svr_body, mode_key);
	const Str_View difficulty_sv = get_svr_attr_val(svr_body, difficulty_key);
	if (!mode_sv.data || !difficulty_sv.data) {
		return sv_empty;
	}

	const char mode_ch = *mode_sv.data;
	const char difficulty_ch = *difficulty_sv.data;

	if (!isdigit(mode_ch)) {
		eprintf("Invalid game mode (%c)\n", *mode_sv.data);
		return sv_empty;
	}
	if (!isdigit(*difficulty_sv.data)) {
		eprintf("Invalid game difficulty (%c)\n", *difficulty_sv.data);
		return sv_empty;
	}

	svr->mode = (Svr_Mode)mode_ch - '0';
	svr->difficulty = (Svr_Mode)difficulty_ch - '0';

	const Str_View max_players_sv = get_svr_attr_val(svr_body, max_players_key);
	const Str_View bots_count_sv = get_svr_attr_val(svr_body, bots_count_key);
	const Str_View dist_km_sv = get_svr_attr_val(svr_body, dist_km_key);
	if (!max_players_sv.data || !bots_count_sv.data || !dist_km_sv.data) {
		return sv_empty;
	}

	if (!sv_to_size(max_players_sv, &svr->max_players)) {
		eprintf("Failed to parse server's max players\n");
		return sv_empty;
	}
	if (!sv_to_size(bots_count_sv, &svr->bots_count)) {
		eprintf("Failed to parse server's bots count\n");
		return sv_empty;
	}
	if (!sv_to_uint(dist_km_sv, &svr->dist_km)) {
		eprintf("Failed to parse server's distance\n");
		return sv_empty;
	}

	const char *const after = body.data + body.len;
	const size_t svr_body_len_rem =
		svr_body.len - (size_t)(after - svr_body.data);
	return sv_make(after, svr_body_len_rem);
}

static Find_Res find_players_body(
	const Str_View svr_body_rem,
	Str_View *const players_body
) {
	constexpr Str_View start_delim = SV("<players");
	const char *const start = sv_memmem_after(svr_body_rem, start_delim);
	if (!start) {
		eprintf("Players body not found\n");
		return find_res_err;
	}

	if (*start == '/') {
		return find_res_missing;
	}

	constexpr Str_View end_delim = SV("</players>");
	const size_t len_rem =
		svr_body_rem.len - (size_t)(start - svr_body_rem.data);
	const char *const end = sv_memmem(sv_make(start, len_rem), end_delim);
	if (!end) {
		eprintf("Invalid players body\n");
		return find_res_err;
	}

	*players_body = sv_make(start, (size_t)(end - start));
	return find_res_ok;
}

// also checks whether there exists a player to be parsed
static Find_Res find_player_body(
	const Str_View players_body,
	Str_View *const player_body
) {
	constexpr Str_View start_delim = SV("<player-info");
	const char *const start = sv_memmem_after(players_body, start_delim);
	if (!start) {
		return find_res_missing;
	}

	constexpr Str_View end_delim = SV("/>");
	const size_t players_body_len_rem =
		players_body.len - (size_t)(start - players_body.data);
	const char *const end = sv_memmem(
		sv_make(start, players_body_len_rem),
		end_delim
	);
	if (!end) {
		eprintf("Invalid player body\n");
		return find_res_err;
	}

	*player_body = sv_make(start, (size_t)(end - start));

	return find_res_ok;
}

static bool parse_player_attrs(
	const Str_View player_body,
	Player *const player
) {
	constexpr Str_View username_key       = SV("username");
	constexpr Str_View country_code_key   = SV("country-code");
	constexpr Str_View races_count_key    = SV("num-races-done");
	constexpr Str_View pts_key            = SV("scores");
	constexpr Str_View rank_key           = SV("rank");
	constexpr Str_View minutes_played_key = SV("time-played");

	player->username = get_svr_attr_val(player_body, username_key);
	if (!player->username.data) {
		return false;
	}

	player->country_code = get_svr_attr_val(player_body, country_code_key);
	if (!player->country_code.data) {
		return false;
	}

	const Str_View minutes_played_sv =
		get_svr_attr_val(player_body, minutes_played_key);
	if (!minutes_played_sv.data) {
		return false;
	}
	if (!sv_to_uint(minutes_played_sv, &player->minutes_played)) {
		eprintf("Failed to parse player's playtime\n");
		return false;
	}

	const Str_View pts_sv = get_opt_attr_val(player_body, pts_key);
	const Str_View rank_sv = get_opt_attr_val(player_body, rank_key);
	const Str_View races_count_sv = get_opt_attr_val(
		player_body,
		races_count_key
	);
	if (!pts_sv.data || !rank_sv.data || !races_count_sv.data) {
		player->rank = 0;
		player->races_count = 0;
		return true; // unranked player doesn't have those attributes
	}

	if (!sv_to_long(pts_sv, &player->pts)) {
		eprintf("Failed to parse player's rating points\n");
		return false;
	}
	if (!sv_to_size(rank_sv, &player->rank)) {
		eprintf("Failed to parse player's rank\n");
		return false;
	}
	if (!sv_to_size(races_count_sv, &player->races_count)) {
		eprintf("Failed to parse player's race count\n");
		return false;
	}

	return true;
}

static Find_Res parse_player(
	Str_View *const players_body,
	Player *const player
) {
	Str_View player_body = {};
	const Find_Res res = find_player_body(*players_body, &player_body);
	if (res < find_res_ok) {
		return res;
	}

	if (!parse_player_attrs(player_body, player)) {
		return find_res_err;
	}

	const char *const after = player_body.data + player_body.len;
	const size_t len_rem =
		players_body->len - (size_t)(after - players_body->data);
	*players_body = sv_make(after, len_rem);

	return find_res_ok;
}

Find_Res parse_svr(Str_View *const buf_rem, Svr *const svr) {
	Str_View body = {};
	Find_Res res = find_svr_body(*buf_rem, &body);
	if (res < find_res_ok) {
		return res;
	}
	
	const Str_View body_rem = parse_svr_attrs(body, svr);
	if (!body_rem.data) {
		return find_res_err;
	}

	Str_View players_body = {};
	res = find_players_body(body_rem, &players_body);
	if (res == find_res_err) {
		return find_res_err;
	} if (res == find_res_missing) {
		goto players_skip;
	}

	size_t i = 0;
	for (; i < ARR_CAP(svr->players); ++i) {
		Player *const player = &svr->players[i];

		const Find_Res res = parse_player(&players_body, player);
		if (res == find_res_err) {
			return find_res_err;
		} if (res == find_res_missing) {
			break;
		}
	}
	svr->players_count = i;

players_skip:
	const char *const after = body.data + body.len;
	const size_t buf_len_rem = buf_rem->len - body.len;
	*buf_rem = sv_make(after, buf_len_rem);

	return find_res_ok;
}

Str_View find_lb_body(const Str_View buf) {
	constexpr Str_View start_delim = SV("<tbody>");
	const char *const start = sv_memmem_after(buf, start_delim);
	if (!start) {
		eprintf("Leaderboard body is missing\n");
		return sv_empty;
	}

	constexpr Str_View end_delim = SV("</tbody>");
	const size_t buf_len_rem = buf.len - (size_t)(start - buf.data);
	const char *const end = sv_memmem_rev(
		sv_make(start, buf_len_rem),
		end_delim
	);
	if (!end) {
		eprintf("Leaderboard is missing the end\n");
		return sv_empty;
	}

	const size_t len = (size_t)(end - start);
	return sv_make(start, len);
}

Find_Res find_lb_player_body(
	Str_View *const body_rem,
	Str_View *const player_body
) {
	constexpr Str_View start_delim = SV("<tr>");
	const char *const start = sv_memmem_after(*body_rem, start_delim);
	if (!start) {
		return find_res_missing;
	}

	constexpr Str_View end_delim = SV("</tr>");
	const size_t body_len_rem =
		body_rem->len - (size_t)(start - body_rem->data);
	const char *const end = sv_memmem(
		sv_make(start, body_len_rem),
		end_delim
	);
	if (!end) {
		eprintf("Player body is missing the end\n");
		return find_res_err;
	}

	player_body->data = start;
	player_body->len = (size_t)(end - start);

	const char *const new_body_data = player_body->data + player_body->len;
	const size_t new_body_len =
		body_rem->len - (size_t)(new_body_data - body_rem->data);
	*body_rem = sv_make(new_body_data, new_body_len);

	return find_res_ok;
}

static Str_View get_lb_player_attr_val(Str_View *const it) {
	constexpr Str_View start_delim = SV("<th>");
	const char *const start = sv_memmem_after(*it, start_delim);
	if (!start) {
		eprintf("Player attribute is missing\n");
		return sv_empty;
	}

	constexpr Str_View end_delim = SV("</th>");
	const size_t it_len_rem = it->len - (size_t)(start - it->data);
	const char *const end = sv_memmem(
		sv_make(start, it_len_rem),
		end_delim
	);
	if (!end) {
		eprintf("Player attribute is missing the end\n");
		return sv_empty;
	}

	const size_t val_len = (size_t)(end - start);

	const char *const new_it_data = end + end_delim.len;
	const size_t new_it_len = it_len_rem - val_len - end_delim.len;
	*it = sv_make(new_it_data, new_it_len);

	return sv_make(start, val_len);
}

bool parse_lb_player(const Str_View player_body, Lb_Player *const player) {
	Str_View it = player_body;

	if (
		!get_lb_player_attr_val(&it).data || // skipping the rank
		!(player->username = get_lb_player_attr_val(&it)).data
	) {
		return false;
	}
	if (player->username.len > username_max_len) {
		eprintf("Player username is too long (%zu)\n", player->username.len);
	}

	Str_View pts, pts_max;
	Str_View races_count, rating_deviation, disconn_rate;
	if (
		!(pts              = get_lb_player_attr_val(&it)).data ||
		!(pts_max          = get_lb_player_attr_val(&it)).data ||
		!(races_count      = get_lb_player_attr_val(&it)).data ||
		!(rating_deviation = get_lb_player_attr_val(&it)).data ||
		!(disconn_rate     = get_lb_player_attr_val(&it)).data
	) {
		return false;
	}

	--disconn_rate.len; // removing the '%'

	if (
		!sv_to_long_decimal(pts, &player->pts) ||
		!sv_to_long_decimal(pts_max, &player->pts_max) ||
		!sv_to_ulong_decimal(rating_deviation, &player->rating_deviation) ||
		!sv_to_uint_decimal(disconn_rate, &player->disconn_rate) ||
		!sv_to_size(races_count, &player->races_count)
	) {
		return false;
	}

	return true;
}
