/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "g_local.h"
#include "tdm_plugins_internal.h"
#include "tdm.h"
#include "version.h"

game_locals_t	game;
level_locals_t	level;
game_import_t	gi;
game_export_t	globals;
spawn_temp_t	st;
oldscores_t		old_scores;

int	sm_meat_index;
int	snd_fry;
int meansOfDeath;

edict_t		*g_edicts;

cvar_t	*deathmatch;
cvar_t	*dmflags;
cvar_t	*skill;
cvar_t	*fraglimit;
cvar_t	*timelimit;
cvar_t	*password;
cvar_t	*spectator_password;
cvar_t	*needpass;
cvar_t	*maxclients;
cvar_t	*maxspectators;
cvar_t	*maxentities;
cvar_t	*g_select_empty;
cvar_t	*dedicated;

cvar_t	*filterban;

cvar_t	*sv_maxvelocity;
cvar_t	*sv_gravity;

cvar_t	*sv_rollspeed;
cvar_t	*sv_rollangle;
cvar_t	*gun_x;
cvar_t	*gun_y;
cvar_t	*gun_z;

cvar_t	*run_pitch;
cvar_t	*run_roll;
cvar_t	*bob_up;
cvar_t	*bob_pitch;
cvar_t	*bob_roll;

cvar_t	*sv_cheats;

cvar_t	*flood_msgs;
cvar_t	*flood_persecond;
cvar_t	*flood_waitdelay;

cvar_t	*sv_maplist;
//TDM++
cvar_t	*allow_bfg;
cvar_t	*allow_powerups;
cvar_t	*allow_gibs;
cvar_t	*allow_hud;
cvar_t	*allow_hand3;
cvar_t	*admin_password;
cvar_t	*port;
cvar_t	*stinkyboy;
cvar_t	*fastweapons;
cvar_t	*score_a;
cvar_t	*score_b;
cvar_t	*timeleft;
cvar_t	*instagib;
cvar_t	*domination;
cvar_t	*sv_configlist;
cvar_t	*sv_adminlist;
cvar_t	*sv_spawnrandom;
cvar_t	*sv_spawnrandom_numtries;
cvar_t	*sv_displaynamechange;
cvar_t	*sv_obsmode;
cvar_t	*sv_spawn_invincible;
cvar_t	*sv_referee_tag;
cvar_t	*sv_referee_flags;

cvar_t	*sv_log_connect;
cvar_t	*sv_log_admin;
cvar_t	*sv_log_change;
cvar_t	*sv_log_votes;

cvar_t	*allow_vote_dmf;
cvar_t	*allow_vote_tl;
cvar_t	*allow_vote_bfg;
cvar_t	*allow_vote_powerups;
cvar_t	*allow_vote_map;
cvar_t	*allow_vote_config;
cvar_t	*allow_vote_kick;
cvar_t	*allow_vote_fastweapons;
cvar_t	*allow_vote_tp;
cvar_t	*allow_vote_hud;
cvar_t	*allow_vote_hand3;

#ifdef __linux__
cvar_t	*ud_filename;
cvar_t	*ud_address;
cvar_t	*ud_restart;
cvar_t	*ud_time;
#endif
//TDM--

