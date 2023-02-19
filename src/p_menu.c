#include "g_local.h"
#include "p_menu.h"
#include "version.h"

//voting things

#define CHANGED_MAP			VOTE_MAP
#define CHANGED_TIMELIMIT	VOTE_TIMELIMIT
#define CHANGED_BFG			VOTE_BFG
#define CHANGED_POWERUPS	VOTE_POWERUPS
#define CHANGED_KICK		VOTE_KICK
#define CHANGED_DMFLAGS		VOTE_DMFLAGS
#define CHANGED_FASTWEAPONS VOTE_FASTWEAPONS
#define CHANGED_CONFIG		VOTE_CONFIG
#define CHANGED_TP			VOTE_TP
#define CHANGED_HUD			VOTE_HUD

const struct dmflags_t dmflags_table [NUM_DMFLAGS] =
{
//	name					flags
	{ "[SELECT]",			0},
	{ "No Health",			DF_NO_HEALTH},
	{ "No Items",			DF_NO_ITEMS},
	{ "Weapons Stay",		DF_WEAPONS_STAY},
	{ "No Falling",			DF_NO_FALLING},
	{ "Instant Items",		DF_INSTANT_ITEMS},
	{ "Same Level",			DF_SAME_LEVEL},
	{ "Spawn NewTDM",		DF_SPAWN_NEWTDM},
	{ "128: UNUSED",		DF_SPAWN_RANDOM},
	{ "No Friendly Fire",		DF_NO_FRIENDLY_FIRE},
	{ "Spawn Farthest",		DF_SPAWN_FARTHEST},
	{ "Force Respawn",		DF_FORCE_RESPAWN},
	{ "No Armor",			DF_NO_ARMOR},
	{ "Allow Exit",			DF_ALLOW_EXIT},
	{ "Infinite Ammo",		DF_INFINITE_AMMO},
	{ "Quad Drop",			DF_QUAD_DROP},
	{ "Fixed Fov",			DF_FIXED_FOV}
};
//end

const struct menu menus [16] =
{
//	name				index			prev_men		num_items
	{ "Main Menu",		MAIN_MENU,		-1,				7},
	{ "Voting Menu",	VOTE_MENU,		MAIN_MENU,		12},
	{ "Credits",		CREDITS,		MAIN_MENU,		2},
	{ "Contact",		CONTACT,		CREDITS,		1},
	{ "Admin help1",	ADMIN_HELP_1,	-1,				2},
	{ "Admin help2",	ADMIN_HELP_2,	ADMIN_HELP_1,	2},
	{ "Admin help3",	ADMIN_HELP_3,	ADMIN_HELP_2,	2},
	{ "Admin help4",	ADMIN_HELP_4,	ADMIN_HELP_3,	2},
	{ "Admin help5",	ADMIN_HELP_5,	ADMIN_HELP_4,	2},
	{ "Commands 1",		COMMANDS_1,		MAIN_MENU,		2},
	{ "Commands 2",		COMMANDS_2,		COMMANDS_1,		2},
	{ "Commands 3",		COMMANDS_3,		COMMANDS_2,		2},
	{ "Commands 4",		COMMANDS_4,		COMMANDS_3,		2},
	{ "Commands 5",		COMMANDS_5,		COMMANDS_4,		2},
	{ "Commands 6",		COMMANDS_6,		COMMANDS_5,		2},
	{ "Commands 7",		COMMANDS_7,		COMMANDS_6,		2},
};

#define	NUM_MI	222

