#include "output.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "errio.h"

#define ABS(x)    ((x) >= 0 ? (x) : -(x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void set_color(const char *const color) {
#if TERMINAL_COLORS
	printf("%s", color);
#else
	(void)color;
#endif
}

static void reset_color(void) {
#if TERMINAL_COLORS
	printf("%s", COLOR_RESET);
#endif
}

static const char *svr_mode_to_str(const Svr_Mode m) {
	switch (m) {
		case svr_mode_grand_prix_race:
			return "Grand Prix (Race)";
		case svr_mode_grand_prix_timetrial:
			return "Grand Prix (Time Trial)";
		case svr_mode_race:
			return "Race";
		case svr_mode_time_trial:
			return "Time Trial";
		case svr_mode_soccer:
			return "Soccer";
		case svr_mode_free_for_all:
			return "FFA";
		case svr_mode_capture_the_flag:
			return "CTF";
		default:
			return "Unknown Mode";
	}
}

static const char *svr_difficulty_to_str(const Svr_Difficulty d) {
	switch (d) {
		case svr_difficulty_novice:
			return "Novice";
		case svr_difficulty_intermediate:
			return "Intermediate";
		case svr_difficulty_expert:
			return "Expert";
		case svr_difficulty_supertux:
			return "SuperTux";
		default:
			return "Unknown Difficulty";
	}
}

static void print_player(const Player *const player) {
	printf("%s", indent);

	set_color(color_country);
	printf(
		"(%.*s)",
		(int)player->country_code.len,
		player->country_code.data
	);
	reset_color();

	putchar(' ');

	set_color(color_username);
	printf("%.*s", (int)player->username.len, player->username.data);
	reset_color();

	putchar(' ');

	set_color(color_play_time);
	printf("[%um]", player->minutes_played);
	reset_color();

	if (player->rank) {
		putchar(' ');

		set_color(color_ranking_info);
		printf(
			"[#%ld, %ldpts, %zu races]",
			player->rank,
			player->pts,
			player->races_count
		);
		reset_color();
	}

	putchar('\n');
}

static bool print_html_char(Str_View buf) {
	int base = 10;
	if (tolower(*buf.data) == 'x') {
		base = 16;
		++buf.data;
		--buf.len;
	} else if (!isdigit((uchar)*buf.data)) {
		return false;
	}

	int code = 0;
	for (size_t i = 0; i < buf.len; ++i) {
		const uchar c = (uchar)tolower(buf.data[i]);

		if (base == 10) {
			if (!isdigit(c)) {
				return false;
			}
		} else {
			if (!isxdigit(c)) {
				return false;
			}
		}

		int digit;
		if (isdigit(c)) {
			digit = c - '0';
		} else if (isxdigit(c)) {
			digit = c - 'a' + 10;
		} else {
			return false;
		}

		code = (code * base) + digit;
	}

	uchar bytes[4] = {};
	size_t count;

	if (code <= 0x7F) {
		count = 1;
		bytes[0] = (uchar)code;
	} else if (code <= 0x7FF) {                                         // 11
		bytes[0] = (uchar)(0b11000000 | (code >> 6));                 // 5
		bytes[1] = (uchar)(0b10000000 | (code & 0b00111111));         // 6
		count = 2;
	} else if (code <= 0xFFFF) {                                        // 16
		bytes[0] = (uchar)(0b11100000 | (code >> 12));                // 4
		bytes[1] = (uchar)(0b10000000 | ((code >> 6) & 0b00111111));  // 6
		bytes[2] = (uchar)(0b10000000 | (code & 0b00111111));         // 6
		count = 3;
	} else if (code <= 0x1FFFFF) {                                      // 21
		bytes[0] = (uchar)(0b11110000 | (code >> 18));                // 3
		bytes[1] = (uchar)(0b10000000 | ((code >> 12) & 0b00111111)); // 6
		bytes[2] = (uchar)(0b10000000 | ((code >> 6) & 0b00111111));  // 6
		bytes[3] = (uchar)(0b10000000 | (code & 0b00111111));         // 6
		count = 4;
	} else {
		return false;	
	}

	for (size_t i = 0; i < count; ++i) {
		putchar(bytes[i]);
	}

	return true;
}

