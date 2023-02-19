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
/* g_local.h -- local definitions for game module*/

#include "q_shared.h"

/* define GAME_INCLUDE so that game.h does not define the
** short, server-visible gclient_t and edict_t structures,
** because we define the full size ones in this file
*/
#define	GAME_INCLUDE
#include "game.h"

/* the "gameversion" client command will print this plus compile date*/
#define	GAMEVERSION	"tdm"
/* protocol bytes that can be directly added to messages*/
#define	svc_muzzleflash		1
#define	svc_muzzleflash2	2
#define	svc_temp_entity		3
#define	svc_layout			4
#define	svc_inventory		5
#define	svc_stufftext		11
#define svc_configstring	13

/*==================================================================*/

/* view pitching times*/
#define DAMAGE_TIME		0.5
#define	FALL_TIME		0.3


/* edict->spawnflags
** these are set with checkboxes on each entity in the map editor*/
#define	SPAWNFLAG_NOT_EASY			0x00000100
#define	SPAWNFLAG_NOT_MEDIUM		0x00000200
#define	SPAWNFLAG_NOT_HARD			0x00000400
#define	SPAWNFLAG_NOT_DEATHMATCH	0x00000800
#define	SPAWNFLAG_NOT_COOP			0x00001000

/* edict->flags*/
#define	FL_FLY					0x00000001
#define	FL_SWIM					0x00000002	/*implied immunity to drowining*/
#define FL_IMMUNE_LASER			0x00000004
#define	FL_INWATER				0x00000008
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
#define FL_IMMUNE_SLIME			0x00000040
#define FL_IMMUNE_LAVA			0x00000080
#define	FL_PARTIALGROUND		0x00000100	/*not all corners are valid*/
#define	FL_WATERJUMP			0x00000200	/*player jumping out of water*/
#define	FL_TEAMSLAVE			0x00000400	/*not the first on the team*/
#define FL_NO_KNOCKBACK			0x00000800
#define FL_POWER_ARMOR			0x00001000	/*power armor (if any) is active*/
#define FL_RESPAWN				0x80000000	/*used for item respawning*/


#define	FRAMETIME		0.1f
#define SERVERFPS		10.0f
#define GAMESECONDS(x) 		((qtime)(SERVERFPS*x))

/*memory tags to allow dynamic memory to be cleaned up*/
#define	TAG_GAME	765		/*clear when unloading the dll*/
#define	TAG_LEVEL	766		/*clear when loading a new level*/


#define MELEE_DISTANCE	80

#define BODY_QUEUE_SIZE		8

typedef enum
{
	DAMAGE_NO,
	DAMAGE_YES,			/*will take damage if hit*/
	DAMAGE_AIM			/*auto targeting recognizes this*/
} damage_t;

typedef enum 
{
	WEAPON_READY, 
	WEAPON_ACTIVATING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} weaponstate_t;

typedef enum
{
	AMMO_BULLETS,
	AMMO_SHELLS,
	AMMO_ROCKETS,
	AMMO_GRENADES,
	AMMO_CELLS,
	AMMO_SLUGS
} ammo_t;


/*deadflag*/
#define DEAD_NO					0
#define DEAD_DYING				1
#define DEAD_DEAD				2
#define DEAD_RESPAWNABLE		3

/*range*/
#define RANGE_MELEE				0
#define RANGE_NEAR				1
#define RANGE_MID				2
#define RANGE_FAR				3

/*gib types*/
#define GIB_ORGANIC				0
#define GIB_METALLIC			1

/*monster ai flags*/
#define AI_STAND_GROUND			0x00000001
#define AI_TEMP_STAND_GROUND	0x00000002
#define AI_SOUND_TARGET			0x00000004
#define AI_LOST_SIGHT			0x00000008
#define AI_PURSUIT_LAST_SEEN	0x00000010
#define AI_PURSUE_NEXT			0x00000020
#define AI_PURSUE_TEMP			0x00000040
#define AI_HOLD_FRAME			0x00000080
#define AI_GOOD_GUY				0x00000100
#define AI_BRUTAL				0x00000200
#define AI_NOSTEP				0x00000400
#define AI_DUCKED				0x00000800
#define AI_COMBAT_POINT			0x00001000
#define AI_MEDIC				0x00002000
#define AI_RESURRECTING			0x00004000

/*monster attack state*/
#define AS_STRAIGHT				1
#define AS_SLIDING				2
#define	AS_MELEE				3
#define	AS_MISSILE				4

/*armor types*/
#define ARMOR_NONE				0
#define ARMOR_JACKET			1
#define ARMOR_COMBAT			2
#define ARMOR_BODY				3
#define ARMOR_SHARD				4

/*power armor types*/
#define POWER_ARMOR_NONE		0
#define POWER_ARMOR_SCREEN		1
#define POWER_ARMOR_SHIELD		2

/*handedness values*/
#define RIGHT_HANDED			0
#define LEFT_HANDED				1
#define CENTER_HANDED			2


/*game.serverflags values*/
#define SFL_CROSS_TRIGGER_1		0x00000001
#define SFL_CROSS_TRIGGER_2		0x00000002
#define SFL_CROSS_TRIGGER_3		0x00000004
#define SFL_CROSS_TRIGGER_4		0x00000008
#define SFL_CROSS_TRIGGER_5		0x00000010
#define SFL_CROSS_TRIGGER_6		0x00000020
#define SFL_CROSS_TRIGGER_7		0x00000040
#define SFL_CROSS_TRIGGER_8		0x00000080
#define SFL_CROSS_TRIGGER_MASK	0x000000ff


/*noise types for PlayerNoise*/
#define PNOISE_SELF				0
#define PNOISE_WEAPON			1
#define PNOISE_IMPACT			2


/*edict->movetype values*/
typedef enum
{
MOVETYPE_NONE,			/*never moves*/
MOVETYPE_NOCLIP,		/*origin and angles change with no interaction*/
MOVETYPE_PUSH,			/*no clip to world, push on box contact*/
MOVETYPE_STOP,			/*no clip to world, stops on box contact*/

MOVETYPE_WALK,			/*gravity*/
MOVETYPE_STEP,			/*gravity, special edge handling*/
MOVETYPE_FLY,
MOVETYPE_TOSS,			/*gravity*/
MOVETYPE_FLYMISSILE,	/*extra size to monsters*/
MOVETYPE_BOUNCE
} movetype_t;