const struct menu_items menu_items_table [NUM_MI] =
{
//	name							menu		item	sub_men	t_dist	l_dist	func				flags
	{ TDMVERSION,					MAIN_MENU,	-1,		-1,		1,		2,		NULL,				MI_CENTER},
	{ "%s",							MAIN_MENU,	-1,		-1,		3,		5,		NULL,				MI_MAPNAME | MI_CENTER},
	{ "%s %s",						MAIN_MENU,	0,		-1,		5,		2,		join_team_a,		0},
	{ "(%i)",						MAIN_MENU,	-1,		-1,		5,		23,		NULL,				MI_APLAYERS},
	{ "%s %s",						MAIN_MENU,	1,		-1,		6,		2,		join_team_b,		0},
	{ "(%i)",						MAIN_MENU,	-1,		-1,		6,		23,		NULL,				MI_BPLAYERS},
	{ "%s",							MAIN_MENU,	2,		-1,		8,		2,		SpectateOrChase,	0},
	{ "(%i)",						MAIN_MENU,	-1,		-1,		8,		23,		NULL,				MI_SPLAYERS},
	{ "Voting Menu",				MAIN_MENU,	3,		VOTE_MENU,		9,		2,		submenu_enter,		0},
	{ "TDM Commands",				MAIN_MENU,	4,		COMMANDS_1,		10,		2,		submenu_enter,		0},
	{ "Credits",					MAIN_MENU,	5,		CREDITS,		11,		2,		submenu_enter,		0},
	{ "Return to game",				MAIN_MENU,	6,		-1,		12,		2,		menu_off,			0},
	{ "Use [ and ] to select",		MAIN_MENU,	-1,		-1,		16,		2,		NULL,				0},
	{ "ENTER to accept",			MAIN_MENU,	-1,		-1,		17,		2,		NULL,				0},
	{ "TAB to open/exit menu",		MAIN_MENU,	-1,		-1,		18,		2,		NULL,				0},
	{ "http://tdm.quake2.com.pl",		MAIN_MENU,	-1,		-1,		22,		0,		NULL,				MI_CENTER},

	{ "Voting Menu",				VOTE_MENU,	-1,		-1,		1,		2,		NULL,				MI_CENTER},
	{ "Map: %s",					VOTE_MENU,	0,		-1,		3,		2,		change_map,			MI_OPTIONS},
	{ "Timelimit: %i",				VOTE_MENU,	1,		-1,		4,		2,		change_timelimit,	MI_OPTIONS},
	{ "TP: %i",						VOTE_MENU,	2,		-1,		5,		2,		change_tp,			MI_OPTIONS},
	{ "Allow BFG: %s",				VOTE_MENU,	3,		-1,		6,		2,		change_bfg,			MI_OPTIONS},
	{ "Allow Powerups: %s",			VOTE_MENU,	4,		-1,		7,		2,		change_powerups,	MI_OPTIONS},
	{ "Allow Hud: %s",				VOTE_MENU,	5,		-1,		8,		2,		change_hud,			MI_OPTIONS},
	{ "Fastweapons: %s",			VOTE_MENU,	6,		-1,		9,		2,		change_fastweapons,	MI_OPTIONS},
	{ "Kick: %s",					VOTE_MENU,	7,		-1,		10,		2,		change_kick,		MI_OPTIONS},
	{ "Config: %s",					VOTE_MENU,	8,		-1,		11,		2,		change_config,		MI_OPTIONS},
	{ "DMFlags:",					VOTE_MENU,	-1,		-1,		12,		2,		NULL,				0},
	{ "%s: %s",						VOTE_MENU,	9,		-1,		13,		2,		change_dmflags,		MI_OPTIONS},
	{ "Propose changes",			VOTE_MENU,	10,		-1,		15,		2,		proposal_changes,	0},
	{ "Return to Main Menu",		VOTE_MENU,	11,		-1,		16,		2,		submenu_leave,		0},

	{ "Credits",					CREDITS,	-1,		-1,		2,		0,		NULL,				MI_CENTER},
	{ "Design:",					CREDITS,	-1,		-1,		5,		0,		NULL,				MI_CENTER | MI_GREEN},
	{ "Marek 'onimusha' Reda",		CREDITS,	-1,		-1,		6,		0,		NULL,				MI_CENTER},
	{ "Code:",						CREDITS,	-1,		-1,		8,		0,		NULL,				MI_CENTER | MI_GREEN},
	{ "Mateusz 'Harven' Kwasniewski",	CREDITS,	-1,		-1,		9,		0,		NULL,				MI_CENTER},
	{ "Beta Testers:",				CREDITS,	-1,		-1,		11,		0,		NULL,				MI_CENTER | MI_GREEN},
	{ "As_best, onimusha, Harven",		CREDITS,	-1,		-1,		12,		0,		NULL,				MI_CENTER},
	{ "Contact",					CREDITS,	0,		CONTACT,		14,		0,		submenu_enter,		MI_CENTER},
	{ "Return to Main Menu",		CREDITS,	1,		-1,		18,		2,		submenu_leave,		MI_CENTER},

	{ "Contact",					CONTACT,	-1,		-1,		2,		0,		NULL,				MI_CENTER},
	{ "onimusha: oni@akpl.pl",		CONTACT,	-1,		-1,		4,		1,		NULL,				MI_GREEN},
	{ "GG 8884",					CONTACT,	-1,		-1,		5,		1,		NULL,				0},
	{ "Harven: harven@gingers.rulez.pl",	CONTACT,	-1,		-1,		7,		1,		NULL,				MI_GREEN},
	{ "GG 9333",					CONTACT,	-1,		-1,		8,		1,		NULL,				0},
	{ "As_best: as_best@o2.pl",		CONTACT,	-1,		-1,		10,		1,		NULL,				MI_GREEN},
	{ "Return to Credits",			CONTACT,	0,		-1,		18,		2,		submenu_leave,		0},

	{ "acommands -",				ADMIN_HELP_1, -1,	-1,		1,		1,		NULL,				MI_GREEN},
	{ "Display this help.",			ADMIN_HELP_1, -1,	-1,		2,		1,		NULL,				0},
	{ "ban -",						ADMIN_HELP_1, -1,	-1,		3,		1,		NULL,				MI_GREEN},
	{ "Ban given IP.",				ADMIN_HELP_1, -1,	-1,		4,		1,		NULL,				0},
	{ "banlist -",					ADMIN_HELP_1, -1,	-1,		5,		1,		NULL,				MI_GREEN},
	{ "List baned players.",		ADMIN_HELP_1, -1,	-1,		6,		1,		NULL,				0},
	{ "bfg -",						ADMIN_HELP_1, -1,	-1,		7,		1,		NULL,				MI_GREEN},
	{ "Allow BFG 1/0.",				ADMIN_HELP_1, -1,	-1,		8,		1,		NULL,				0},
	{ "break -",					ADMIN_HELP_1, -1,	-1,		9,		1,		NULL,				MI_GREEN},
	{ "Return to warmup.",			ADMIN_HELP_1, -1,	-1,		10,		1,		NULL,				0},
	{ "deladmin -",					ADMIN_HELP_1, -1,	-1,		11,		1,		NULL,				MI_GREEN},
	{ "Delete given addmin password.", ADMIN_HELP_1, -1,	-1,		12,		1,		NULL,				0},
	{ "details -",					ADMIN_HELP_1, -1,	-1,		13,		1,		NULL,				MI_GREEN},
	{ "Show players details.",		ADMIN_HELP_1, -1,	-1,		14,		1,		NULL,				0},
	{ "Next Page",					ADMIN_HELP_1, 0,	ADMIN_HELP_2,		17,		2,		submenu_enter,		0},
	{ "Return to game",				ADMIN_HELP_1, 1,	-1,		18,		2,		menu_off,			0},

	{ "dmf -",						ADMIN_HELP_2, -1,	-1,		1,		1,		NULL,				MI_GREEN},
	{ "Change dmflags.",			ADMIN_HELP_2, -1,	-1,		2,		1,		NULL,				0},
	{ "giveadmin -",				ADMIN_HELP_2, -1,	-1,		3,		1,		NULL,				MI_GREEN},
	{ "Give admin to given player.",ADMIN_HELP_2, -1,	-1,		4,		1,		NULL,				0},
	{ "hold -",						ADMIN_HELP_2, -1,	-1,		5,		1,		NULL,				MI_GREEN},
	{ "Pause/resume the game.",		ADMIN_HELP_2, -1,	-1,		6,		1,		NULL,				0},
	{ "isbanned -",					ADMIN_HELP_2, -1,	-1,		7,		1,		NULL,				MI_GREEN},
	{ "Checks if given ip is banned.", ADMIN_HELP_2, -1,	-1,		8,		1,		NULL,				0},
	{ "kickban -",					ADMIN_HELP_2, -1,	-1,		9,		1,		NULL,				MI_GREEN},
	{ "Kick and ban given player.",	ADMIN_HELP_2, -1,	-1,		10,		1,		NULL,				0},
	{ "kickplayer -",				ADMIN_HELP_2, -1,	-1,		11,		1,		NULL,				MI_GREEN},
	{ "Kick player from a team.",	ADMIN_HELP_2, -1,	-1,		12,		1,		NULL,				0},
	{ "kickuser -",					ADMIN_HELP_2, -1,	-1,		13,		1,		NULL,				MI_GREEN},
	{ "Kick player from the game.",	ADMIN_HELP_2, -1,	-1,		14,		1,		NULL,				0},
	{ "Next Page",					ADMIN_HELP_2, 0,	ADMIN_HELP_3,		17,		2,		submenu_enter,		0},
	{ "Prev Page",					ADMIN_HELP_2, 1,	-1,		18,		2,		submenu_leave,		0},

	{ "lockteam -",					ADMIN_HELP_3, -1,	-1,		1,		1,		NULL,				MI_GREEN},
	{ "Lock given team.",			ADMIN_HELP_3, -1,	-1,		2,		1,		NULL,				0},
	{ "obsmode -",					ADMIN_HELP_3, -1,	-1,		3,		1,		NULL,				MI_GREEN},
	{ "Spectators speak permission.",ADMIN_HELP_3, -1,	-1,		4,		1,		NULL,				0},
	{ "pass -",						ADMIN_HELP_3, -1,	-1,		5,		1,		NULL,				MI_GREEN},
	{ "Change server password.",	ADMIN_HELP_3, -1,	-1,		6,		1,		NULL,				0},
	{ "pickplayer -",				ADMIN_HELP_3, -1,	-1,		7,		1,		NULL,				MI_GREEN},
	{ "Pick player to given team.",	ADMIN_HELP_3, -1,	-1,		8,		1,		NULL,				0},
	{ "powerups -",					ADMIN_HELP_3, -1,	-1,		9,		1,		NULL,				MI_GREEN},
	{ "Allow powerups 1/0.",		ADMIN_HELP_3, -1,	-1,		10,		1,		NULL,				0},
	{ "refcode -",					ADMIN_HELP_3, -1,	-1,		11,		1,		NULL,				MI_GREEN},
	{ "Change referee password.",	ADMIN_HELP_3, -1,	-1,		12,		1,		NULL,				0},
	{ "refflags -",					ADMIN_HELP_3, -1,	-1,		13,		1,		NULL,				MI_GREEN},
	{ "Change referee entitlements.", ADMIN_HELP_3, -1,	-1,		14,		1,		NULL,				0},
	{ "Next Page",					ADMIN_HELP_3, 0,	ADMIN_HELP_4,		17,		2,		submenu_enter,		0},
	{ "Prev Page",					ADMIN_HELP_3, 1,	-1,		18,		2,		submenu_leave,		0},

	{ "reftag -",					ADMIN_HELP_4, -1,	-1,		1,		1,		NULL,				MI_GREEN},
	{ "Change referee tag.",		ADMIN_HELP_4, -1,	-1,		2,		1,		NULL,				0},
	{ "smap -",						ADMIN_HELP_4, -1,	-1,		3,		1,		NULL,				MI_GREEN},
	{ "Change current map.",		ADMIN_HELP_4, -1,	-1,		4,		1,		NULL,				0},
	{ "start -",					ADMIN_HELP_4, -1,	-1,		5,		1,		NULL,				MI_GREEN},
	{ "Force countdown.",			ADMIN_HELP_4, -1,	-1,		6,		1,		NULL,				0},
	{ "teamname -",					ADMIN_HELP_4, -1,	-1,		7,		1,		NULL,				MI_GREEN},
	{ "Change the team name.",		ADMIN_HELP_4, -1,	-1,		8,		1,		NULL,				0},
	{ "teamskin -",					ADMIN_HELP_4, -1,	-1,		9,		1,		NULL,				MI_GREEN},
	{ "Change the team skin.",		ADMIN_HELP_4, -1,	-1,		10,		1,		NULL,				0},
	{ "timelimit -",				ADMIN_HELP_4, -1,	-1,		11,		1,		NULL,				MI_GREEN},
	{ "Change current time limit.",	ADMIN_HELP_4, -1,	-1,		12,		1,		NULL,				0},
	{ "unban -",					ADMIN_HELP_4, -1,	-1,		13,		1,		NULL,				MI_GREEN},
	{ "Unban given IP.",			ADMIN_HELP_4, -1,	-1,		14,		1,		NULL,				0},
	{ "Next Page",					ADMIN_HELP_4, 0,	ADMIN_HELP_5,		17,		2,		submenu_enter,		0},
	{ "Prev Page",					ADMIN_HELP_4, 1,	-1,		18,		2,		submenu_leave,		0},

	{ "unlockteam -",				ADMIN_HELP_5, -1,	-1,		1,		1,		NULL,				MI_GREEN},
	{ "Unlock given team.",			ADMIN_HELP_5, -1,	-1,		2,		1,		NULL,				0},
	{ "vlock/vunlock -",			ADMIN_HELP_5, -1,	-1,		3,		1,		NULL,				MI_GREEN},
	{ "Lock/unlock given vote option.",	ADMIN_HELP_5, -1,	-1,		4,		1,		NULL,				0},
	{ "Prev Page",					ADMIN_HELP_5, 0,	-1,		17,		2,		submenu_leave,		0},
	{ "Return to game",				ADMIN_HELP_5, 1,	-1,		18,		2,		menu_off,			0},

	{ "accuracy -",					COMMANDS_1,	-1,		-1,		1,		1,		NULL,				MI_GREEN},
	{ "Shows your accuracy stats.",	COMMANDS_1, -1,		-1,		2,		1,		NULL,				0},
	{ "admin -",					COMMANDS_1, -1,		-1,		3,		1,		NULL,				MI_GREEN},
	{ "Lets you to become an admin.", COMMANDS_1, -1,	-1,		4,		1,		NULL,				0},
	{ "bfg -",						COMMANDS_1, -1,		-1,		5,		1,		NULL,				MI_GREEN},
	{ "Shows allow_bfg value.",		COMMANDS_1, -1,		-1,		6,		1,		NULL,				0},
	{ "captain -",					COMMANDS_1, -1,		-1,		7,		1,		NULL,				MI_GREEN},
	{ "Allows to chose new captain", COMMANDS_1, -1,	-1,		8,		1,		NULL,				0},
	{ "or shows teams captains.",	COMMANDS_1, -1,		-1,		9,		1,		NULL,				0},
	{ "chase -",					COMMANDS_1, -1,		-1,		10,		1,		NULL,				MI_GREEN},
	{ "Enter chasecam mode.",		COMMANDS_1, -1,		-1,		11,		1,		NULL,				0},
	{ "commands -",					COMMANDS_1, -1,		-1,		12,		1,		NULL,				MI_GREEN},
	{ "Displays this help.",		COMMANDS_1, -1,		-1,		13,		1,		NULL,				0},
	{ "configlist -",				COMMANDS_1, -1,		-1,		14,		1,		NULL,				MI_GREEN},
	{ "Displays available configs.",COMMANDS_1, -1,		-1,		15,		1,		NULL,				0},
	{ "Next Page",					COMMANDS_1, 0,		COMMANDS_2,		17,		2,		submenu_enter,				0},
	{ "Return to game",				COMMANDS_1, 1,		-1,		18,		2,		menu_off,			0},

	{ "dmf -",						COMMANDS_2,	-1,		-1,		1,		1,		NULL,				MI_GREEN},
	{ "Displays current dmflags.",	COMMANDS_2, -1,		-1,		2,		1,		NULL,				0},
	{ "fastweaps -",				COMMANDS_2, -1,		-1,		3,		1,		NULL,				MI_GREEN},
	{ "Shows fastweapons value.",	COMMANDS_2, -1,		-1,		4,		1,		NULL,				0},
	{ "gibs -",						COMMANDS_2, -1,		-1,		5,		1,		NULL,				MI_GREEN},
	{ "Shows allow_gibs value.",	COMMANDS_2, -1,		-1,		6,		1,		NULL,				0},
	{ "highlight -",				COMMANDS_2, -1,		-1,		7,		1,		NULL,				MI_GREEN},
	{ "Lets you choose which pos.",	COMMANDS_2, -1,		-1,		8,		1,		NULL,				0},
	{ "to highlight on scoreboard .",COMMANDS_2, -1,	-1,		9,		1,		NULL,				0},
	{ "hud -",						COMMANDS_2, -1,		-1,		10,		1,		NULL,				MI_GREEN},
	{ "Enable/disable weapons list.",COMMANDS_2, -1,	-1,		11,		1,		NULL,				0},
	{ "id -",						COMMANDS_2, -1,		-1,		12,		1,		NULL,				MI_GREEN},
	{ "Displays players id on hud",	COMMANDS_2, -1,		-1,		13,		1,		NULL,				0},
	{ "invnext, weapnext -",		COMMANDS_2, -1,		-1,		14,		1,		NULL,				MI_GREEN},
	{ "Chase next or next menu item.",COMMANDS_2, -1,	-1,		15,		1,		NULL,				0},
	{ "Next Page",					COMMANDS_2, 0,		COMMANDS_3,		17,		2,		submenu_enter,				0},
	{ "Prev Page",					COMMANDS_2, 1,		-1,		18,		2,		submenu_leave,		0},

	{ "invprev, weapprev -",		COMMANDS_3,	-1,		-1,		1,		1,		NULL,				MI_GREEN},
	{ "Chase prev or prev menu item.",COMMANDS_3, -1,	-1,		2,		1,		NULL,				0},
	{ "items -",					COMMANDS_3, -1,		-1,		3,		1,		NULL,				MI_GREEN},
	{ "Shows items pickup stats.",	COMMANDS_3, -1,		-1,		4,		1,		NULL,				0},
	{ "join, team -",				COMMANDS_3, -1,		-1,		5,		1,		NULL,				MI_GREEN},
	{ "Enter chosen team.",			COMMANDS_3, -1,		-1,		6,		1,		NULL,				0},
	{ "kickplayer, kickp -",		COMMANDS_3, -1,		-1,		7,		1,		NULL,				MI_GREEN},
	{ "Kick player from your team.",COMMANDS_3, -1,		-1,		8,		1,		NULL,				0},
	{ "leave, observer, spectate -",COMMANDS_3, -1,		-1,		9,		1,		NULL,				MI_GREEN},
	{ "Enter spectator mode.",		COMMANDS_3, -1,		-1,		10,		1,		NULL,				0},
	{ "lockteam, lock -",			COMMANDS_3, -1,		-1,		11,		1,		NULL,				MI_GREEN},
	{ "Lock your team.",			COMMANDS_3, -1,		-1,		12,		1,		NULL,				0},
	{ "maplist -",					COMMANDS_3, -1,		-1,		13,		1,		NULL,				MI_GREEN},
	{ "Displays map list.",			COMMANDS_3, -1,		-1,		14,		1,		NULL,				0},
	{ "Next Page",					COMMANDS_3, 0,		COMMANDS_4,		17,		2,		submenu_enter,		0},
	{ "Prev Page",					COMMANDS_3, 1,		-1,		18,		2,		submenu_leave,		0},

	{ "matchinfo, sets -",			COMMANDS_4,	-1,		-1,		1,		1,		NULL,				MI_GREEN},
	{ "Displays match info.",		COMMANDS_4, -1,		-1,		2,		1,		NULL,				0},
	{ "menu, inven -",				COMMANDS_4, -1,		-1,		3,		1,		NULL,				MI_GREEN},
	{ "Turn in-game menu on/off.",	COMMANDS_4, -1,		-1,		4,		1,		NULL,				0},
	{ "menuenter, invuse -",		COMMANDS_4, -1,		-1,		5,		1,		NULL,				MI_GREEN},
	{ "Enter selected menu item.",	COMMANDS_4, -1,		-1,		6,		1,		NULL,				0},
	{ "menuleave, invdrop -",		COMMANDS_4, -1,		-1,		7,		1,		NULL,				MI_GREEN},
	{ "Enter previous menu.",		COMMANDS_4, -1,		-1,		8,		1,		NULL,				0},
	{ "new_drops -",				COMMANDS_4, -1,		-1,		9,		1,		NULL,				MI_GREEN},
	{ "Toggle new drop mode.",		COMMANDS_4, -1,	-1,		10,		1,		NULL,				0},
	{ "no, yes -",					COMMANDS_4, -1,		-1,		11,		1,		NULL,				MI_GREEN},
	{ "Allows you to vote no/yes.",	COMMANDS_4, -1,		-1,		12,		1,		NULL,				0},
	{ "noready, notready, unready-",COMMANDS_4, -1,		-1,		13,		1,		NULL,				MI_GREEN},
	{ "Changes your ready status.",	COMMANDS_4, -1,		-1,		14,		1,		NULL,				0},
	{ "Next Page",					COMMANDS_4, 0,		COMMANDS_5,		17,		2,		submenu_enter,				0},
	{ "Prev Page",					COMMANDS_4, 1,		-1,		18,		2,		submenu_leave,		0},

	{ "obsmode -",					COMMANDS_5,	-1,		-1,		1,		1,		NULL,				MI_GREEN},
	{ "Shows current obsmode value.",COMMANDS_5, -1,	-1,		2,		1,		NULL,				0},
	{ "oldscore -",					COMMANDS_5, -1,		-1,		3,		1,		NULL,				MI_GREEN},
	{ "Displays old score.",		COMMANDS_5, -1,		-1,		4,		1,		NULL,				0},
	{ "pickplayer, pick -",			COMMANDS_5, -1,		-1,		5,		1,		NULL,				MI_GREEN},
	{ "Pick player to your team.",	COMMANDS_5, -1,		-1,		6,		1,		NULL,				0},
	{ "playerlist, players -",		COMMANDS_5, -1,		-1,		7,		1,		NULL,				MI_GREEN},
	{ "Displays player list.",		COMMANDS_5, -1,		-1,		8,		1,		NULL,				0},
	{ "powerups -",					COMMANDS_5, -1,		-1,		9,		1,		NULL,				MI_GREEN},
	{ "Shows allow_powerups value.",COMMANDS_5, -1,		-1,		10,		1,		NULL,				0},
	{ "ready -",					COMMANDS_5, -1,		-1,		11,		1,		NULL,				MI_GREEN},
	{ "Changes your ready status.",	COMMANDS_5, -1,		-1,		12,		1,		NULL,				0},
	{ "readyteam -",				COMMANDS_5, -1,		-1,		13,		1,		NULL,				MI_GREEN},
	{ "Forces your team to ready.",	COMMANDS_5, -1,		-1,		14,		1,		NULL,				0},
	{ "Next Page",					COMMANDS_5, 0,		COMMANDS_6,		17,		2,		submenu_enter,				0},
	{ "Prev Page",					COMMANDS_5, 1,		-1,		18,		2,		submenu_leave,		0},

	{ "score -",					COMMANDS_6,	-1,		-1,		1,		1,		NULL,				MI_GREEN},
	{ "Displays the scoreboard.",	COMMANDS_6, -1,		-1,		2,		1,		NULL,				0},
	{ "steam, say_team -",			COMMANDS_6, -1,		-1,		3,		1,		NULL,				MI_GREEN},
	{ "Talk with your teammates.",	COMMANDS_6, -1,		-1,		4,		1,		NULL,				0},
	{ "talk, Tell -",				COMMANDS_6, -1,		-1,		5,		1,		NULL,				MI_GREEN},
	{ "Talk to given player.",		COMMANDS_6, -1,		-1,		6,		1,		NULL,				0},
	{ "teamname -",					COMMANDS_6, -1,		-1,		7,		1,		NULL,				MI_GREEN},
	{ "Changes your team name.",	COMMANDS_6, -1,		-1,		8,		1,		NULL,				0},
	{ "teamskin -",					COMMANDS_6, -1,		-1,		9,		1,		NULL,				MI_GREEN},
	{ "Changes your team skin.",	COMMANDS_6, -1,		-1,		10,		1,		NULL,				0},
	{ "time -",						COMMANDS_6, -1,		-1,		11,		1,		NULL,				MI_GREEN},
	{ "Calls timeout for your team.",COMMANDS_6, -1,	-1,		12,		1,		NULL,				0},
	{ "tl -",						COMMANDS_6, -1,		-1,		13,		1,		NULL,				MI_GREEN},
	{ "Shows timelimit value.",		COMMANDS_6, -1,		-1,		14,		1,		NULL,				0},
	{ "Next Page",					COMMANDS_6, 0,		COMMANDS_7,		17,		2,		submenu_enter,				0},
	{ "Prev Page",					COMMANDS_6, 1,		-1,		18,		2,		submenu_leave,		0},

	{ "tp -",						COMMANDS_7,	-1,		-1,		1,		1,		NULL,				MI_GREEN},
	{ "Shows current TP value .",	COMMANDS_7, -1,		-1,		2,		1,		NULL,				0},
	{ "unlockteam, unlock -",		COMMANDS_7, -1,		-1,		3,		1,		NULL,				MI_GREEN},
	{ "Unlock your team.",			COMMANDS_7, -1,		-1,		4,		1,		NULL,				0},
	{ "vote -",						COMMANDS_7, -1,		-1,		5,		1,		NULL,				MI_GREEN},
	{ "Allows you to call a vote.",	COMMANDS_7, -1,		-1,		6,		1,		NULL,				0},
	{ "Prev Page",					COMMANDS_7, 0,		-1,		17,		2,		submenu_leave,		0},
	{ "Return to game",				COMMANDS_7, 1,		-1,		18,		2,		menu_off,		0},
};