static void print_html_str(const Str_View buf) {
	for (size_t i = 0; i < buf.len; ++i) {
		const char *const c = (char *)(buf.data + i);

		constexpr char delim[] = "&#";
		constexpr size_t delim_len = sizeof(delim) - 1;

		if (buf.len - i < delim_len + 2 || memcmp(c, delim, delim_len)) {
			putchar(*c);
			continue;
		}

		const char *const start = c + delim_len;
		const size_t buf_len_rem = buf.len - (size_t)(start - buf.data);

		constexpr size_t max_digits = 7;
		const size_t search_len = MIN(max_digits, buf_len_rem);
		const char *const end = memchr(start, ';', search_len);
		if (!end) {
			goto err;
		}

		const Str_View html_char = sv_make(start, (size_t)(end - start));
		if (!print_html_char(html_char)) {
			goto err;
		}

		i += delim_len + html_char.len;
	}

	return;
err:
	eprintf("\nInvalid HTML character\n");
}

void print_svr(const Svr *const svr) {
	putchar('(');

	if (svr->priv) {
		printf("%s ", priv_svr_indicator);
	}

	set_color(color_country);
	printf("%.*s", (int)svr->country_code.len, svr->country_code.data);
	reset_color();

	putchar('/');

	set_color(color_svr_dist);
	printf("%ukm", svr->dist_km);
	reset_color();

	printf(") ");

	set_color(color_svr_name);
	print_html_str(svr->name);
	reset_color();

	putchar('\n');

	set_color(color_player_count);
	printf("(%zu", svr->players_count);
	if (svr->bots_count) {
		printf("+%zu", svr->bots_count);
	}
	printf("/%zu)", svr->max_players);
	reset_color();

	printf(" [");

	set_color(color_svr_mode);
	printf("%s", svr_mode_to_str(svr->mode));
	reset_color();

	printf(", ");

	set_color(color_svr_difficulty);
	printf("%s", svr_difficulty_to_str(svr->difficulty));
	reset_color();

	putchar(']');

	if (svr->curr_track.len) {
		printf(" - ");

		set_color(color_track);
		printf(
			"%.*s",
			(int)svr->curr_track.len,
			svr->curr_track.data
		);
		reset_color();
	}

	putchar('\n');

	for (size_t i = 0; i < svr->players_count; ++i) {
		print_player(&svr->players[i]);
	}

	putchar('\n');
}

// num must fit in intmax_t
#define PRINT_DECIMAL(num, int_part_w) \
	do { \
		constexpr intmax_t div_ = 100; \
		const intmax_t n_ = (intmax_t)num; \
		printf( \
			"%*jd.%.*jd", \
			(int)int_part_w, \
			n_ / div_, \
			(int)decimal_digits_count, \
			ABS(n_ % div_) \
		); \
	} while (false)

void print_lb_player(const Lb_Player *const player, const size_t rank) {
	set_color(color_lb_rank);
	constexpr int rank_w = 5;
	printf("%*zu.", rank_w, rank);
	reset_color();

	putchar(' ');

	set_color(color_lb_username);
	const int written = printf(
		"%.*s",
		(int)player->username.len,
		player->username.data
	);
	reset_color();

	putchar(' ');
	for (size_t i = (size_t)written + 1; i < username_max_len; ++i) {
		putchar('.');
	}

	printf("  ");

	constexpr int pts_int_part_w = 5;

	set_color(color_lb_pts);
	PRINT_DECIMAL(player->pts, pts_int_part_w);
	reset_color();
	printf(" pts");

	printf("  ");

	set_color(color_lb_pts);
	PRINT_DECIMAL(player->pts_max, pts_int_part_w);
	reset_color();
	printf(" max");

	printf("  ");

	set_color(color_lb_race_count);
	constexpr int races_count_w = 5;
	printf("%*zu", races_count_w, player->races_count);
	reset_color();
	printf(" races");

	printf("  ");

	set_color(color_lb_deviation);
	constexpr int dev_int_part_w = 5;
	PRINT_DECIMAL(player->rating_deviation, dev_int_part_w);
	reset_color();
	printf(" dev");

	printf("  ");

	set_color(color_lb_disconn);
	constexpr int drate_int_part_w = 3;
	PRINT_DECIMAL(player->disconn_rate, drate_int_part_w);
	reset_color();
	printf(" disc");

	putchar('\n');
}