typedef struct
{
	int		base_count;
	int		max_count;
	float	normal_protection;
	float	energy_protection;
	int		armor;
} gitem_armor_t;


/*gitem_t->flags*/
#define	IT_WEAPON		1		/*use makes active weapon*/
#define	IT_AMMO			2
#define IT_ARMOR		4
#define IT_STAY_COOP	8
#define IT_KEY			16
#define IT_POWERUP		32

/*gitem_t->weapmodel for weapons indicates model index*/
#define WEAP_BLASTER			1 
#define WEAP_SHOTGUN			2 
#define WEAP_SUPERSHOTGUN		3 
#define WEAP_MACHINEGUN			4 
#define WEAP_CHAINGUN			5 
#define WEAP_GRENADES			6 
#define WEAP_GRENADELAUNCHER	7 
#define WEAP_ROCKETLAUNCHER		8 
#define WEAP_HYPERBLASTER		9 
#define WEAP_RAILGUN			10
#define WEAP_BFG				11

typedef struct gitem_s
{
	char *classname; /*spawning name*/
	qboolean (*pickup)(struct edict_s *ent, struct edict_s *other);
	void (*use)(struct edict_s *ent, struct gitem_s *item);
	void (*drop)(struct edict_s *ent, struct gitem_s *item);
	void (*weaponthink)(struct edict_s *ent);
	char *pickup_sound;
	char *world_model;
	int world_model_flags;
	char *view_model;

	/*client side info*/
	char *icon;
	char *pickup_name; /*for printing on pickup*/
	int count_width; /*number of digits to display by icon*/

	int quantity; /*for ammo how much, for weapons how much is used per shot*/
	char *ammo; /*for weapons*/
	int flags; /*IT_* flags*/

	int weapmodel; /*weapon model index (for weapons)*/

	void *info;
	int tag;

	char *precaches; /*string of all models, sounds, and images this item will use*/
}gitem_t;

/*
** this structure is left intact through an entire game
** it should be initialized at dll load time, and read/written to
** the server.ssv file for savegames
*/
typedef struct
{
	char		helpmessage1[512];
	char		helpmessage2[512];
	int			helpchanged;	/*flash F1 icon if non 0, play sound*/
								/*and increment only if 1, 2, or 3*/

	gclient_t	*clients;		/*[maxclients]*/

	/*
	** can't store spawnpoint in level, because
	** it would get overwritten by the savegame restore
	*/
	char		spawnpoint[512];	/*needed for coop respawns*/

	/*store latched cvars here that we want to get at often*/
	int			maxclients;
	int			maxentities;

	/*cross level triggers*/
	int			serverflags;

	/*items*/
	int			num_items;

	qboolean	autosaved;

	//teams
	char		teamA_name[MAX_QPATH];
	char		teamB_name[MAX_QPATH];
	char		teamA_skin[MAX_QPATH];
	char		teamB_skin[MAX_QPATH];
	edict_t		*teamA_captain;
	edict_t		*teamB_captain;
	qboolean	teamA_locked;
	qboolean	teamB_locked;
	qboolean	is_old_score;

	int			tp;
	struct ban_s *ban_first;
	struct ban_s *ban_last;
	struct ban_s *banToCheck;
	struct admin_s *admin_first;
	struct admin_s *admin_last;
	time_t servertime;

	int server_features;
} game_locals_t;


/*
** this structure is cleared as each map is entered
** it is read/written to the level.sav file for savegames
*/

#define IS_SHOTGUN				0x00000001
#define IS_SSHOTGUN				0x00000002
#define IS_MACHINEGUN			0x00000004
#define IS_CHAINGUN				0x00000008
#define IS_GLAUNCHER			0x00000010
#define IS_RLAUNCHER			0x00000020
#define IS_HBLASTER				0x00000040
#define IS_RAILGUN				0x00000080
#define IS_BFG					0x00000100
#define IS_GRENADE				0x00000200

#define IS_MH					0x00000001
#define IS_BA					0x00000002
#define IS_CA					0x00000004
#define IS_JA					0x00000008
#define IS_QD					0x00000010
#define IS_IV					0x00000020
#define IS_PS					0x00000040

#define VOTE_MAP				0x00000001
#define VOTE_TIMELIMIT			0x00000002
#define VOTE_TP					0x00000004
#define VOTE_KICK				0x00000008
#define VOTE_DMFLAGS			0x00000010
#define VOTE_CONFIG				0x00000020
#define VOTE_BFG				0x00000040
#define VOTE_POWERUPS			0x00000080
#define VOTE_FASTWEAPONS		0x00000100
#define VOTE_HUD				0x00000200
#define VOTE_HAND3				0x00000400

typedef struct
{
	char		map[MAX_QPATH];
	int			timelimit;
	int			tp;
	int			kick;
	int			dmflags;
	char		config[MAX_QPATH];
	qboolean	bfg;
	qboolean	powerups;
	qboolean	fastweapons;
	qboolean	hud;
	qboolean	hand3;

	edict_t		*voter;
	qboolean	vote_active;
	int			vote_what;
	qtime		vote_time;
	int			vote_yes;
	int			vote_no;
	qboolean	update_vote;
	qboolean	multivote;
} vote_struct;

#define PAUSER_ACAPTAIN 1
#define PAUSER_BCAPTAIN 2
#define PAUSER_ADMIN 3

typedef struct
{
	edict_t *ent;
	qboolean free;
	unsigned int range_from_players;
}spawn_point_t;