field_t fields[] = {
	{"classname", FOFS(classname), F_LSTRING},
	{"model", FOFS(model), F_LSTRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"accel", FOFS(accel), F_FLOAT},
	{"decel", FOFS(decel), F_FLOAT},
	{"target", FOFS(target), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"pathtarget", FOFS(pathtarget), F_LSTRING},
	{"deathtarget", FOFS(deathtarget), F_LSTRING},
	{"killtarget", FOFS(killtarget), F_LSTRING},
	{"combattarget", FOFS(combattarget), F_LSTRING},
	{"message", FOFS(message), F_LSTRING},
	{"team", FOFS(team), F_LSTRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"delay", FOFS(delay), F_FLOAT},
	{"random", FOFS(random), F_FLOAT},
	{"move_origin", FOFS(move_origin), F_VECTOR},
	{"move_angles", FOFS(move_angles), F_VECTOR},
	{"style", FOFS(style), F_INT},
	{"count", FOFS(count), F_INT},
	{"health", FOFS(health), F_INT},
	{"sounds", FOFS(sounds), F_INT},
	{"light", 0, F_IGNORE},
	{"dmg", FOFS(dmg), F_INT},
	{"mass", FOFS(mass), F_INT},
	{"volume", FOFS(volume), F_FLOAT},
	{"attenuation", FOFS(attenuation), F_FLOAT},
	{"map", FOFS(map), F_LSTRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"angle", FOFS(s.angles), F_ANGLEHACK},

	{"goalentity", FOFS(goalentity), F_EDICT, FFL_NOSPAWN},
	{"movetarget", FOFS(movetarget), F_EDICT, FFL_NOSPAWN},
	{"enemy", FOFS(enemy), F_EDICT, FFL_NOSPAWN},
	{"oldenemy", FOFS(oldenemy), F_EDICT, FFL_NOSPAWN},
	{"activator", FOFS(activator), F_EDICT, FFL_NOSPAWN},
	{"groundentity", FOFS(groundentity), F_EDICT, FFL_NOSPAWN},
	{"teamchain", FOFS(teamchain), F_EDICT, FFL_NOSPAWN},
	{"teammaster", FOFS(teammaster), F_EDICT, FFL_NOSPAWN},
	{"owner", FOFS(owner), F_EDICT, FFL_NOSPAWN},
	{"mynoise", FOFS(mynoise), F_EDICT, FFL_NOSPAWN},
	{"mynoise2", FOFS(mynoise2), F_EDICT, FFL_NOSPAWN},
	{"target_ent", FOFS(target_ent), F_EDICT, FFL_NOSPAWN},
	{"chain", FOFS(chain), F_EDICT, FFL_NOSPAWN},

	{"prethink", FOFS(prethink), F_FUNCTION, FFL_NOSPAWN},
	{"think", FOFS(think), F_FUNCTION, FFL_NOSPAWN},
	{"blocked", FOFS(blocked), F_FUNCTION, FFL_NOSPAWN},
	{"touch", FOFS(touch), F_FUNCTION, FFL_NOSPAWN},
	{"use", FOFS(use), F_FUNCTION, FFL_NOSPAWN},
	{"pain", FOFS(pain), F_FUNCTION, FFL_NOSPAWN},
	{"die", FOFS(die), F_FUNCTION, FFL_NOSPAWN},

	{"stand", FOFS(monsterinfo.stand), F_FUNCTION, FFL_NOSPAWN},
	{"idle", FOFS(monsterinfo.idle), F_FUNCTION, FFL_NOSPAWN},
	{"search", FOFS(monsterinfo.search), F_FUNCTION, FFL_NOSPAWN},
	{"walk", FOFS(monsterinfo.walk), F_FUNCTION, FFL_NOSPAWN},
	{"run", FOFS(monsterinfo.run), F_FUNCTION, FFL_NOSPAWN},
	{"dodge", FOFS(monsterinfo.dodge), F_FUNCTION, FFL_NOSPAWN},
	{"attack", FOFS(monsterinfo.attack), F_FUNCTION, FFL_NOSPAWN},
	{"melee", FOFS(monsterinfo.melee), F_FUNCTION, FFL_NOSPAWN},
	{"sight", FOFS(monsterinfo.sight), F_FUNCTION, FFL_NOSPAWN},
	{"checkattack", FOFS(monsterinfo.checkattack), F_FUNCTION, FFL_NOSPAWN},
	{"currentmove", FOFS(monsterinfo.currentmove), F_MMOVE, FFL_NOSPAWN},

	{"endfunc", FOFS(moveinfo.endfunc), F_FUNCTION, FFL_NOSPAWN},

	// temp spawn vars -- only valid when the spawn function is called
	{"lip", STOFS(lip), F_INT, FFL_SPAWNTEMP},
	{"distance", STOFS(distance), F_INT, FFL_SPAWNTEMP},
	{"height", STOFS(height), F_INT, FFL_SPAWNTEMP},
	{"noise", STOFS(noise), F_LSTRING, FFL_SPAWNTEMP},
	{"pausetime", STOFS(pausetime), F_FLOAT, FFL_SPAWNTEMP},
	{"item", STOFS(item), F_LSTRING, FFL_SPAWNTEMP},

//need for item field in edict struct, FFL_SPAWNTEMP item will be skipped on saves
	{"item", FOFS(item), F_ITEM},

	{"gravity", STOFS(gravity), F_LSTRING, FFL_SPAWNTEMP},
	{"sky", STOFS(sky), F_LSTRING, FFL_SPAWNTEMP},
	{"skyrotate", STOFS(skyrotate), F_FLOAT, FFL_SPAWNTEMP},
	{"skyaxis", STOFS(skyaxis), F_VECTOR, FFL_SPAWNTEMP},
	{"minyaw", STOFS(minyaw), F_FLOAT, FFL_SPAWNTEMP},
	{"maxyaw", STOFS(maxyaw), F_FLOAT, FFL_SPAWNTEMP},
	{"minpitch", STOFS(minpitch), F_FLOAT, FFL_SPAWNTEMP},
	{"maxpitch", STOFS(maxpitch), F_FLOAT, FFL_SPAWNTEMP},
	{"nextmap", STOFS(nextmap), F_LSTRING, FFL_SPAWNTEMP},
	{"rune1", STOFS(rune1), F_LSTRING, FFL_SPAWNTEMP},
	{"rune2", STOFS(rune2), F_LSTRING, FFL_SPAWNTEMP},
	{"rune3", STOFS(rune3), F_LSTRING, FFL_SPAWNTEMP},

	{0, 0, 0, 0}

};

