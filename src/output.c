#include "output.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

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
	constexpr char indent[] = "\t";
	printf(indent);

	printf("(%.*s)", (int)player->country_code.len, player->country_code.data);
	printf(
		" %.*s [%umin]",
		(int)player->username.len,
		player->username.data,
		player->minutes_played
	);

	if (player->rank > 0) {
		printf(
			" [#%ld, %ldpts, %zu races]",
			player->rank,
			player->pts,
			player->races_count
		);
	}

	putchar('\n');
}

void print_svr(const Svr *const svr) {
	printf(
		"(%s%.*s/%ukm)",
		svr->priv ? "private " : "",
		(int)svr->country_code.len,
		svr->country_code.data,
		svr->dist_km
	);

	printf(" %.*s\n", (int)svr->name.len, svr->name.data);

	printf("(%zu", svr->players_count);
	if (svr->bots_count) {
		printf("+%zu", svr->bots_count);
	}
	printf("/%zu)", svr->max_players);

	const char *const mode = svr_mode_to_str(svr->mode);
	const char *const diff = svr_difficulty_to_str(svr->difficulty);
	printf(" [%s, %s]", mode, diff);

	if (svr->curr_track.len) {
		printf(" - '%.*s'", (int)svr->curr_track.len, svr->curr_track.data);
	}

	putchar('\n');

	for (size_t i = 0; i < svr->players_count; ++i) {
		print_player(&svr->players[i]);
	}

	putchar('\n');
}

// num must fit in intmax_t
#define PRINT_DECIMAL(num, int_part_w, start_str, end_str) \
	do { \
		constexpr intmax_t div_ = 100; \
		const intmax_t n_ = (intmax_t)num; \
		printf( \
			start_str "%*jd.%.*jd" end_str, \
			(int)int_part_w, \
			n_ / div_, \
			(int)decimal_digits_count, \
			ABS(n_ % div_) \
		); \
	} while (false)

void print_lb_player(const Lb_Player *const player, const size_t rank) {
	constexpr int rank_w = 5;
	printf("%*zu.", rank_w, rank);

	const int written = printf(
		" %.*s ",
		(int)player->username.len,
		player->username.data
	);
	// skipping first char
	for (size_t i = (size_t)written + 1; i < username_max_len; ++i) {
		putchar('.');
	}

	constexpr int pts_int_part_w = 5;
	PRINT_DECIMAL(player->pts, pts_int_part_w, "  ", " pts");
	PRINT_DECIMAL(player->pts_max, pts_int_part_w, "  ", " max");

	constexpr int races_count_w = 5;
	printf("  %*zu races", races_count_w, player->races_count);

	constexpr int dev_int_part_w = 5;
	PRINT_DECIMAL(player->rating_deviation, dev_int_part_w, "  ", " dev");

	constexpr int drate_int_part_w = 3;
	PRINT_DECIMAL(player->disconn_rate, drate_int_part_w, "  ", "%% disc");

	putchar('\n');
}
