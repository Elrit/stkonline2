#pragma once

#define COLOR_RESET "\e[0m"

#define COLOR_BLACK  "\e[0;30m"
#define COLOR_RED    "\e[0;31m"
#define COLOR_GREEN  "\e[0;32m"
#define COLOR_YELLOW "\e[0;33m"
#define COLOR_BLUE   "\e[0;34m"
#define COLOR_PURPLE "\e[0;35m"
#define COLOR_CYAN   "\e[0;36m"
#define COLOR_WHITE  "\e[0;37m"

#define COLOR_BOLD_BLACK  "\e[1;30m"
#define COLOR_BOLD_RED    "\e[1;31m"
#define COLOR_BOLD_GREEN  "\e[1;32m"
#define COLOR_BOLD_YELLOW "\e[1;33m"
#define COLOR_BOLD_BLUE   "\e[1;34m"
#define COLOR_BOLD_PURPLE "\e[1;35m"
#define COLOR_BOLD_CYAN   "\e[1;36m"
#define COLOR_BOLD_WHITE  "\e[1;37m"

#define COLOR_UNDERLINE_BLACK  "\e[4;30m"
#define COLOR_UNDERLINE_RED    "\e[4;31m"
#define COLOR_UNDERLINE_GREEN  "\e[4;32m"
#define COLOR_UNDERLINE_YELLOW "\e[4;33m"
#define COLOR_UNDERLINE_BLUE   "\e[4;34m"
#define COLOR_UNDERLINE_PURPLE "\e[4;35m"
#define COLOR_UNDERLINE_CYAN   "\e[4;36m"
#define COLOR_UNDERLINE_WHITE  "\e[4;37m"

// CONFIG:

#define TERMINAL_COLORS 0

constexpr char color_country[]        = COLOR_BOLD_RED;
constexpr char color_svr_dist[]       = COLOR_BOLD_BLUE;
constexpr char color_svr_name[]       = COLOR_BOLD_GREEN;
constexpr char color_player_count[]   = COLOR_BOLD_CYAN;
constexpr char color_svr_mode[]       = COLOR_BOLD_PURPLE;
constexpr char color_svr_difficulty[] = COLOR_BOLD_PURPLE;
constexpr char color_track[]          = COLOR_BOLD_YELLOW;
constexpr char color_username[]       = COLOR_BOLD_CYAN;
constexpr char color_play_time[]      = COLOR_WHITE;
constexpr char color_ranking_info[]   = COLOR_BOLD_WHITE;

constexpr char color_lb_rank[]       = COLOR_BOLD_RED;
constexpr char color_lb_username[]   = COLOR_BOLD_CYAN;
constexpr char color_lb_pts[]        = COLOR_BOLD_GREEN;
constexpr char color_lb_race_count[] = COLOR_BOLD_BLUE;
constexpr char color_lb_deviation[]  = COLOR_BOLD_PURPLE;
constexpr char color_lb_disconn[]    = COLOR_BOLD_YELLOW;

constexpr char priv_svr_indicator[] = "🔒";
constexpr char indent[] = "\t";