typedef struct
{
	qtime			framenum;
	qtime		time;

	char		level_name[MAX_QPATH];	/*the descriptive name (Outer Base, etc)*/
	char		mapname[MAX_QPATH];		/*the server name (base1, etc)*/
	char		nextmap[MAX_QPATH];		/*go here when fraglimit is hit*/

	/*intermission state*/
	qtime		intermissiontime;		/*time the intermission was started*/
	char		*changemap;
	int			exitintermission;
	float		exitintermissionblend;
	vec3_t		intermission_origin;
	vec3_t		intermission_angle;

	edict_t		*sight_client;	/*changed once each frame for coop games*/

	edict_t		*sight_entity;
	int			sight_entity_framenum;
	edict_t		*sound_entity;
	int			sound_entity_framenum;
	edict_t		*sound2_entity;
	int			sound2_entity_framenum;

	int			pic_health;

	int			total_secrets;
	int			found_secrets;

	int			total_goals;
	int			found_goals;

	int			total_monsters;
	int			killed_monsters;

	edict_t		*current_entity;	/*entity running from G_RunFrame*/
	int			body_que;			/*dead bodies*/

	int			power_cubes;		/*ugly necessity for coop*/

//TDM++
	int			num_A_players;
	int			num_B_players;
	int			num_spectators;

	int			teamA_score;
	int			teamB_score;
	int			teamA_numpauses;
	int			teamB_numpauses;

	int			match_state;
	qtime		countdown_framenum;
	qtime		start_framenum;
	qtime		end_framenum;

	vote_struct vote;

	qboolean	update_score;
	char hud_setting_num;
	int			update_score_client_num;
	qboolean	paused;
	edict_t		*pauser;
	char pauserType;
	qtime		pause_time;
	int			pause_3sec;

	char		status[32];
	int			weapons;
	int			items;

	int			notready;

#ifdef __linux__	
	qtime		update_break;	
#endif

	//domination
	edict_t		*rune1;
	edict_t		*rune2;
	edict_t		*rune3;
	char		*rune1_name;
	char		*rune2_name;
	char		*rune3_name;
	int			rune1_owner;
	int			rune2_owner;
	int			rune3_owner;

	float intermissionDrifting;

	spawn_point_t *spawnPoints;
	int numSpawnPoints;
//TDM--
} level_locals_t;

/*
** spawn_temp_t is only used to hold entity field values that
** can be set from the editor, but aren't actualy present
** in edict_t during gameplay
*/
typedef struct
{
	/*world vars*/
	char		*sky;
	float		skyrotate;
	vec3_t		skyaxis;
	char		*nextmap;

	int			lip;
	int			distance;
	int			height;
	char		*noise;
	float		pausetime;
	char		*item;
	char		*gravity;

	float		minyaw;
	float		maxyaw;
	float		minpitch;
	float		maxpitch;

	//domination
	char		*rune1;
	char		*rune2;
	char		*rune3;
} spawn_temp_t;


typedef struct
{
	/*fixed data*/
	vec3_t		start_origin;
	vec3_t		start_angles;
	vec3_t		end_origin;
	vec3_t		end_angles;

	int			sound_start;
	int			sound_middle;
	int			sound_end;

	double		accel;
	double		speed;
	double		decel;
	double		distance;

	float		wait;

	/*state data*/
	int			state;
	vec3_t		dir;
	double		current_speed;
	double		move_speed;
	double		next_speed;
	double		remaining_distance;
	double		decel_distance;
	void		(*endfunc)(edict_t *);
} moveinfo_t;


typedef struct
{
	void	(*aifunc)(edict_t *self, float dist);
	float	dist;
	void	(*thinkfunc)(edict_t *self);
} mframe_t;

typedef struct
{
	int			firstframe;
	int			lastframe;
	mframe_t	*frame;
	void		(*endfunc)(edict_t *self);
} mmove_t;

typedef struct
{
	mmove_t		*currentmove;
	int			aiflags;
	int			nextframe;
	float		scale;

	void		(*stand)(edict_t *self);
	void		(*idle)(edict_t *self);
	void		(*search)(edict_t *self);
	void		(*walk)(edict_t *self);
	void		(*run)(edict_t *self);
	void		(*dodge)(edict_t *self, edict_t *other, float eta);
	void		(*attack)(edict_t *self);
	void		(*melee)(edict_t *self);
	void		(*sight)(edict_t *self, edict_t *other);
	qboolean	(*checkattack)(edict_t *self);

	qtime		pausetime;
	float		attack_finished;

	vec3_t		saved_goal;
	float		search_time;
	float		trail_time;
	vec3_t		last_sighting;
	int			attack_state;
	int			lefty;
	float		idle_time;
	int			linkcount;

	int			power_armor_type;
	int			power_armor_power;
} monsterinfo_t;

extern	game_locals_t	game;
extern	level_locals_t	level;
extern	game_import_t	gi;
extern	game_export_t	globals;
extern	spawn_temp_t	st;

extern	int	sm_meat_index;
extern	int	snd_fry;

extern	int	jacket_armor_index;
extern	int	combat_armor_index;
extern	int	body_armor_index;


/*means of death*/
#define MOD_UNKNOWN			0
#define MOD_BLASTER			1
#define MOD_SHOTGUN			2
#define MOD_SSHOTGUN		3
#define MOD_MACHINEGUN		4
#define MOD_CHAINGUN		5
#define MOD_GRENADE			6
#define MOD_G_SPLASH		7
#define MOD_ROCKET			8
#define MOD_R_SPLASH		9
#define MOD_HYPERBLASTER	10
#define MOD_RAILGUN			11
#define MOD_BFG_LASER		12
#define MOD_BFG_BLAST		13
#define MOD_BFG_EFFECT		14
#define MOD_HANDGRENADE		15
#define MOD_HG_SPLASH		16
#define MOD_WATER			17
#define MOD_SLIME			18
#define MOD_LAVA			19
#define MOD_CRUSH			20
#define MOD_TELEFRAG		21
#define MOD_FALLING			22
#define MOD_SUICIDE			23
#define MOD_HELD_GRENADE	24
#define MOD_EXPLOSIVE		25
#define MOD_BARREL			26
#define MOD_BOMB			27
#define MOD_EXIT			28
#define MOD_SPLASH			29
#define MOD_TARGET_LASER	30
#define MOD_TRIGGER_HURT	31
#define MOD_HIT				32
#define MOD_TARGET_BLASTER	33
#define MOD_FRIENDLY_FIRE	0x8000000

extern	int	meansOfDeath;


extern	edict_t			*g_edicts;

#define	FOFS(x) (int)&(((edict_t *)0)->x)
#define	STOFS(x) (int)&(((spawn_temp_t *)0)->x)
#define	LLOFS(x) (int)&(((level_locals_t *)0)->x)
#define	CLOFS(x) (int)&(((gclient_t *)0)->x)

#define random()	((rand () & 0x7fff) / ((float)0x7fff))
#define crandom()	(2.0 * (random() - 0.5))