void menu_draw (edict_t *self);

void SpectateOrChase (edict_t *self, qboolean forward)
{
	if (self->client->pers.save_data.team != TEAM_NONE)
	{
		self->client->pers.save_data.team = TEAM_NONE;
//		CheckCaptain(self);
		Write_ConfigString2(self);

		if (forward == false)
			spectator_respawn (self, true);
		else
			spectator_respawn (self, false);

		menu_off(self, true);

		if (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH)
			GetChaseTarget(self);
	}
	else if (!self->client->chase_target && self->client->pers.save_data.team == TEAM_NONE)
	{
		GetChaseTarget(self);
		if (!self->client->chase_target)
			gi.centerprintf(self, "No other players to chase.");
		menu_off(self, true);
	}
	else if (self->client->chase_target && self->client->pers.save_data.team == TEAM_NONE && (level.match_state == WARMUP || level.match_state == PREGAME || level.match_state == COUNTDOWN))
	{
		StopChasing(self);
		menu_off(self, true);
	}
}

void join_team_a (edict_t *self, qboolean forward)
{
	if (self->client->pers.save_data.team == TEAM_A)
	{
		SpectateOrChase (self, true);
		return;
	}
	Cmd_Join_f(self, TEAM_A);
	menu_off(self, true);
}

void join_team_b (edict_t *self, qboolean forward)
{
	if (self->client->pers.save_data.team == TEAM_B)
	{
		SpectateOrChase (self, true);
		return;
	}
	Cmd_Join_f(self, TEAM_B);
	menu_off(self, true);
}

