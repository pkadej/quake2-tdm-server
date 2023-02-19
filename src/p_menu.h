#define MAIN_MENU 0
#define VOTE_MENU 1
#define CREDITS	2
#define CONTACT 3
#define ADMIN_HELP_1 4
#define ADMIN_HELP_2 5
#define ADMIN_HELP_3 6
#define ADMIN_HELP_4 7
#define ADMIN_HELP_5 8
#define COMMANDS_1	9
#define COMMANDS_2	10
#define COMMANDS_3	11
#define COMMANDS_4	12
#define COMMANDS_5	13
#define COMMANDS_6	14
#define COMMANDS_7	15


#define	MI_ADMIN	0x00000001
#define	MI_OPTIONS	0x00000002
#define	MI_MAPNAME	0x00000004
#define	MI_CENTER	0x00000008
#define	MI_APLAYERS	0x00000010
#define	MI_BPLAYERS	0x00000020
#define	MI_SPLAYERS	0x00000040
#define	MI_GREEN	0x00000080

void proposal_changes (edict_t *self, qboolean forward);
void change_map (edict_t *self, qboolean forward);
void change_timelimit (edict_t *self, qboolean forward);
void change_bfg (edict_t *self, qboolean forward);
void change_powerups (edict_t *self, qboolean forward);
void change_fastweapons (edict_t *self, qboolean forward);
void change_dmflags (edict_t *self, qboolean forward);
void change_kick (edict_t *self, qboolean forward);
void change_config (edict_t *self, qboolean forward);
void change_tp (edict_t *self, qboolean forward);
void change_hud (edict_t *self, qboolean forward);
void SpectateOrChase (edict_t *self, qboolean forward);
void join_team_a (edict_t *self, qboolean forward);
void join_team_b (edict_t *self, qboolean forward);
void menu_off (edict_t *self, qboolean forward);
void menu_display (edict_t *self, int menu);
void menu_next(edict_t *self);
void menu_prev(edict_t *self);
void menu_enter (edict_t *self, qboolean forward);
void menu_change_option (edict_t *self);
void submenu_enter (edict_t *self, qboolean forward);
void submenu_leave (edict_t *self, qboolean forward);

struct menu_items
{
	char	*name;
	int		menu_index;
	int		item_index;
	int		sub_menu_index;
	int		dist_top;
	int		dist_left;
	void	(*execute)(edict_t *self, qboolean forward);
	int		flags;
};

struct dmflags_t
{
	char	*name;
	int		flags;
};

#define NUM_DMFLAGS 17

extern const struct dmflags_t dmflags_table [NUM_DMFLAGS];

struct menu
{
	char	*name;
	int		index;
	int		prev_menu;
	int		num_items;
};