extern	cvar_t	*maxentities;
extern	cvar_t	*deathmatch;
extern	cvar_t	*dmflags;
extern	cvar_t	*skill;
extern	cvar_t	*fraglimit;
extern	cvar_t	*timelimit;
extern	cvar_t	*password;
extern	cvar_t	*spectator_password;
extern	cvar_t	*needpass;
extern	cvar_t	*g_select_empty;
extern	cvar_t	*dedicated;

extern	cvar_t	*filterban;

extern	cvar_t	*sv_gravity;
extern	cvar_t	*sv_maxvelocity;

extern	cvar_t	*gun_x, *gun_y, *gun_z;
extern	cvar_t	*sv_rollspeed;
extern	cvar_t	*sv_rollangle;

extern	cvar_t	*run_pitch;
extern	cvar_t	*run_roll;
extern	cvar_t	*bob_up;
extern	cvar_t	*bob_pitch;
extern	cvar_t	*bob_roll;

extern	cvar_t	*sv_cheats;
extern	cvar_t	*maxclients;
extern	cvar_t	*maxspectators;

extern	cvar_t	*flood_msgs;
extern	cvar_t	*flood_persecond;
extern	cvar_t	*flood_waitdelay;

extern	cvar_t	*sv_maplist;

//TDM++
extern	cvar_t	*allow_bfg;
extern	cvar_t	*allow_powerups;
extern	cvar_t	*allow_gibs;
extern	cvar_t	*allow_hud;
extern	cvar_t	*allow_hand3;
extern	cvar_t	*admin_password;
extern	cvar_t	*port;
extern	cvar_t	*stinkyboy;
extern	cvar_t	*fastweapons;

extern	cvar_t	*score_a;
extern	cvar_t	*score_b;
extern	cvar_t	*timeleft;

extern	cvar_t	*instagib;
extern	cvar_t	*domination;
extern	cvar_t	*sv_configlist;
extern	cvar_t	*sv_adminlist;
extern	cvar_t	*sv_spawnrandom;
extern	cvar_t	*sv_spawnrandom_numtries;
extern	cvar_t	*sv_displaynamechange;
extern	cvar_t	*sv_obsmode;
extern	cvar_t	*sv_spawn_invincible;
extern	cvar_t	*sv_referee_tag;
extern	cvar_t	*sv_referee_flags;

extern	cvar_t	*sv_log_connect;
extern	cvar_t	*sv_log_admin;
extern	cvar_t	*sv_log_change;
extern	cvar_t	*sv_log_votes;

extern	cvar_t	*allow_vote_dmf;
extern	cvar_t	*allow_vote_tl;
extern	cvar_t	*allow_vote_bfg;
extern	cvar_t	*allow_vote_powerups;
extern	cvar_t	*allow_vote_map;
extern	cvar_t	*allow_vote_config;
extern	cvar_t	*allow_vote_kick;
extern	cvar_t	*allow_vote_fastweapons;
extern	cvar_t	*allow_vote_tp;
extern	cvar_t	*allow_vote_hud;
extern 	cvar_t	*allow_vote_hand3;

#ifdef __linux__
extern	cvar_t	*ud_filename;
extern	cvar_t	*ud_address;
extern	cvar_t	*ud_restart;
extern	cvar_t	*ud_time;
#endif
//TDM--

#define world	(&g_edicts[0])

/*item spawnflags*/
#define ITEM_TRIGGER_SPAWN		0x00000001
#define ITEM_NO_TOUCH			0x00000002
/* 6 bits reserved for editor flags*/
/* 8 bits used as power cube id bits for coop games*/
#define DROPPED_ITEM			0x00010000
#define	DROPPED_PLAYER_ITEM		0x00020000
#define ITEM_TARGETS_USED		0x00040000

/*
** fields are needed for spawning from the entity string
** and saving / loading games
*/
#define FFL_SPAWNTEMP		1
#define FFL_NOSPAWN			2

typedef enum {
	F_INT, 
	F_FLOAT,
	F_LSTRING,			/*string on disk, pointer in memory, TAG_LEVEL*/
	F_GSTRING,			/*string on disk, pointer in memory, TAG_GAME*/
	F_VECTOR,
	F_ANGLEHACK,
	F_EDICT,			/*index on disk, pointer in memory*/
	F_ITEM,				/*index on disk, pointer in memory*/
	F_CLIENT,			/*index on disk, pointer in memory*/
	F_FUNCTION,
	F_MMOVE,
	F_IGNORE
} fieldtype_t;

typedef struct
{
	char	*name;
	int		ofs;
	fieldtype_t	type;
	int		flags;
} field_t;


extern	field_t fields[];
extern	gitem_t	itemlist[];


/*
** g_cmds.c
*/
void Cmd_PutAway_f (edict_t *ent);
void Cmd_Help_f (edict_t *ent);
void Cmd_Score_f (edict_t *ent);
void Cmd_PlayerList_f(edict_t *ent);


/*
** g_items.c
*/
void PrecacheItem (gitem_t *it);
void InitItems (void);
void SetItemNames (void);
gitem_t	*FindItem (char *pickup_name);
gitem_t	*FindItemByClassname (char *classname);
#define	ITEM_INDEX(x) ((x)-itemlist)
edict_t *Drop_Item (edict_t *ent, gitem_t *item);
void SetRespawn (edict_t *ent, float delay);
void ChangeWeapon (edict_t *ent);
void SpawnItem (edict_t *ent, gitem_t *item);
void Think_Weapon (edict_t *ent);
int ArmorIndex (edict_t *ent);
int PowerArmorType (edict_t *ent);
gitem_t	*GetItemByIndex (int index);
qboolean Add_Ammo (edict_t *ent, gitem_t *item, int count);
void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

/*
** g_utils.c
*/
qboolean	KillBox (edict_t *ent);
void	G_ProjectSource (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
edict_t *G_Find (edict_t *from, int fieldofs, char *match);
edict_t *findradius (edict_t *from, vec3_t org, float rad);
edict_t *G_PickTarget (char *targetname);
void	G_UseTargets (edict_t *ent, edict_t *activator);
void	G_SetMovedir (vec3_t angles, vec3_t movedir);

void	G_InitEdict (edict_t *e);
edict_t	*G_Spawn (void);
void	G_FreeEdict (edict_t *e);

void	G_TouchTriggers (edict_t *ent);
void	G_TouchSolids (edict_t *ent);

char	*G_CopyString (char *in);

float	*tv (float x, float y, float z);
char	*vtos (vec3_t v);

float vectoyaw (vec3_t vec);
void vectoangles (vec3_t vec, vec3_t angles);

/*
** g_combat.c
*/
qboolean OnSameTeam (edict_t *ent1, edict_t *ent2);
qboolean CanDamage (edict_t *targ, edict_t *inflictor);
void T_Damage (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, int mod);
qboolean T_RadiusDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod);

