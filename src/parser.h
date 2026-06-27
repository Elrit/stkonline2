#pragma once

#include "sv.h"
#include "types.h"

typedef enum {
	find_res_err,
	find_res_missing,
	find_res_ok
} Find_Res;

Find_Res parse_svr(Str_View *const buf_rem, Svr *const svr);

Str_View find_lb_body(const Str_View body);
Find_Res find_lb_player_body(
	Str_View *const body_rem,
	Str_View *const player_body
);
bool parse_lb_player(const Str_View player_body, Lb_Player *const player);