void menu_off (edict_t *self, qboolean forward)
{
	if (!self->client)
		return;

	self->client->showmenu = false;
	self->client->menu.current_menu_item = 0;
	self->client->menu.current_menu = 0;
}

void proposal_changes (edict_t *self, qboolean forward)
{
	char	voteoptions[128];
	qboolean	was_before = false;

	self->client->menu.multivote = false;

	if (!self->client->menu.changed_item)
	{
		gi.cprintf(self, PRINT_HIGH, "No changes. No proposal initiated.\n");
		menu_off(self, true);
		return;
	}

	if (level.vote.vote_active)
	{
		gi.cprintf(self, PRINT_HIGH, "Vote already in progress.\n");
		return;
	}

	if ((level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH) && self->client->pers.save_data.team == TEAM_NONE)
	{
		gi.cprintf(self, PRINT_HIGH, "You can't vote during the match unless you are playing.\n");
		return;
	}

	level.vote.vote_what = self->client->menu.changed_item;

	MyCPrintfInit(self);
	MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has initiated a vote!\nProposal:\n",self->client->pers.netname);

	if (level.vote.vote_what & VOTE_MAP)
	{
		strcpy(level.vote.map, self->client->menu.mapname);
		MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Change the map to %s.\n", level.vote.map);
		was_before = true;
	}
	if (level.vote.vote_what & VOTE_TIMELIMIT)
	{
		level.vote.timelimit = self->client->menu.v_timelimit;
		MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Set the timelimit to %d.\n", level.vote.timelimit);
		if (was_before)
			self->client->menu.multivote = true;
		else
			was_before = true;
	}
	if (level.vote.vote_what & VOTE_TP)
	{
		level.vote.tp = self->client->menu.v_tp;
		MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Set tp to %d.\n", level.vote.tp);
		if (was_before)
			self->client->menu.multivote = true;
		else
			was_before = true;
	}
	if ( (level.vote.vote_what & VOTE_KICK ) && g_edicts[1+level.vote.kick].client && g_edicts[1+level.vote.kick].inuse )
	{
		if ( self->client->menu.v_kick < 0 )
			level.vote.vote_what &= ~VOTE_KICK;
		else
		{
			level.vote.kick = self->client->menu.v_kick;
			MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Kick client %d (%s).\n", level.vote.kick, g_edicts[1+level.vote.kick].client->pers.netname);
			if (was_before)
				self->client->menu.multivote = true;
			else
				was_before = true;
		}
	}
	if (level.vote.vote_what & VOTE_DMFLAGS)
	{
		int	i;
		qboolean isvalidflag = false;

		level.vote.dmflags = dmflags_table[self->client->menu.v_dmflags].flags;

		for (i=1; i<NUM_DMFLAGS; i++)
		{
			if (level.vote.dmflags == dmflags_table[i].flags && level.vote.dmflags != 128)
			{
				isvalidflag = true;
				break;
			}
		}
		if ((int)dmflags->value & level.vote.dmflags)
			MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "DISABLE dmflag \"%s\".\n", dmflags_table[i].name);
		else
			MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "ENABLE dmflag \"%s\".\n", dmflags_table[i].name);
		if (was_before)
			self->client->menu.multivote = true;
		else
			was_before = true;
	}
	if (level.vote.vote_what & VOTE_CONFIG)
	{
		self->client->menu.config = GetConfigFromList(self->client->menu.v_config);
		sprintf(level.vote.config, "%s", self->client->menu.config);
		if (self->client->menu.config)
			free(self->client->menu.config);
		MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Execute config: %s.\n", level.vote.config);
		if (was_before)
			self->client->menu.multivote = true;
		else
			was_before = true;
	}
	if (level.vote.vote_what & VOTE_BFG)
	{
		level.vote.bfg = self->client->menu.v_bfg;
		MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Change the bfg to %d.\n", level.vote.bfg);
		if (was_before)
			self->client->menu.multivote = true;
		else
			was_before = true;
	}
	if (level.vote.vote_what & VOTE_POWERUPS)
	{
		level.vote.powerups = self->client->menu.v_powerups;
		MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Change the powerups to %d.\n", level.vote.powerups);
		if (was_before)
			self->client->menu.multivote = true;
		else
			was_before = true;
	}
	if (level.vote.vote_what & VOTE_FASTWEAPONS)
	{
		level.vote.fastweapons = self->client->menu.v_fastweapons;
		MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Change the fastweapons to %d.\n", level.vote.fastweapons);
		if (was_before)
			self->client->menu.multivote = true;
		else
			was_before = true;
	}
	if (level.vote.vote_what & VOTE_HUD)
	{
		level.vote.hud = self->client->menu.v_hud;
		if (level.vote.hud == true)
			MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Allow players to enable weapon list in the hud.\n");
		else
			MyCPrintf(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Disable weapon list in the hud.\n");
		if (was_before)
			self->client->menu.multivote = true;
		else
			was_before = true;
	}

	MyCPrintfEnd(self, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE);

	level.vote.vote_time = level.time + GAMESECONDS(60);
	level.vote.vote_active = true;

	if (self->client->menu.multivote)
		level.vote.multivote = true;
	else
		level.vote.multivote = false;

	sprintf(voteoptions, "MULTIVOTE");
	Log(self, LOG_VOTE, voteoptions);
	level.vote.voter = self;
	level.vote.update_vote = true;	
	self->client->pers.save_data.vote_yon = VOTE_YES;
	level.vote.vote_no = 0;
	level.vote.vote_yes = 1;
	menu_off(self, true);
}