/*damage flags*/
#define DAMAGE_RADIUS			0x00000001	/*damage was indirect*/
#define DAMAGE_NO_ARMOR			0x00000002	/*armour does not protect from this damage*/
#define DAMAGE_ENERGY			0x00000004	/*damage is from an energy based weapon*/
#define DAMAGE_NO_KNOCKBACK		0x00000008	/*do not affect velocity, just view angles*/
#define DAMAGE_BULLET			0x00000010	/*damage is from a bullet (used for ricochets)*/
#define DAMAGE_NO_PROTECTION	0x00000020		/*armor, shields, invulnerability, and godmode have no effect*/

#define DEFAULT_BULLET_HSPREAD	300
#define DEFAULT_BULLET_VSPREAD	500
#define DEFAULT_SHOTGUN_HSPREAD	1000
#define DEFAULT_SHOTGUN_VSPREAD	500
#define DEFAULT_DEATHMATCH_SHOTGUN_COUNT	12
#define DEFAULT_SHOTGUN_COUNT	12
#define DEFAULT_SSHOTGUN_COUNT	20

/*
** g_monster.c
*/
void M_droptofloor (edict_t *ent);
void M_CatagorizePosition (edict_t *ent);
void M_CheckGround (edict_t *ent);

/*
** g_misc.c
*/
void ThrowHead (edict_t *self, char *gibname, int damage, int type);
void ThrowClientHead (edict_t *self, int damage);
void ThrowGib (edict_t *self, char *gibname, int damage, int type);
void BecomeExplosion1(edict_t *self);

/*
** g_weapon.c
*/
void ThrowDebris (edict_t *self, char *modelname, float speed, vec3_t origin);
qboolean fire_hit (edict_t *self, vec3_t aim, int damage, int kick);
void fire_bullet (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int mod);
void fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int mod);
void fire_blaster (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int effect, qboolean hyper);
void fire_grenade (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius);
void fire_grenade2 (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius, qboolean held);
void fire_rocket (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage);
void fire_rail (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick);
void fire_bfg (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius);

/*
** g_client.c
*/
void respawn (edict_t *ent, qboolean was_dead, qboolean telefrag);
void BeginIntermission (edict_t *targ);
void PutClientInServer (edict_t *ent, qboolean telefrag);
void InitClientPersistant (gclient_t *client);
void InitClientResp (gclient_t *client);
void InitBodyQue (void);
void ClientBeginServerFrame (edict_t *ent);
void spectator_respawn(edict_t *ent, qboolean autom);
void Write_ConfigString(void);
void ClientUserinfoChanged (edict_t *ent, char *userinfo);

/*
** g_player.c
*/
void player_pain (edict_t *self, edict_t *other, float kick, int damage);
void player_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

/*
** g_svcmds.c
*/
void	ServerCommand (void);
qboolean SV_FilterPacket (char *from);

/*
** p_view.c
*/
void ClientEndServerFrame (edict_t *ent);

/*
** p_hud.c
*/
void MoveClientToIntermission (edict_t *client);
void G_SetStats (edict_t *ent);
void G_SetSpectatorStats (edict_t *ent);
void G_CheckChaseStats (edict_t *ent);
void ValidateSelectedItem (edict_t *ent);
void DeathmatchScoreboardMessage (edict_t *client, edict_t *killer);

/*
** g_pweapon.c
*/
void PlayerNoise(edict_t *who, vec3_t where, int type);

/*
** m_move.c
*/
qboolean M_CheckBottom (edict_t *ent);
qboolean M_walkmove (edict_t *ent, float yaw, float dist);

/*
** g_phys.c
*/
void G_RunEntity (edict_t *ent);

/*
** g_main.c
*/
void SaveClientData (void);
void FetchClientEntData (edict_t *ent);
void EndDMLevel (qboolean nextmap, qboolean with_intermission);

/*
** g_chase.c
*/
void UpdateChaseCam(edict_t *ent);
void ChaseNext(edict_t *ent);
void ChasePrev(edict_t *ent);
void GetChaseTarget(edict_t *ent);

/*
** g_tdm.c
*/
#define LOWER(c)		((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))