void SpawnEntities (char *mapname, char *entities, char *spawnpoint);
void ClientThink (edict_t *ent, usercmd_t *cmd);
qboolean ClientConnect (edict_t *ent, char *userinfo);
void ClientUserinfoChanged (edict_t *ent, char *userinfo);
void ClientDisconnect (edict_t *ent);
void ClientBegin (edict_t *ent);
void ClientCommand (edict_t *ent);
void RunEntity (edict_t *ent);
void WriteGame (char *filename, qboolean autosave);
void ReadGame (char *filename);
void WriteLevel (char *filename);
void ReadLevel (char *filename);
void InitGame (void);
void G_RunFrame (void);


//===================================================================

/*
============
InitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/

void InitGame (void)
{
	cvar_t	*g_features;
	cvar_t	*sv_features;

	gi.dprintf ("==== InitGame: " TDMVERSION " ====\n");
	gi.dprintf ("by Harven, onimusha & As_Best (http://tdm.quake2.com.pl)\n");

	InitHashTables(); //plugins functions

	gi.cvar_set("deathmatch", "1");
	gi.cvar_forceset("deathmatch", "1");

	gun_x = gi.cvar ("gun_x", "0", 0);
	gun_y = gi.cvar ("gun_y", "0", 0);
	gun_z = gi.cvar ("gun_z", "0", 0);

	//FIXME: sv_ prefix is wrong for these
	sv_rollspeed = gi.cvar ("sv_rollspeed", "200", 0);
	sv_rollangle = gi.cvar ("sv_rollangle", "2", 0);
	sv_maxvelocity = gi.cvar ("sv_maxvelocity", "2000", 0);
	sv_gravity = gi.cvar ("sv_gravity", "800", 0);

	// noset vars
	dedicated = gi.cvar ("dedicated", "0", CVAR_NOSET);

	// latched vars
	sv_cheats = gi.cvar ("cheats", "0", CVAR_SERVERINFO|CVAR_LATCH);
	gi.cvar ("gamename", TDMVERSION , CVAR_SERVERINFO | CVAR_LATCH);
	gi.cvar_forceset("gamename", TDMVERSION);
	gi.cvar ("gamedate", __DATE__ , CVAR_SERVERINFO | CVAR_LATCH);
	gi.cvar_forceset("gamedate", __DATE__);

	maxclients = gi.cvar ("maxclients", "12", CVAR_SERVERINFO | CVAR_LATCH);
	maxspectators = gi.cvar ("maxspectators", "4", CVAR_SERVERINFO);
	deathmatch = gi.cvar ("deathmatch", "0", CVAR_LATCH);
	skill = gi.cvar ("skill", "1", CVAR_LATCH);
	maxentities = gi.cvar ("maxentities", "1024", CVAR_LATCH);

	// change anytime vars
	dmflags = gi.cvar ("dmflags", "1072", CVAR_SERVERINFO);
	fraglimit = gi.cvar ("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = gi.cvar ("timelimit", "20", CVAR_SERVERINFO);
	password = gi.cvar ("password", "", CVAR_USERINFO);
	spectator_password = gi.cvar ("spectator_password", "", CVAR_USERINFO);
	needpass = gi.cvar ("needpass", "0", CVAR_SERVERINFO);
	filterban = gi.cvar ("filterban", "1", 0);

	g_select_empty = gi.cvar ("g_select_empty", "0", CVAR_ARCHIVE);

	run_pitch = gi.cvar ("run_pitch", "0.002", 0);
	run_roll = gi.cvar ("run_roll", "0.005", 0);
	bob_up  = gi.cvar ("bob_up", "0.005", 0);
	bob_pitch = gi.cvar ("bob_pitch", "0.002", 0);
	bob_roll = gi.cvar ("bob_roll", "0.002", 0);

	// flood control
	flood_msgs = gi.cvar ("flood_msgs", "4", 0);
	flood_persecond = gi.cvar ("flood_persecond", "4", 0);
	flood_waitdelay = gi.cvar ("flood_waitdelay", "10", 0);

	// dm map list
	sv_maplist = gi.cvar ("sv_maplist", "maps.lst", 0);

	port = gi.cvar ("port", "", 0);

	// items
	InitItems ();

	Com_sprintf (game.helpmessage1, sizeof(game.helpmessage1), "");

	Com_sprintf (game.helpmessage2, sizeof(game.helpmessage2), "");

	// initialize all entities for this game
	game.maxentities = maxentities->value;
	g_edicts =  gi.TagMalloc (game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;
	globals.max_edicts = game.maxentities;

	// initialize all clients for this game
	game.maxclients = maxclients->value;
	game.is_old_score = false;
	game.clients = gi.TagMalloc (game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.num_edicts = game.maxclients+1;

	//TDM++
	domination = gi.cvar ("domination", "0", CVAR_SERVERINFO);
	if (domination->value)
	{
		Com_sprintf(game.teamB_name, sizeof(game.teamB_name), "Blue");
		Com_sprintf(game.teamA_name, sizeof(game.teamA_name), "Red");
	}
	else
	{
		Com_sprintf(game.teamB_name, sizeof(game.teamB_name), "B");
		Com_sprintf(game.teamA_name, sizeof(game.teamA_name), "A");
	}
	Com_sprintf(game.teamA_skin, sizeof(game.teamA_skin), "male/grunt");
	Com_sprintf(game.teamB_skin, sizeof(game.teamB_skin), "female/athena");
	game.teamA_captain = NULL;
	game.teamB_captain = NULL;
	game.tp = 2;

	score_a = gi.cvar ("#Score_A", "WARMUP", CVAR_SERVERINFO);
	score_b = gi.cvar ("#Score_B", "WARMUP", CVAR_SERVERINFO);
	timeleft = gi.cvar ("#Time_Left", "WARMUP", CVAR_SERVERINFO);
	allow_bfg = gi.cvar ("allow_bfg", "1", CVAR_SERVERINFO);
	allow_powerups = gi.cvar ("allow_powerups", "1", CVAR_SERVERINFO);
	allow_gibs = gi.cvar ("allow_gibs", "1", CVAR_SERVERINFO);
	allow_hud = gi.cvar ("allow_hud", "1", CVAR_SERVERINFO);
	allow_hand3 = gi.cvar ("allow_hand3", "0", CVAR_SERVERINFO );
	stinkyboy = gi.cvar ("stinkyboy", "0", 0);
	fastweapons = gi.cvar ("fastweapons", "0", CVAR_SERVERINFO);
	instagib = gi.cvar ("instagib", "0", CVAR_SERVERINFO);
	sv_configlist = gi.cvar ("sv_configs", "", 0);
	sv_adminlist = gi.cvar ("sv_adminlist_filename", "", 0);
	sv_spawnrandom = gi.cvar ("sv_spawnrandom", "0", 0);
	sv_spawnrandom_numtries = gi.cvar ("sv_spawnrandom_numtries", "0", 0); //unlimited
	sv_displaynamechange = gi.cvar ("sv_displaynamechange", "1", 0);
	sv_obsmode = gi.cvar ("sv_obsmode", "1", 0); //whisper
	sv_spawn_invincible = gi.cvar ("sv_spawn_invincible", "0", 0);
	sv_referee_tag = gi.cvar ("sv_referee_tag", "[judge]", CVAR_SERVERINFO);
	sv_referee_flags = gi.cvar ("sv_referee_flags", "15", 0);

	sv_log_connect = gi.cvar ("sv_log_connect", "tdm.log", 0);
	sv_log_admin = gi.cvar ("sv_log_admin", "tdm.log", 0);
	sv_log_change = gi.cvar ("sv_log_change", "tdm.log", 0);
	sv_log_votes = gi.cvar ("sv_log_votes", "tdm.log", 0);

	allow_vote_dmf = gi.cvar ("allow_vote_dmf", "0", 0);
	allow_vote_tl = gi.cvar ("allow_vote_tl", "1", 0);
	allow_vote_bfg = gi.cvar ("allow_vote_bfg", "1", 0);
	allow_vote_powerups = gi.cvar ("allow_vote_powerups", "1", 0);
	allow_vote_map = gi.cvar ("allow_vote_map", "1", 0);
	allow_vote_config = gi.cvar ("allow_vote_config", "0", 0);
	allow_vote_kick = gi.cvar ("allow_vote_kick", "1", 0);
	allow_vote_fastweapons = gi.cvar ("allow_vote_fastweapons", "1", 0);
	allow_vote_tp = gi.cvar ("allow_vote_tp", "1", 0);
	allow_vote_hud = gi.cvar ("allow_vote_hud", "1", 0);
	allow_vote_hand3 = gi.cvar ("allow_vote_hand3", "0", 0);

#if defined __linux__ && defined UDSYSTEM
	ud_filename = gi.cvar("ud_filename", "gamei386.so", 0);
	ud_address = gi.cvar("ud_address", "http://gingers.rulez.pl/tdm/update.php", 0);
	ud_restart = gi.cvar("ud_restart", "1", 0);
	ud_time = gi.cvar("ud_time", "3", 0);
#endif

	g_features = gi.cvar ("g_features", "0", CVAR_NOSET);
	gi.cvar_forceset ("g_features", va("%d", GMF_CLIENTNUM | GMF_MVDSPEC));

	sv_features = gi.cvar ("sv_features", NULL, 0);
	if (sv_features)
		game.server_features = (int)sv_features->value;
	else
		game.server_features = 0;


	init_mm();
	srand(time(NULL));
	LoadBanList();
	LoadAdminList();
	InitSavedPlayers();

	LoadPlugins();
	//TDM--
}

void ShutdownGame (void)
{
	struct ban_s *IPBan;
	struct admin_s *admin;

	gi.dprintf ("==== ShutdownGame ====\n");

	//TDM++
	FreeOldScores();
	FreeSavedPlayers();
	FreeSpawnRanges();

	WriteBanList();
	WriteAdminList();

	IPBan = game.ban_first;
	while(IPBan)
	{
		UNLINK(IPBan, game.ban_first, game.ban_last, next, prev);
		free(IPBan);
		IPBan = game.ban_first;
	}

	admin = game.admin_first;
	while(admin)
	{
		UNLINK(admin, game.admin_first, game.admin_last, next, prev);
		free(admin);
		admin = game.admin_first;
	}
	if (level.spawnPoints)
	{
		free(level.spawnPoints);
		level.spawnPoints = NULL;
	}
	UnloadPlugins();
	FreeHashTables();
//TDM--
	gi.FreeTags (TAG_LEVEL);
	gi.FreeTags (TAG_GAME);
}


/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/

void WriteGame (char *filename, qboolean autosave)
{
}
void ReadGame (char *filename)
{
}
void WriteLevel (char *filename)
{
}
void ReadLevel (char *filename)
{
}

typedef void (*bprintfFunction) (int printlevel, char *fmt, ...);
bprintfFunction orgBprintf;

void tdmBprintf(int printlevel, char *fmt, ...)
{
	va_list	argptr;
	char string[2048];

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	EventCall(EVENT_BPRINTF, printlevel, (long)string);

	if (string[0] != 0)
	{
		if (printlevel & ATTR_PRINT_LOW)
			orgBprintf(PRINT_LOW, string);
		else if (printlevel & ATTR_PRINT_HIGH)
			orgBprintf(PRINT_HIGH, string);
		else if (printlevel & ATTR_PRINT_CHAT)
			orgBprintf(PRINT_CHAT, string);
		else
			orgBprintf(PRINT_HIGH, string);
	}
}
game_export_t *GetGameAPI (game_import_t *import)
{
	gi = *import;

	orgBprintf = gi.bprintf;
	gi.bprintf = tdmBprintf;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.WriteGame = WriteGame;
	globals.ReadGame = ReadGame;
	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	return &globals;
}

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	gi.error (ERR_FATAL, "%s", text);
}

void Com_Printf (char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	gi.dprintf ("%s", text);
}

#endif

//======================================================================


/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames (void)
{
	int		i;
	edict_t	*ent;

	// calc the player views now that all pushing
	// and damage has been added
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		ClientEndServerFrame (ent);
	}

}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
edict_t *CreateTargetChangeLevel(char *map)
{
	edict_t *ent;

	ent = G_Spawn ();
	ent->classname = "target_changelevel";
	Com_sprintf(level.nextmap, sizeof(level.nextmap), "%s", map);
	ent->map = level.nextmap;
	return ent;
}

/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel (qboolean nextmap, qboolean with_intermission)
{
	int		i;
	edict_t	*cl;

	for (i=0 ; i<game.maxclients ; i++)
	{
		cl = &g_edicts[1+i];
		if (!cl->inuse)
			continue;
		
		if (cl->client->pers.save_data.autorecord)
		{
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("stop\n");
			gi.unicast(cl, true);
		}

		cl->client->pers.save_data.ready_or_not = false;
	}

	if (((int)dmflags->value & DF_SAME_LEVEL) && !nextmap)
	{
		if ( with_intermission )
			BeginIntermission (CreateTargetChangeLevel (level.mapname) );
		else
		{
			level.changemap = level.mapname;
			level.exitintermission = true;
		}
		return;
	}
	else
	{
		if (level.nextmap[0]) // go to a specific map
		{
			if (IsValidMap(level.nextmap, false))
			{
				if ( with_intermission )
					BeginIntermission (CreateTargetChangeLevel (level.nextmap) );
				else
					level.changemap = level.nextmap;
			}
			else
			{
				gi.dprintf("BUG: %s not on the map list", level.nextmap);
				if ( with_intermission )
					BeginIntermission (CreateTargetChangeLevel (level.mapname) );
				else
					level.changemap = level.mapname;
			}
		}
		else
		{
			if ( with_intermission )
				BeginIntermission (CreateTargetChangeLevel (level.mapname) );
			else
				level.changemap = level.mapname;
		}

		if ( with_intermission == false )
			level.exitintermission = true;
	}
}


/*
=================
CheckNeedPass
=================
*/
void CheckNeedPass (void)
{
	int need;

	// if password or spectator_password has changed, update needpass
	// as needed
	if (password->modified || spectator_password->modified) 
	{
		password->modified = spectator_password->modified = false;

		need = 0;

		if (*password->string && Q_stricmp(password->string, "none"))
			need |= 1;
		if (*spectator_password->string && Q_stricmp(spectator_password->string, "none"))
			need |= 2;

		gi.cvar_set("needpass", va("%d", need));
	}
}