void change_config (edict_t *self, qboolean forward)
{
	if (!CanVote (self, "config"))
		return;

	if (self->client->menu.changed_item & CHANGED_MAP)
	{
		gi.cprintf(self, PRINT_HIGH, "You can't vote \"map\" and \"config\" at one time.\n");
		return;
	}

	self->client->menu.v_config++;
	self->client->menu.config = GetConfigFromList(self->client->menu.v_config);
	if (!self->client->menu.config)
		self->client->menu.v_config = -1;
	else
		free(self->client->menu.config);

	if (self->client->menu.v_config != -1)
		self->client->menu.changed_item |= CHANGED_CONFIG;
	else if (self->client->menu.changed_item & CHANGED_CONFIG)
			self->client->menu.changed_item &= ~CHANGED_CONFIG;

	menu_draw(self);
}
void change_kick (edict_t *self, qboolean forward)
{
	edict_t	*ent;
	int		i;

	if (!CanVote (self, "kickuser"))
		return;

	if (forward)
	{
		for (i=0 ; i<game.maxclients ; i++)
		{
			ent = &g_edicts[1+i];

			if (ent->client && ent->inuse && !ent->client->pers.mvdspec)
			{
				if (self->client->menu.v_kick < i)
					self->client->menu.v_kick = i;
				else
					continue;

				sprintf(self->client->menu.kick, "%s", ent->client->pers.netname);
				break;
			}
			else if (i+1 == game.maxclients)
			{
				self->client->menu.v_kick = -1;
				sprintf(self->client->menu.kick, "[SELECT]");
				break;
			}
		}
	}
	else
	{
		for (i=game.maxclients-1 ; i>-1 ; i--)
		{
			ent = &g_edicts[1+i];

			if (ent->client && ent->inuse && !ent->client->pers.mvdspec)
			{
				if (self->client->menu.v_kick != i)
					self->client->menu.v_kick = i;
				else
					continue;

				sprintf(self->client->menu.kick, "%s", ent->client->pers.netname);
				break;
			}
			else if (self->client->menu.v_kick == 0)
			{
				self->client->menu.v_kick = -1;
				sprintf(self->client->menu.kick, "[SELECT]");
				break;
			}
		}
	}
	if (self->client->menu.v_kick != -1)
		self->client->menu.changed_item |= CHANGED_KICK;
	else if (self->client->menu.changed_item & CHANGED_KICK)
		self->client->menu.changed_item &= ~CHANGED_KICK;

	menu_draw(self);
}