void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0);
void Cmd_Vote_f (edict_t *ent);
void Cmd_Vote_Yes_f (edict_t *ent);
void Cmd_Vote_No_f (edict_t *ent);
void Cmd_Ready_f(edict_t *ent, qboolean ready);
void Cmd_ReadyTeam_f(edict_t *ent);
void Cmd_Menu_f(edict_t *ent);
void Cmd_Vote_Stats_f(edict_t *ent, int index);
void Cmd_Teamname_f(edict_t *ent);
void Cmd_Lockteam_f(edict_t *ent);
void Cmd_Unlockteam_f(edict_t *ent);
void Cmd_Pickplayer_f (edict_t *ent);
void Cmd_Kickplayer_f (edict_t *ent);
void Cmd_Time_f (edict_t *ent);
void Cmd_Accuracy_f (edict_t *ent, qboolean from_function);
void Cmd_Admin_f(edict_t *ent);
void Cmd_AdminList_f(edict_t *ent);
void Cmd_DelAdmin_f(edict_t *ent);
void Cmd_Talk_f(edict_t *ent);
void Cmd_Noadmin_f(edict_t *ent);
void Cmd_Banlist_f(edict_t *ent);
void Cmd_Ban_f(edict_t *ent, unsigned long ip, char *fortime);
void Cmd_Unban_f(edict_t *ent);
void Cmd_Kickban_f(edict_t *ent);
void Cmd_GiveAdmin_f(edict_t *ent);
void Cmd_MapList_f(edict_t *ent);
void Cmd_OldScore_f(edict_t *ent);
void Cmd_Items_f(edict_t *ent);
void Cmd_Id_f (edict_t *ent);
void Cmd_Dmf_f(edict_t *ent);
void LoadAdminList(void);
void WriteAdminList(void);
void Cmd_Smap_f(edict_t *ent);
void Cmd_Break_f(edict_t *ent);
void Cmd_Start_f(edict_t *ent);
void Cmd_Details_f(edict_t *ent);
void Cmd_Kickuser_f(edict_t *ent);
void Cmd_Join_f (edict_t *ent, int team);
char *read_word(char *line, int	position);
char *read_line(FILE *f);
void Reset_Accuracy(edict_t *ent);
void Cmd_Sets_f(edict_t *ent);
void Cmd_Teamskin_f(edict_t *ent, qboolean from_function, char *skin);
qboolean Is_Baned(unsigned long ip);
char *IsValidMap(char *mapname, qboolean mapNum);
void DeleteAdmin(struct admin_s *admin);
void CheckProposes(void);
void CheckMatchState(void);
void LoadBanList(void);
void TDM_SelectSpawnPoint (edict_t *ent, vec3_t origin, vec3_t angles);
void Calc_AvgPing (edict_t *ent);
void ShowPlayerId(edict_t *ent);
void spectate (edict_t *self, qboolean forward);
void ThrowGib2 (edict_t *self, char *gibname, int damage, int type);
void Write_ConfigString2(edict_t *ent);
void SVCmd_AdminList_f (void);
void SVCmd_AddAdmin_f (void);
void SVCmd_DelAdmin_f (void);
void CheckCaptain(edict_t *ent);
void FreeOldScores(void);
void FreeSavedPlayers(void);
void CheckSavedPlayer(edict_t *ent);
char *GetConfigFromList (int conf_num);
void Cmd_ConfigList_f (edict_t *ent);
qboolean CheckSvConfigList (char *search);
void Cmd_Pass_f (edict_t *ent);
void Cmd_Captain_f (edict_t *ent);
void Cmd_VoteLock_f(edict_t *ent, int lock);
void NoAmmoWeaponChange (edict_t *ent);
void CreateOldScores (void);
void Update_ServerInfo(void);
void UpdateConfigStrings(void);
void init_mm(void);
void InitSavedPlayers(void);
void SpawnScoreBoards(void);
qboolean findspawnpoint (edict_t *ent, vec3_t mins, vec3_t maxs, vec3_t origin, vec3_t angles);
void MyCPrintfInit(edict_t *ent);
void AddPlayerToOldScore(edict_t *ent);
void SavePlayerData(edict_t *ent);
void Cmd_Hold_f(edict_t *ent);
void Cmd_Chase_f(edict_t *ent);
void DisplayOldScore(edict_t *ent);
void Log (edict_t *ent, int event, char *optional);
void Cmd_Obsmode_f (edict_t *ent);
void FreeSpawnRanges(void);
void InitSpawnRanges(void);
void ClearDominationRunes(void);
void SetDominationStatusbar(void);
void MatchEndInfo(void);
void Cmd_Config_f (edict_t *ent);
void VoteCheckDisconnect (edict_t *ent);
void MyCPrintf(edict_t *ent, int type, int level, char *text, ...);
void MyCPrintfEnd(edict_t *ent, int type, int level);
qboolean CanVote (edict_t *ent, char *what);

//
// g_ai.c
//
void AI_SetSightClient (void);

void ai_stand (edict_t *self, float dist);
void ai_move (edict_t *self, float dist);
void ai_walk (edict_t *self, float dist);
void ai_turn (edict_t *self, float dist);
void ai_run (edict_t *self, float dist);
void ai_charge (edict_t *self, float dist);
int range (edict_t *self, edict_t *other);

void FoundTarget (edict_t *self);
qboolean infront (edict_t *self, edict_t *other);
qboolean visible (edict_t *self, edict_t *other);
qboolean FacingIdeal(edict_t *self);

/*============================================================================*/

/*client_t->anim_priority*/
#define	ANIM_BASIC		0		/*stand / run*/
#define	ANIM_WAVE		1
#define	ANIM_JUMP		2
#define	ANIM_PAIN		3
#define	ANIM_ATTACK		4
#define	ANIM_DEATH		5
#define	ANIM_REVERSE	6

typedef struct
{
	int		shots;
	int		hits;
	int		dmg;
	int		dmr;
	int		kills;
	int		deaths;
}weapon_stats;

typedef struct
{
	weapon_stats	weaps[11];

	int			quaddamage;
	int			invulnerability;
	int			megahealth;
	int			ammopack;
	int			ja;
	int			ca;
	int			ba;
	int			ps;
	int			adrenaline;

	int			team_damg;
	int			team_damr;

	int			hudlist;

	qboolean	is_admin;
	qboolean	reconnected;
	qboolean	hudid;
	qboolean	new_drops;

	int			team;
	int			admin_flags;
	char		admin_password[16];
	qboolean	judge;
	unsigned long ip;
	int			id_x;
	int			id_y;

	int			highlight;
	int			vote_yon;
	int			vote_change_count;
	qboolean	ready_or_not;
	qboolean	autorecord;
	int			autoscreenshot;//1 - tga, 2 - jpg
} client_save_data_t;

#define HIGHLIGHT_CAPTAIN 0
#define HIGHLIGHT_PLAYER 1
#define HIGHLIGHT_NOBODY 2

/*client data that stays across multiple level loads*/
typedef struct
{
	char		userinfo[MAX_INFO_STRING];
	char		netname[16];
	int			hand;

	qboolean	connected;			/*a loadgame will leave valid entities that*/
									/*just don't have a connection yet*/

	/*values saved and restored from edicts when changing levels*/
	int			health;
	int			max_health;
	int			savedFlags;

	int			selected_item;
	int			inventory[MAX_ITEMS];

	/*ammo capacities*/
	int			max_bullets;
	int			max_shells;
	int			max_rockets;
	int			max_grenades;
	int			max_cells;
	int			max_slugs;

	gitem_t		*weapon;
	gitem_t		*lastweapon;

	int			power_cubes;	/*used for tracking the cubes in coop games*/
	int			score;			/*for calculating total unit score in coop games*/

	int			game_helpchanged;
	int			helpchanged;

	client_save_data_t 	save_data;

	qboolean mvdspec;
} client_persistant_t;