/*
=================
CheckDMRules
=================
*/
void CheckDMRules (void)
{
	char	text[32];
	int		i;
	struct admin_s *adminCheck;

	if (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH)
	{
		if (level.countdown_framenum == level.framenum-1 || level.match_state == SUDDENDEATH)
		{
			if (level.match_state != OVERTIME && level.match_state != SUDDENDEATH && level.teamA_score == level.teamB_score)
			{
				level.countdown_framenum = level.framenum + GAMESECONDS(300) + 1;
				level.match_state = OVERTIME;
				gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_MAROON, "OVER TIME has started!\n");
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "5 minutes left.\n");
				return;
			}
			else if (level.match_state == OVERTIME && level.teamA_score == level.teamB_score)
			{
				sprintf(text, "SUDDEN DEATH");
				if (strcmp(level.status, text))
				{
					strcpy(level.status, text);
					for (i=0; i<strlen(text); i++)
						text[i] |= 128;
					gi.configstring(CS_AIRACCEL-2, text);
				}
				level.match_state = SUDDENDEATH;
				gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_MAROON, "SUDDEN DEATH mode has started!\n");
				return;
			}
			else if (level.match_state == SUDDENDEATH && level.teamA_score == level.teamB_score)
				return;

			//remove timed-out admins
			for (adminCheck = game.admin_first; adminCheck; adminCheck = adminCheck->next)
			{
				if ((strlen(adminCheck->password) > 0) && adminCheck->used == true)
				{
					if (adminCheck->time > 0)
						adminCheck->time -= 1;

					if (adminCheck->time == 0)
						DeleteAdmin(adminCheck);
				}
			}

			WriteAdminList();

			level.match_state = END;

			gi.bprintf (ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Timelimit hit. Match Ended.\n");

			CreateOldScores();
			FreeSavedPlayers();

			level.end_framenum = level.framenum;

			EndDMLevel (false, true);
			return;
		}
	}
}


