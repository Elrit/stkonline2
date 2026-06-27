#pragma once

#include <stddef.h>

#include "int_aliases.h"
#include "sv.h"

constexpr size_t username_min_len = 3;
constexpr size_t username_max_len = 30;
constexpr size_t decimal_digits_count = 2;

typedef struct {
	Str_View username;
	Str_View country_code;
	size_t races_count;
	long pts;
	long rank;
	uint minutes_played;
} Player;

typedef enum {
	svr_mode_grand_prix_race      = 0,
	svr_mode_grand_prix_timetrial = 1,
	svr_mode_race                 = 3,
	svr_mode_time_trial           = 4,
	svr_mode_soccer               = 6,
	svr_mode_free_for_all         = 7,
	svr_mode_capture_the_flag     = 8
} Svr_Mode;

typedef enum {
	svr_difficulty_novice,
	svr_difficulty_intermediate,
	svr_difficulty_expert,
	svr_difficulty_supertux,
} Svr_Difficulty;

typedef struct {
	Player players[64];
	Str_View name;
	Str_View curr_track;	
	Str_View country_code;
	size_t max_players;
	size_t players_count;
	size_t bots_count;
	Svr_Mode mode;
	Svr_Difficulty difficulty;
	uint dist_km;
	bool priv;
	bool game_started;
} Svr;

typedef struct {
	Str_View username;
	size_t races_count;
	long pts;
	long pts_max;
	ulong rating_deviation;
	uint disconn_rate;
} Lb_Player;