/*client data that stays across deathmatch respawns*/
typedef struct
{
	int			enterframe;			/*level.framenum the client entered the game*/
	int			score;				/*frags, etc*/
	int			net;
	vec3_t		cmd_angles;			/*angles sent over in the last command*/

	qtime		idle;

	int			old_ping;
	float		avgping;
	float		old_avgping;

#ifdef TEXTURES_NAMES
	qboolean	showtexture_name;
#endif
	int			id;

	qboolean	ingame;
	qboolean	silenced;
} client_respawn_t;

typedef struct
{
	int current_menu_item;
	int current_menu;
	char mapname[81];
	char kick[81];
	char *config;
	int mapnum;
	int changed_item;
	int v_timelimit;
	int v_kick;
	int v_dmflags;
	int v_config;
	int v_tp;
	qboolean v_dmflags_active;
	qboolean v_bfg;
	qboolean v_powerups;
	qboolean v_fastweapons;
	qboolean v_hud;
	qboolean multivote;
} menu_struct_t;

/*
** this structure is cleared on each PutClientInServer(),
** except for 'client->pers'
*/

#define	TEAM_A		0
#define	TEAM_B		1
#define TEAM_NONE       2

#define VOTE_NONE	0
#define VOTE_YES	1
#define VOTE_NO		2

#define WARMUP		0
#define PREGAME		1
#define COUNTDOWN	2
#define MATCH		3
#define OVERTIME	4
#define SUDDENDEATH 	5
#define END		6

#define EYE_CAM		0
#define BEHINF_CAM	1
#define FREE_CAM	2

//	known to server
struct gclient_s
{
	player_state_t	ps;				//communicated by server to clients
	int ping;
	int clientNum;

//	private to game
	client_persistant_t	pers;
	client_respawn_t	resp;
	pmove_state_t		old_pmove;	//for detecting out-of-pmove changes

	qboolean	showscores;			//set layout stat
	qboolean	showinventory;		//set layout stat
	qboolean	showhelp;
	qboolean	showhelpicon;

	int			ammo_index;

	int			buttons;
	int			oldbuttons;
	int			latched_buttons;

	qboolean	weapon_thunk;

	gitem_t		*newweapon;


//	sum up damage over an entire frame, so
//	shotgun blasts give a single big kick

	int			damage_armor;		//damage absorbed by armor
	int			damage_parmor;		//damage absorbed by power armor
	int			damage_blood;		//damage taken out of health
	int			damage_knockback;	//impact damage
	vec3_t		damage_from;		//origin for vector calculation

	float		killer_yaw;			//when dead, look at killer

	weaponstate_t	weaponstate;
	vec3_t		kick_angles;	//weapon kicks
	vec3_t		kick_origin;
	float		v_dmg_roll, v_dmg_pitch;
	qtime 		v_dmg_time;	//damage kicks
	qtime		fall_time;
	float 		fall_value;		//for view drop on fall
	float		damage_alpha;
	float		bonus_alpha;
	vec3_t		damage_blend;
	vec3_t		v_angle;			//aiming direction
	float		bobtime;			//so off-ground doesn't change it
	vec3_t		oldviewangles;
	vec3_t		oldvelocity;

	qtime		next_drown_time;
	int			old_waterlevel;
	int			breather_sound;

	int			machinegun_shots;	//for weapon raising

	//animation vars
	int			anim_end;
	int			anim_priority;
	qboolean	anim_duck;
	qboolean	anim_run;

	//powerup timers
	qtime		quad_framenum;
	qtime		invincible_framenum;
	qtime		breather_framenum;
	qtime		enviro_framenum;

	qboolean	grenade_blew_up;
	qtime		grenade_time;
	int			silencer_shots;
	int			weapon_sound;

	qtime		pickup_msg_time;

	qtime		flood_locktill;		//locked from talking
	qtime		flood_when[10];		//when messages were said
	int			flood_whenhead;		//head pointer for when said

	qtime		respawn_time;		//can respawn when time > this

	edict_t		*chase_target;		//player we are chasing
	qboolean	update_chase;		//need to update chase info?

	//menu
	qboolean 	showmenu;
	menu_struct_t	menu;

	//hud and id
	qboolean	showid;

	qboolean	showsets;
	int			chase_mode;
	qboolean	showoldscore;
	char		mycprintf_outbuff[4096];
	char		*longlist_outbuff;
	int			longlist_outbuff_len;

	qboolean	spawn_invincible;
	char		hud_setting_num;
};

struct edict_s
{
	entity_state_t	s;
	struct gclient_s	*client;	/*NULL if not a player*/
									/*the server expects the first part*/
									/*of gclient_s to be a player_state_t*/
									/*but the rest of it is opaque*/

	qboolean	inuse;
	int			linkcount;

	/*FIXME: move these fields to a server private sv_entity_t*/
	link_t		area;				/*linked to a division node or leaf*/
	
	int			num_clusters;		/*if -1, use headnode instead*/
	int			clusternums[MAX_ENT_CLUSTERS];
	int			headnode;			/*unused if num_clusters != -1*/
	int			areanum, areanum2;

	/*================================*/

	int			svflags;
	vec3_t		mins, maxs;
	vec3_t		absmin, absmax, size;
	solid_t		solid;
	int			clipmask;
	edict_t		*owner;

	/*
	** DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	** EXPECTS THE FIELDS IN THAT ORDER!
	*/
	/*================================*/
	int			movetype;
	int			flags;

	char		*model;
	qtime		freetime;			/*sv.time when the object was freed*/
	
	/*
	** only used locally in game, not by server
	*/
	char		*message;
	char		*classname;
	int			spawnflags;

	qtime		timestamp;

	float		angle;			/*set in qe3, -1 = up, -2 = down*/
	char		*target;
	char		*targetname;
	char		*killtarget;
	char		*team;
	char		*pathtarget;
	char		*deathtarget;
	char		*combattarget;
	edict_t		*target_ent;

	float		speed, accel, decel;
	vec3_t		movedir;
	vec3_t		pos1, pos2;

	vec3_t		velocity;
	vec3_t		avelocity;
	int			mass;
	qtime		air_finished;
	float		gravity;		/*per entity gravity multiplier (1.0 is normal)*/
						/*use for lowgrav artifact, flares*/

	edict_t		*goalentity;
	edict_t		*movetarget;
	float		yaw_speed;
	float		ideal_yaw;