void change_bfg (edict_t *self, qboolean forward)
{
	if (!CanVote (self, "bfg"))
		return;

	if (!self->client->menu.v_bfg)
		self->client->menu.v_bfg = true;
	else
		self->client->menu.v_bfg = false;

	if (self->client->menu.v_bfg != allow_bfg->value)
		self->client->menu.changed_item |= CHANGED_BFG;
	else if (self->client->menu.changed_item & CHANGED_BFG)
			self->client->menu.changed_item &= ~CHANGED_BFG;

	menu_draw(self);
}

void change_powerups (edict_t *self, qboolean forward)
{
	if (!CanVote (self, "powerups"))
		return;

	if (!self->client->menu.v_powerups)
		self->client->menu.v_powerups = true;
	else
		self->client->menu.v_powerups = false;

	if (self->client->menu.v_powerups != allow_powerups->value)
		self->client->menu.changed_item |= CHANGED_POWERUPS;
	else if (self->client->menu.changed_item & CHANGED_POWERUPS)
			self->client->menu.changed_item &= ~CHANGED_POWERUPS;

	menu_draw(self);
}

void change_fastweapons (edict_t *self, qboolean forward)
{
	if (!CanVote (self, "fastweapons"))
		return;

	if (!self->client->menu.v_fastweapons)
		self->client->menu.v_fastweapons = true;
	else
		self->client->menu.v_fastweapons = false;

	if (self->client->menu.v_fastweapons != fastweapons->value)
		self->client->menu.changed_item |= CHANGED_FASTWEAPONS;
	else if (self->client->menu.changed_item & CHANGED_FASTWEAPONS)
			self->client->menu.changed_item &= ~CHANGED_FASTWEAPONS;

	menu_draw(self);
}

void change_map (edict_t *self, qboolean forward)
{
	cvar_t	*basedir, *gamedir;
	char	filename[256];
	FILE	*f=NULL;

	if (!CanVote (self, "map"))
		return;

	if (self->client->menu.changed_item & CHANGED_CONFIG)
	{
		gi.cprintf(self, PRINT_HIGH, "You can't vote \"map\" and \"config\" at one time.\n");
		return;
	}

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);

	strcpy(filename,basedir->string);
	if(strlen(gamedir->string))
	{
		strcat(filename,"/");
		strcat(filename,gamedir->string);
		strcat(filename,"/");
		if(strlen(sv_maplist->string))
			strcat(filename, sv_maplist->string);
		else
			strcat(filename,"maps.lst");
		f = fopen(filename,"r");
	}

	if (!f || !strlen(gamedir->string))
	{
		strcpy(filename,basedir->string);
		strcat(filename,"/baseq2/");
		if(strlen(sv_maplist->string))
			strcat(filename, sv_maplist->string);
		else
			strcat(filename,"maps.lst");
	}

	f = fopen(filename,"r");
	if(!f)
	{
		sprintf(self->client->menu.mapname, "[maps.lst NOT FOUND!]");
		menu_draw(self);
		return;
	}
	else
	{
		int			count=0;
		char		*line;

		if (forward)
			self->client->menu.mapnum++;
		else
			self->client->menu.mapnum--;

		if (self->client->menu.mapnum < 0)
		{
			self->client->menu.mapnum = -1;
			sprintf(self->client->menu.mapname, "[SELECT]");
			if (self->client->menu.changed_item & CHANGED_MAP)
				self->client->menu.changed_item &= ~CHANGED_MAP;
		}
		else
		{
			do
			{
				line = read_line(f);
				if (feof(f) && count != self->client->menu.mapnum)
				{
					count = self->client->menu.mapnum = 0;
					rewind(f);
					continue;
				}
				if (count == self->client->menu.mapnum)
				{
					char *word = read_word(line, 0);
					if (!strcmp(level.mapname, word))
					{
						if (self->client->menu.changed_item & CHANGED_MAP)
							self->client->menu.changed_item &= ~CHANGED_MAP;
					}
					else
						self->client->menu.changed_item |= CHANGED_MAP;
					strcpy(self->client->menu.mapname, word);
					break;
				}
				count++;
			} while (!feof(f));
		}
		fclose( f );
	}

	menu_draw(self);
}