/*
=============
ExitLevel
=============
*/
void ExitLevel (void)
{
	int		i;
	edict_t	*ent;
	char	command [256];

	Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString (command);
	level.changemap = NULL;
	ClientEndServerFrames ();
	level.exitintermission = 0;
	//level.intermissiontime = 0;

	// clear some things before going to next level
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->health > ent->client->pers.max_health)
			ent->health = ent->client->pers.max_health;
	}

}
/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
void G_RunFrame (void)
{
	int		i;
	int		client_num=0;
	edict_t	*ent;
	char	text[32];
	int	mins, secs;

	time(&game.servertime);

	level.intermissionDrifting += 0.1;
	if (level.intermissionDrifting > 180)
		level.intermissionDrifting = 0;

	level.framenum++;
	level.time = (float)level.framenum;//*FRAMETIME;

	AllowSyncFunctionsCalls();
	DisallowSyncFunctionsCalls();

	//TDM++
	if (level.paused)
	{
		qboolean skip=false;

		if (level.pause_time != -1)
			level.pause_time -= GAMESECONDS(FRAMETIME);

		if (level.pause_time <= 0 && level.pause_time > -1)
		{
			skip=true;
			level.paused = false;
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Game resumed.\n");
		}

		if (!skip)
		{
			level.framenum--;
			level.time = (float)level.framenum;//*FRAMETIME;

			if ((level.pause_time == GAMESECONDS(3)) || (level.pause_time == GAMESECONDS(2)) || (level.pause_time == GAMESECONDS(1)))
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Time in: %d\n", (qtime)(FRAMETIME*(float)level.pause_time));

			mins = (level.countdown_framenum - level.framenum)/600;
			secs = ((level.countdown_framenum - level.framenum)-(mins*600))/10;

			if (level.pause_time == -1)
				sprintf(text, "%2d:%02d (PAUSED)", mins, secs);
			else
				sprintf(text, "%2d:%02d (%02d)", mins, secs, (int)level.pause_time);
			if (strcmp(level.status, text))
			{
				strcpy(level.status, text);
				for (i=0; i<strlen(text); i++)
					text[i] |= 128;
				gi.configstring(CS_AIRACCEL-2, text);
			}
			strcpy(level.status, text);
			for(i=0; i<game.maxclients; i++)
			{
				ent = &g_edicts[1+i];
	
				CheckCaptain(ent);

				if (!ent->inuse)
					continue;

				if (ent->client->showscores && !(level.pause_3sec))
				{
					DeathmatchScoreboardMessage (ent, ent->enemy);
					gi.unicast (ent, false);
				}

				if (level.pause_time == GAMESECONDS(3))
				{
					gi.WriteByte (svc_stufftext);
					gi.WriteString ("play world/fuseout.wav\n");
					gi.unicast(ent, true);
				}

				if (ent->client->pers.save_data.team == TEAM_NONE)
					G_SetSpectatorStats(ent);
				else
					G_SetStats (ent);
			}
			if (level.pause_3sec == 0)
				level.pause_3sec = 5;
			level.pause_3sec -= 1;

			CheckProposes();

			CheckMatchState();

			EventCall(EVENT_GRUNFRAME, 0, 0);

			return;
		}
	}
	//TDM--

	EventCall(EVENT_GRUNFRAME, 0, 0);

	// exit intermissions

	if (level.exitintermission)
	{
		if (level.exitintermissionblend > 1)
		{
			ExitLevel ();
			return;
		}
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
//TDM++
	level.num_A_players = 0;
	level.num_B_players = 0;
	level.num_spectators = 0;
//TDM--
	ent = &g_edicts[0];
	for (i=0 ; i<globals.num_edicts ; i++, ent++)
	{
		//TDM++
		if (i > 0 && i <= maxclients->value && level.framenum > GAMESECONDS(15)) //15 seconds
			CheckCaptain(ent);
		//TDM--

		if (!ent->inuse)
			continue;

		level.current_entity = ent;
		VectorCopy (ent->s.origin, ent->s.old_origin);

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;
			if ( !(ent->flags & (FL_SWIM|FL_FLY)) && (ent->svflags & SVF_MONSTER) )
			{
				M_CheckGround (ent);
			}
		}

		if (i > 0 && i <= maxclients->value)
		{
			ClientBeginServerFrame (ent);
//TDM++
			if ( ent->client->pers.mvdspec == false )
			{
				if (ent->client->pers.save_data.team == TEAM_A)
					level.num_A_players += 1;
				else if (ent->client->pers.save_data.team == TEAM_B)
					level.num_B_players += 1;
				else
					level.num_spectators += 1;
				client_num++;
			}
//TDM--
			if (level.end_framenum+i == level.framenum && level.match_state == END)
			{
				ent->client->showoldscore = true;
				DisplayOldScore(ent);
			}

			continue;
		}

		G_RunEntity (ent);
	}

	if (level.match_state == END && level.end_framenum+30 == level.framenum)
		MatchEndInfo();

	CheckProposes();

	CheckMatchState();

	Update_ServerInfo();

	UpdateConfigStrings();

#if defined __linux__ && defined UDSYSTEM
	if (level.update_break <= level.time)
		CheckForUpdate();
#endif
	//TDM--

	CheckBans();

	// see if it is time to end a deathmatch
	CheckDMRules();

	// see if needpass needs updated
	CheckNeedPass();

	// build the playerstate_t structures for all players
	ClientEndServerFrames();

	if (level.exitintermission && level.exitintermissionblend < 1 )
		level.exitintermissionblend += 0.07;
}