	qtime		nextthink;
	void		(*prethink) (edict_t *ent);
	void		(*think)(edict_t *self);
	void		(*blocked)(edict_t *self, edict_t *other);	/*move to moveinfo?*/
	void		(*touch)(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
	void		(*use)(edict_t *self, edict_t *other, edict_t *activator);
	void		(*pain)(edict_t *self, edict_t *other, float kick, int damage);
	void		(*die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

	qtime		touch_debounce_time;		/*are all these legit?  do we need more/less of them?*/
	qtime		pain_debounce_time;
	qtime		damage_debounce_time;
	qtime		fly_sound_debounce_time;	/*move to clientinfo*/
	qtime		last_move_time;

	int			health;
	int			max_health;
	int			gib_health;
	int			deadflag;
	qboolean	show_hostile;

	qtime		powerarmor_time;

	char		*map;			/*target_changelevel*/

	int			viewheight;		/*height above origin where eyesight is determined*/
	int			takedamage;
	int			dmg;
	int			radius_dmg;
	float		dmg_radius;
	int			sounds;			/*make this a spawntemp var?*/
	int			count;

	edict_t		*chain;
	edict_t		*enemy;
	edict_t		*oldenemy;
	edict_t		*activator;
	edict_t		*groundentity;
	int			groundentity_linkcount;
	edict_t		*teamchain;
	edict_t		*teammaster;

	edict_t		*mynoise;		/*can go in client only*/
	edict_t		*mynoise2;

	int			noise_index;
	int			noise_index2;
	float		volume;
	float		attenuation;

	/*timing variables*/
	float		wait;
	float		delay;			/*before firing targets*/
	float		random;

	qtime		teleport_time;

	int			watertype;
	int			waterlevel;

	vec3_t		move_origin;
	vec3_t		move_angles;

	/*move this to clientinfo?*/
	int			light_level;

	int			style;			/*also used as areaportal number*/

	gitem_t		*item;			/*for bonus items*/

	/*common data blocks*/
	moveinfo_t		moveinfo;
	monsterinfo_t	monsterinfo;
	edict_t		*slave1;
	edict_t		*slave2;
	edict_t		*slave3;
	edict_t		*slave4;
	edict_t		*slave5;
};

typedef struct
{
	char	name[16];
	int		ping;
	int		avg_ping;
	int		score;
	int		net;
} team_players;

typedef struct
{
	char	name[16];
	int		ping;
	qboolean chasecam;
	char	chasing[16];
} spectator_player;

typedef struct
{
	char		level_name[MAX_QPATH];
	char		mapname[MAX_QPATH];
	int			team_a_avgping;
	int			team_b_avgping;
	int			team_a_score;
	int			team_b_score;
	char		teamA_name[MAX_QPATH];
	char		teamB_name[MAX_QPATH];
	char		teamA_skin[MAX_QPATH];
	char		teamB_skin[MAX_QPATH];
	team_players *playersA;
	team_players *playersB;
	int			num_playersA;
	int			num_playersB;
	int			sorted_A[64];
	int			sorted_B[64];
	spectator_player *spectators;
	int			num_spectators;
} oldscores_t;

extern  oldscores_t		old_scores;

typedef struct
{
	client_persistant_t	pers;
	client_respawn_t	resp;
	int					ammo_index;
	int					health;
} saved_player_t;

int num_saved_players;
saved_player_t	*saved_players;

#define LOG_CONNECT 0
#define LOG_ADMIN 1
#define LOG_CHANGE 2
#define LOG_VOTE 3

typedef struct
{
	vec3_t	mins;
	vec3_t	maxs;
} spawn_range_t;

int num_spawn_ranges;
spawn_range_t *spawn_ranges;

#define CHASE_INEYES 0
#define CHASE_BEHIND 1
#define CHASE_FOLLOWQD 2
#define CHASE_FREE 3

#define CPRINTF 0
#define CENTERPRINTF 1
#define BPRINTF 2

#define ATTR_C_WHITE			0x00000001
#define ATTR_C_BLACK			0x00000002
#define ATTR_C_DKBLUE			0x00000004
#define ATTR_C_GREEN			0x00000008
#define ATTR_C_RED				0x00000010
#define ATTR_C_MAROON			0x00000020
#define ATTR_C_PURPLE			0x00000040
#define ATTR_C_ORANGE			0x00000080
#define ATTR_C_YELLOW			0x00000100
#define ATTR_C_LTGREEN			0x00000200
#define ATTR_C_TEAL				0x00000400
#define ATTR_C_CYAN				0x00000800
#define ATTR_C_BLUE				0x00001000
#define ATTR_C_FUCHSIA			0x00002000
#define ATTR_C_DKGRAY			0x00004000
#define ATTR_C_LTGRAY			0x00008000
#define ATTR_BOLD				0x00010000
#define ATTR_ITALIC				0x00020000
#define ATTR_UNDERLINE			0x00040000
#define	ATTR_PRINT_LOW			0x00080000
#define	ATTR_PRINT_MEDIUM		0x00100000
#define	ATTR_PRINT_HIGH			0x00200000
#define	ATTR_PRINT_CHAT			0x00400000

#define LINK(link, first, last, next, prev) \
do \
{ \
    if (!(first)) \
      (first) = (link); \
    else \
      (last)->next = (link); \
    (link)->next = NULL;	\
    (link)->prev = (last); \
    (last) = (link); \
} while(0)

#define UNLINK(link, first, last, next, prev) \
do \
{ \
    if (!(link)->prev) \
      (first) = (link)->next; \
    else \
      (link)->prev->next = (link)->next; \
    if (!(link)->next) \
      (last) = (link)->prev; \
    else \
      (link)->next->prev = (link)->prev; \
} while(0) 

//server features
// server is able to read clientNum field from gclient_s struct and hide appropriate entity from client
// game DLL fills clientNum with useful information (current POV index)
#define GMF_CLIENTNUM   1

// game DLL always sets 'inuse' flag properly allowing the server to reject entities quickly
#define GMF_PROPERINUSE 2

// server will set '\mvdspec\<version>' key/value pair in userinfo string if (and only if) client is dummy MVD client (this client represents all MVD spectators and is needed for scoreboard support, etc)
#define GMF_MVDSPEC     4

// inform game DLL of disconnects between level changes
#define GMF_WANT_ALL_DISCONNECTS 8