void change_timelimit (edict_t *self, qboolean forward)
{
	if (!CanVote (self, "timelimit"))
		return;

	if (forward)
	{
		if (self->client->menu.v_timelimit == 120)
			self->client->menu.v_timelimit = 0;
		else
			self->client->menu.v_timelimit += 5;
	}
	else
	{
		if (self->client->menu.v_timelimit == 0)
			self->client->menu.v_timelimit = 120;
		else
			self->client->menu.v_timelimit -= 5;
	}

	if (self->client->menu.v_timelimit != timelimit->value)
		self->client->menu.changed_item |= CHANGED_TIMELIMIT;
	else if (self->client->menu.changed_item & CHANGED_TIMELIMIT)
		self->client->menu.changed_item &= ~CHANGED_TIMELIMIT;

	menu_draw(self);
}

void change_dmflags (edict_t *self, qboolean forward)
{
	if (!CanVote (self, "dmflags"))
		return;

	if (forward)
	{
		if (self->client->menu.v_dmflags == NUM_DMFLAGS-1)
			self->client->menu.v_dmflags = 0;
		else
			self->client->menu.v_dmflags += 1;
	}
	else
	{
		if (self->client->menu.v_dmflags == -1)
			self->client->menu.v_dmflags = NUM_DMFLAGS-1;
		else
			self->client->menu.v_dmflags -= 1;
	}
	if (self->client->menu.v_dmflags != 0 && self->client->menu.v_dmflags != 8)
		self->client->menu.changed_item |= CHANGED_DMFLAGS;
	else if (self->client->menu.changed_item & CHANGED_DMFLAGS)
		self->client->menu.changed_item &= ~CHANGED_DMFLAGS;

	menu_draw(self);
}

void change_tp (edict_t *self, qboolean forward)
{
	if (!CanVote (self, "tp"))
		return;

	if (forward)
	{
		self->client->menu.v_tp++;

		if (self->client->menu.v_tp > 4)
			self->client->menu.v_tp = 1;
	}
	else
	{
		self->client->menu.v_tp--;
		if (self->client->menu.v_tp < 1)
			self->client->menu.v_tp = 4;
	}

	if (self->client->menu.v_tp != game.tp)
		self->client->menu.changed_item |= CHANGED_TP;
	else if (self->client->menu.changed_item & CHANGED_TP)
		self->client->menu.changed_item &= ~CHANGED_TP;

	menu_draw(self);
}

void change_hud (edict_t *self, qboolean forward)
{
	if (!CanVote (self, "hud"))
		return;

	if (!self->client->menu.v_hud)
		self->client->menu.v_hud = true;
	else
		self->client->menu.v_hud = false;

	if (self->client->menu.v_hud != allow_hud->value)
		self->client->menu.changed_item |= CHANGED_HUD;
	else if (self->client->menu.changed_item & CHANGED_HUD)
			self->client->menu.changed_item &= ~CHANGED_HUD;

	menu_draw(self);
}

void menu_draw (edict_t *self)
{
	char	string[1400];
	char	*text;
	char	entry[1024];
	int		stringlength;
	int		i,j,k,n,newpos;
	char	selected=13;
	qboolean	print_spects=true;
	edict_t *other;

	Com_sprintf (string, sizeof(string), "xv 32 yv 8 picn inventory ");
	stringlength = strlen(string);

	for (i=0; i < NUM_MI; i++)
	{
		if (menu_items_table[i].menu_index != self->client->menu.current_menu)
			continue;

		text = (char *)malloc(257);
		strcpy(text, menu_items_table[i].name);

		if (menu_items_table[i].execute == join_team_a && self->client->pers.save_data.team != TEAM_A)
			sprintf(text, menu_items_table[i].name, "Team", game.teamA_name);
		else if (menu_items_table[i].execute == join_team_a)
			sprintf(text, menu_items_table[i].name, "Leave", game.teamA_name);
		if (menu_items_table[i].execute == join_team_b && self->client->pers.save_data.team != TEAM_B)
			sprintf(text, menu_items_table[i].name, "Team", game.teamB_name);
		else if (menu_items_table[i].execute == join_team_b)
			sprintf(text, menu_items_table[i].name, "Leave", game.teamB_name);
		else if (menu_items_table[i].execute == SpectateOrChase)
		{
			if ((!self->client->chase_target && self->client->pers.save_data.team == TEAM_NONE) || (self->client->pers.save_data.team != TEAM_NONE && (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH)))
				sprintf(text, menu_items_table[i].name, "Chase");
			else if (self->client->pers.save_data.team != TEAM_NONE || (self->client->pers.save_data.team == TEAM_NONE && (level.match_state == WARMUP || level.match_state == PREGAME || level.match_state == COUNTDOWN)))
				sprintf(text, menu_items_table[i].name, "Spectate");
			else
			{
				print_spects = false;
				free(text);
				continue;
			}
		}

		if (menu_items_table[i].flags & MI_OPTIONS)
		{
			if (menu_items_table[i].execute == change_timelimit)
				sprintf(text, menu_items_table[i].name, self->client->menu.v_timelimit);
			else if (menu_items_table[i].execute == change_map)
				sprintf(text, menu_items_table[i].name, self->client->menu.mapname);
			else if (menu_items_table[i].execute == change_bfg)
			{
				if (self->client->menu.v_bfg)
					sprintf(text, menu_items_table[i].name, "YES");
				else
					sprintf(text, menu_items_table[i].name, "NO");
			}
			else if (menu_items_table[i].execute == change_powerups)
			{
				if (self->client->menu.v_powerups)
					sprintf(text, menu_items_table[i].name, "YES");
				else
					sprintf(text, menu_items_table[i].name, "NO");
			}
			else if (menu_items_table[i].execute == change_fastweapons)
			{
				if (self->client->menu.v_fastweapons)
					sprintf(text, menu_items_table[i].name, "YES");
				else
					sprintf(text, menu_items_table[i].name, "NO");
			}
			else if (menu_items_table[i].execute == change_kick)
				sprintf(text, menu_items_table[i].name, self->client->menu.kick);
			else if (menu_items_table[i].execute == change_dmflags)
			{
				if (!self->client->menu.v_dmflags)
					sprintf(text, menu_items_table[i].name, dmflags_table[self->client->menu.v_dmflags].name,"YES/NO");
				else
				{
					if ( dmflags_table[self->client->menu.v_dmflags].flags == DF_SPAWN_RANDOM )
						sprintf(text, menu_items_table[i].name, dmflags_table[self->client->menu.v_dmflags].name, "NO CHANGE");
					else if ((int)dmflags->value & dmflags_table[self->client->menu.v_dmflags].flags)
						sprintf(text, menu_items_table[i].name, dmflags_table[self->client->menu.v_dmflags].name,"YES->NO");
					else
						sprintf(text, menu_items_table[i].name, dmflags_table[self->client->menu.v_dmflags].name,"NO->YES");
				}
			}
			else if (menu_items_table[i].execute == change_config)
			{
				self->client->menu.config = GetConfigFromList(self->client->menu.v_config);
				if (!self->client->menu.config)
					sprintf(text, menu_items_table[i].name, "[SELECT]");
				else
					sprintf(text, menu_items_table[i].name, self->client->menu.config);
				free(self->client->menu.config);
			}
			else if (menu_items_table[i].execute == change_tp)
				sprintf(text, menu_items_table[i].name, self->client->menu.v_tp);
			else if (menu_items_table[i].execute == change_hud)
			{
				if (self->client->menu.v_hud)
					sprintf(text, menu_items_table[i].name, "YES");
				else
					sprintf(text, menu_items_table[i].name, "NO");
			}
		}

		if (menu_items_table[i].flags & MI_MAPNAME)
			sprintf(text, menu_items_table[i].name, level.level_name);

		if (print_spects == false && menu_items_table[i].flags & MI_SPLAYERS)
		{
			free(text);
			continue;
		}

		if (menu_items_table[i].flags & MI_APLAYERS || menu_items_table[i].flags & MI_BPLAYERS || menu_items_table[i].flags & MI_SPLAYERS)
		{
			int		a=0;
			int		b=0;
			int		s=0;

			for (j = 1; j <= game.maxclients; j++)
			{
				other = &g_edicts[j];
				if (!other->inuse)
					continue;
				if (!other->client)
					continue;
				if ( other->client->pers.mvdspec )
					continue;

				if (other->client->pers.save_data.team == TEAM_A)
					a++;
				if (other->client->pers.save_data.team == TEAM_B)
					b++;
				if (other->client->pers.save_data.team == TEAM_NONE)
					s++;
			}

			if (menu_items_table[i].flags & MI_APLAYERS)
				sprintf(text, menu_items_table[i].name, a);
			else if (menu_items_table[i].flags & MI_BPLAYERS)
				sprintf(text, menu_items_table[i].name, b);
			else if (menu_items_table[i].flags & MI_SPLAYERS)
				sprintf(text, menu_items_table[i].name, s);
		}

		newpos = menu_items_table[i].dist_left;

		if (menu_items_table[i].flags & MI_CENTER)
		{
			int	leng;

			leng = strlen(text);

			newpos = (28 - leng)/2;
		}

		if (menu_items_table[i].item_index == self->client->menu.current_menu_item)
		{
			k = strlen(text);
			for (n=0; n<k; n++)
				text[n] |= 128;

			Com_sprintf (entry, sizeof(entry), "xv %i yv %i string2 \"%c\" ", 48, 24+8*menu_items_table[i].dist_top, selected);

			j = strlen(entry);
			if (stringlength + j > 1024)
			{
				free(text);
				break;
			}
			strcpy (string + stringlength, entry);
			stringlength += j;
		}

		else if (menu_items_table[i].flags & MI_GREEN)
		{
			k = strlen(text);
			for (n=0; n<k; n++)
				text[n] |= 128;
		}

		Com_sprintf (entry, sizeof(entry), "xv %i yv %i string \"%s\" ", 48+8*newpos, 24+8*menu_items_table[i].dist_top, text);

		j = strlen(entry);
		if (stringlength + j > 1024)
		{
				free(text);
				break;
		}
		strcpy (string + stringlength, entry);
		stringlength += j;
		free(text);
	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
	gi.unicast (self, true);
}

void menu_init_vote( edict_t *self )
{
	sprintf(self->client->menu.mapname, "[SELECT]");
	self->client->menu.mapnum=-1;
	self->client->menu.changed_item = 0;
	self->client->menu.v_timelimit = (int)timelimit->value;
	if (allow_bfg->value == 1)
		self->client->menu.v_bfg = true;
	else
		self->client->menu.v_bfg = false;
	if (allow_powerups->value == 1)
		self->client->menu.v_powerups = true;
	else
		self->client->menu.v_powerups = false;
	if (fastweapons->value == 1)
		self->client->menu.v_fastweapons = true;
	else
		self->client->menu.v_fastweapons = false;
	if (allow_hud->value == 1)
		self->client->menu.v_hud = true;
	else
		self->client->menu.v_hud = false;
	self->client->menu.v_kick = -1;
	self->client->menu.v_config = -1;
	self->client->menu.v_tp = game.tp;
	sprintf(self->client->menu.kick, "[SELECT]");
	self->client->menu.multivote = false;
	self->client->menu.v_dmflags = 0;
	self->client->menu.v_dmflags_active=false;
}

void submenu_enter (edict_t *self, qboolean forward)
{
	int	i;
	int	match=0;

	if (!self->client)
		return;

	for(i=0; i<NUM_MI; i++)
	{
		if (menu_items_table[i].menu_index != self->client->menu.current_menu)
			continue;

		if (self->client->menu.current_menu_item == menu_items_table[i].item_index)
		{
			if (!menu_items_table[i].sub_menu_index)
				return;
			else
				match = i;
		}
	}

	if (match)
	{
		self->client->menu.current_menu = menu_items_table[match].sub_menu_index;
		self->client->menu.current_menu_item = 0;

		menu_init_vote( self );
		menu_draw(self);
	}
}

void submenu_leave (edict_t *self, qboolean forward)
{
	if (!self->client)
		return;

	if (menus[self->client->menu.current_menu].prev_menu >= 0)
	{
		self->client->menu.current_menu_item = 0;
		self->client->menu.current_menu = menus[self->client->menu.current_menu].prev_menu;
		menu_draw(self);
	}
}

void menu_next (edict_t *self)
{
	if (!self->client)
		return;

	self->client->menu.current_menu_item++;
	if (self->client->menu.current_menu_item >= menus[self->client->menu.current_menu].num_items)
		self->client->menu.current_menu_item = 0;
	menu_draw(self);
}

void menu_prev (edict_t *self)
{
	if (!self->client)
		return;

	self->client->menu.current_menu_item--;
	if (self->client->menu.current_menu_item < 0)
		self->client->menu.current_menu_item = menus[self->client->menu.current_menu].num_items-1;
	menu_draw(self);
}

void menu_display (edict_t *self, int menu)
{
	int a=0;
	int	b=0;
	int	i;
	edict_t	*other;

	if (!self->client)
		return;

	self->client->menu.current_menu_item = 0;
	self->client->menu.current_menu = menu;

	menu_init_vote( self );

	if (menu == 0)
	{
		for (i = 1; i <= game.maxclients; i++)
		{
			other = &g_edicts[i];
			if (!other->inuse || other->client->pers.mvdspec == true )
				continue;

			if (other->client->pers.save_data.team == TEAM_A)
				a++;
			if (other->client->pers.save_data.team == TEAM_B)
				b++;
		}
		if (self->client->pers.save_data.team == TEAM_B || (a > b && self->client->pers.save_data.team != TEAM_A))
			self->client->menu.current_menu_item = 1;
		else
			self->client->menu.current_menu_item = 0;
	}
	else
		self->client->menu.current_menu_item = 0;

	menu_draw (self);
}

void menu_enter (edict_t *self, qboolean forward)
{
	int	i;
	if (!self->client)
		return;

	for(i=0; i<NUM_MI; i++)
	{
		if (menu_items_table[i].menu_index != self->client->menu.current_menu)
			continue;

		if (self->client->menu.current_menu_item == menu_items_table[i].item_index)
		{
			if (menu_items_table[i].execute)
			{
				menu_items_table[i].execute(self, forward);
				return;
			}
		}
	}
}
