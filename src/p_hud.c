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

/*
======================================================================

INTERMISSION

======================================================================
*/

void MoveClientToIntermission (edict_t *ent)
{
//	if (level.match_state == END)
//		ent->client->showoldscore = true;
//	else
	if (level.match_state != END)
		ent->client->showscores = true;
	VectorCopy (level.intermission_origin, ent->s.origin);
	ent->client->ps.pmove.origin[0] = level.intermission_origin[0]*8;
	ent->client->ps.pmove.origin[1] = level.intermission_origin[1]*8;
	ent->client->ps.pmove.origin[2] = level.intermission_origin[2]*8;
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunindex = 0;
	ent->client->ps.blend[3] = 0;
	ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	// clean up powerup info
	ent->client->quad_framenum = 0;
	ent->client->invincible_framenum = 0;
	ent->client->breather_framenum = 0;
	ent->client->enviro_framenum = 0;
	ent->client->grenade_blew_up = false;
	ent->client->grenade_time = 0;

	ent->viewheight = 0;
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.modelindex = 0;
	ent->s.effects = 0;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;

	// add the layout

//	if (level.match_state == END)
//		DisplayOldScore(ent);
//	else
	if (level.match_state != END)
	{
		DeathmatchScoreboardMessage (ent, NULL);
		gi.unicast (ent, true);
	}
}

void BeginIntermission (edict_t *targ)
{
	int		i;
	edict_t	*ent, *client;

	if (level.intermissiontime)
		return;		// already activated

	game.autosaved = false;

	// respawn any dead clients
	for (i=0 ; i<maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		if (client->health <= 0)
			respawn(client, false, true);
	}

	level.intermissiontime = level.time;
	level.changemap = targ->map;

	level.exitintermission = 0;

	// find an intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if (!ent)
	{	// the map creator forgot to put in an intermission point...
		ent = G_Find (NULL, FOFS(classname), "info_player_start");
		if (!ent)
			ent = G_Find (NULL, FOFS(classname), "info_player_deathmatch");
	}
	else
	{	// chose one of four spots
		i = rand() & 3;
		while (i--)
		{
			ent = G_Find (ent, FOFS(classname), "info_player_intermission");
			if (!ent)	// wrap around the list
				ent = G_Find (ent, FOFS(classname), "info_player_intermission");
		}
	}

	VectorCopy (ent->s.origin, level.intermission_origin);
	VectorCopy (ent->s.angles, level.intermission_angle);

	// move all clients to the intermission point
	for (i=0 ; i<maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		MoveClientToIntermission (client);
	}
}

/*
==================
DeathmatchScoreboard

Draw instead of help message.
Note that it isn't that hard to overflow the 1400 byte message limit!
==================
*/
void DeathmatchScoreboard (edict_t *ent)
{
	DeathmatchScoreboardMessage (ent, ent->enemy);
	gi.unicast (ent, true);
}


/*
==================
Cmd_Score_f

Display the scoreboard
==================
*/
void Cmd_Score_f (edict_t *ent)
{
	ent->client->showinventory = false;
	ent->client->showhelp = false;
	ent->client->showmenu = false;
	ent->client->showsets = false;
	ent->client->showid = false;

	if (!deathmatch->value)
		return;

	if (ent->client->showoldscore)
	{
		ent->client->showoldscore = false;
		if (ent->client->chase_target)
			ent->client->update_chase = true;
		return;
	}


	if (!ent->client->showscores)
	{
		ent->client->showscores = true;
		DeathmatchScoreboard (ent);
		return;

	}

	if (level.framenum <= 600)
	{
		if (game.is_old_score && (level.match_state == WARMUP || level.match_state == PREGAME) && !ent->client->showoldscore && !ent->client->showoldscore)
		{
			ent->client->showscores = false;
			ent->client->showoldscore = true;
			DisplayOldScore(ent);
			return;
		}
		else
		{
			ent->client->showscores = false;
			if (ent->client->chase_target)
				ent->client->update_chase = true;
			return;
		}
	}
	else
	{
		ent->client->showscores = false;
		if (ent->client->chase_target)
			ent->client->update_chase = true;
		return;
	}
}


/*
==================
HelpComputer

Draw help computer.
==================
*/
void HelpComputer (edict_t *ent)
{
	char	string[1024];
	char	*sk;

	if (skill->value == 0)
		sk = "easy";
	else if (skill->value == 1)
		sk = "medium";
	else if (skill->value == 2)
		sk = "hard";
	else
		sk = "hard+";

	// send the layout
	Com_sprintf (string, sizeof(string),
		"xv 32 yv 8 picn help "			// background
		"xv 202 yv 12 string2 \"%s\" "		// skill
		"xv 0 yv 24 cstring2 \"%s\" "		// level name
		"xv 0 yv 54 cstring2 \"%s\" "		// help 1
		"xv 0 yv 110 cstring2 \"%s\" "		// help 2
		"xv 50 yv 164 string2 \" kills     goals    secrets\" "
		"xv 50 yv 172 string2 \"%3i/%3i     %i/%i       %i/%i\" ", 
		sk,
		level.level_name,
		game.helpmessage1,
		game.helpmessage2,
		level.killed_monsters, level.total_monsters, 
		level.found_goals, level.total_goals,
		level.found_secrets, level.total_secrets);

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
	gi.unicast (ent, true);
}


/*
==================
Cmd_Help_f

Display the current help message
==================
*/
void Cmd_Help_f (edict_t *ent)
{
	// this is for backwards compatability
	if (deathmatch->value)
	{
		Cmd_Score_f (ent);
		return;
	}

	ent->client->showinventory = false;
	ent->client->showscores = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->pers.helpchanged = 0;
	HelpComputer (ent);
}


//=======================================================================

/*
===============
G_SetStats
===============
*/
void G_SetHudList (edict_t *ent)
{
	gitem_t		*weapon1;
	gitem_t		*weapon2;
	edict_t		*targ=NULL;

	targ = ent;
	if (ent->client->pers.save_data.team == TEAM_NONE)
	{
		if (ent->client->chase_target && ent->client->chase_target->inuse)
			targ = ent->client->chase_target;
	}

	if (ent->client->pers.save_data.hudlist)
	{
		ent->client->ps.stats[STAT_SHELLS] = targ->client->pers.inventory[ITEM_INDEX(FindItem ("shells"))];
		if (ent->client->ps.stats[STAT_SHELLS])
			ent->client->ps.stats[STAT_SG] = gi.imageindex("a_shells");
		else
			ent->client->ps.stats[STAT_SG] = 0;
		ent->client->ps.stats[STAT_BULLETS] = targ->client->pers.inventory[ITEM_INDEX(FindItem ("bullets"))];
		if (ent->client->ps.stats[STAT_BULLETS])
			ent->client->ps.stats[STAT_MG] = gi.imageindex("a_bullets");
		else
			ent->client->ps.stats[STAT_MG] = 0;
		ent->client->ps.stats[STAT_GRENADES] = targ->client->pers.inventory[ITEM_INDEX(FindItem ("grenades"))];
		if (ent->client->ps.stats[STAT_GRENADES])
			ent->client->ps.stats[STAT_GL] = gi.imageindex("a_grenades");
		else
			ent->client->ps.stats[STAT_GL] = 0;
		ent->client->ps.stats[STAT_ROCKETS] = targ->client->pers.inventory[ITEM_INDEX(FindItem ("rockets"))];
		if (ent->client->ps.stats[STAT_ROCKETS])
			ent->client->ps.stats[STAT_RL] = gi.imageindex("a_rockets");
		else
			ent->client->ps.stats[STAT_RL] = 0;
		ent->client->ps.stats[STAT_CELLS] = targ->client->pers.inventory[ITEM_INDEX(FindItem ("cells"))];
		if (ent->client->ps.stats[STAT_CELLS])
			ent->client->ps.stats[STAT_HB] = gi.imageindex("a_cells");
		else
			ent->client->ps.stats[STAT_HB] = 0;
		ent->client->ps.stats[STAT_SLUGS] = targ->client->pers.inventory[ITEM_INDEX(FindItem ("slugs"))];
		if (ent->client->ps.stats[STAT_SLUGS])
			ent->client->ps.stats[STAT_RG] = gi.imageindex("a_slugs");
		else
			ent->client->ps.stats[STAT_RG] = 0;

		weapon1 = FindItem ("Shotgun");
		weapon2 = FindItem ("Super Shotgun");

		if (targ->client->pers.inventory[ITEM_INDEX(weapon1)] && targ->client->pers.inventory[ITEM_INDEX(weapon2)])
		{
			if(level.framenum & 8 && ent->client->pers.save_data.hudlist == 1)
				ent->client->ps.stats[STAT_SG] = gi.imageindex(weapon1->icon);
			else
				ent->client->ps.stats[STAT_SG] = gi.imageindex(weapon2->icon);
		}
		else if (targ->client->pers.inventory[ITEM_INDEX(weapon1)])
			ent->client->ps.stats[STAT_SG] = gi.imageindex(weapon1->icon);
		else if (targ->client->pers.inventory[ITEM_INDEX(weapon2)])
			ent->client->ps.stats[STAT_SG] = gi.imageindex(weapon2->icon);

		weapon1 = FindItem ("Machinegun");
		weapon2 = FindItem ("Chaingun");

		if (targ->client->pers.inventory[ITEM_INDEX(weapon1)] && targ->client->pers.inventory[ITEM_INDEX(weapon2)])
		{
			if(level.framenum & 8 && ent->client->pers.save_data.hudlist == 1)
				ent->client->ps.stats[STAT_MG] = gi.imageindex(weapon1->icon);
			else
				ent->client->ps.stats[STAT_MG] = gi.imageindex(weapon2->icon);
		}
		else if (targ->client->pers.inventory[ITEM_INDEX(weapon1)])
			ent->client->ps.stats[STAT_MG] = gi.imageindex(weapon1->icon);
		else if (targ->client->pers.inventory[ITEM_INDEX(weapon2)])
			ent->client->ps.stats[STAT_MG] = gi.imageindex(weapon2->icon);

		weapon1 = FindItem ("Grenade Launcher");

		if (targ->client->pers.inventory[ITEM_INDEX(weapon1)])
			ent->client->ps.stats[STAT_GL] = gi.imageindex(weapon1->icon);

		weapon1 = FindItem ("Rocket Launcher");

		if (targ->client->pers.inventory[ITEM_INDEX(weapon1)])
			ent->client->ps.stats[STAT_RL] = gi.imageindex(weapon1->icon);

		weapon1 = FindItem ("Hyperblaster");
		weapon2 = FindItem ("BFG10K");

		if (targ->client->pers.inventory[ITEM_INDEX(weapon1)] && targ->client->pers.inventory[ITEM_INDEX(weapon2)])
		{
			if(level.framenum & 8 && ent->client->pers.save_data.hudlist == 1)
				ent->client->ps.stats[STAT_HB] = gi.imageindex(weapon1->icon);
			else
				ent->client->ps.stats[STAT_HB] = gi.imageindex(weapon2->icon);
		}
		else if (targ->client->pers.inventory[ITEM_INDEX(weapon1)])
			ent->client->ps.stats[STAT_HB] = gi.imageindex(weapon1->icon);
		else if (targ->client->pers.inventory[ITEM_INDEX(weapon2)])
			ent->client->ps.stats[STAT_HB] = gi.imageindex(weapon2->icon);

		weapon1 = FindItem ("Railgun");

		if (targ->client->pers.inventory[ITEM_INDEX(weapon1)])
			ent->client->ps.stats[STAT_RG] = gi.imageindex(weapon1->icon);
	}
	else
	{
		ent->client->ps.stats[STAT_SHELLS] = 0;
		ent->client->ps.stats[STAT_BULLETS] = 0;
		ent->client->ps.stats[STAT_GRENADES] = 0;
		ent->client->ps.stats[STAT_ROCKETS] = 0;
		ent->client->ps.stats[STAT_CELLS] = 0;
		ent->client->ps.stats[STAT_SLUGS] = 0;
		ent->client->ps.stats[STAT_SG] = 0;
		ent->client->ps.stats[STAT_MG] = 0;
		ent->client->ps.stats[STAT_GL] = 0;
		ent->client->ps.stats[STAT_RL] = 0;
		ent->client->ps.stats[STAT_HB] = 0;
		ent->client->ps.stats[STAT_RG] = 0;
	}
}

void G_SetStats (edict_t *ent)
{
	gitem_t		*item;
	int			index;
	int			cells=0;
	int			power_armor_type;

	//
	// health
	//
	ent->client->ps.stats[STAT_HEALTH] = ent->health;

	//
	// ammo
	//
	if (!ent->client->ammo_index /* || !ent->client->pers.inventory[ent->client->ammo_index] */)
	{
		ent->client->ps.stats[STAT_AMMO_ICON] = 0;
		ent->client->ps.stats[STAT_AMMO] = 0;
	}
	else
	{
		item = &itemlist[ent->client->ammo_index];
		ent->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex (item->icon);
		ent->client->ps.stats[STAT_AMMO] = ent->client->pers.inventory[ent->client->ammo_index];
	}

	//
	// armor
	//
	power_armor_type = PowerArmorType (ent);
	if (power_armor_type)
	{
		cells = ent->client->pers.inventory[ITEM_INDEX(FindItem ("cells"))];
		if (cells == 0)
		{	// ran out of cells for power armor
			ent->flags &= ~FL_POWER_ARMOR;
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
			power_armor_type = 0;;
		}
	}

	index = ArmorIndex (ent);
	if (power_armor_type && (!index || (level.framenum & 8) ) )
	{	// flash between power armor and other armor icon
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex ("i_powershield");
		ent->client->ps.stats[STAT_ARMOR] = cells;
	}
	else if (index)
	{
		item = GetItemByIndex (index);
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex (item->icon);
		ent->client->ps.stats[STAT_ARMOR] = ent->client->pers.inventory[index];
	}
	else
	{
		ent->client->ps.stats[STAT_ARMOR_ICON] = 0;
		ent->client->ps.stats[STAT_ARMOR] = 0;
	}

	//
	// timers
	//
	if (domination->value)
	{
		if (ent->client->quad_framenum > level.framenum)
		{
			ent->client->ps.stats[STAT_POWERUP1_ICON] = gi.imageindex ("p_quad");
			ent->client->ps.stats[STAT_POWERUP1_TIMER] = (ent->client->quad_framenum - level.framenum)/10;
		}
		else if (ent->client->invincible_framenum > level.framenum)
		{
			ent->client->ps.stats[STAT_POWERUP1_ICON] = gi.imageindex ("p_invulnerability");
			ent->client->ps.stats[STAT_POWERUP1_TIMER] = (ent->client->invincible_framenum - level.framenum)/10;
		}
		else if (ent->client->enviro_framenum > level.framenum)
		{
			ent->client->ps.stats[STAT_POWERUP1_ICON] = gi.imageindex ("p_envirosuit");
			ent->client->ps.stats[STAT_POWERUP1_TIMER] = (ent->client->enviro_framenum - level.framenum)/10;
		}
		else if (ent->client->breather_framenum > level.framenum)
		{
			ent->client->ps.stats[STAT_POWERUP1_ICON] = gi.imageindex ("p_rebreather");
			ent->client->ps.stats[STAT_POWERUP1_TIMER] = (ent->client->breather_framenum - level.framenum)/10;
		}
		else
		{
			ent->client->ps.stats[STAT_POWERUP1_ICON] = 0;
			ent->client->ps.stats[STAT_POWERUP1_TIMER] = 0;
		}
	}
	else
	{
		if (ent->client->quad_framenum > level.framenum)
		{
			ent->client->ps.stats[STAT_POWERUP1_ICON] = gi.imageindex ("p_quad");
			ent->client->ps.stats[STAT_POWERUP1_TIMER] = (ent->client->quad_framenum - level.framenum)/10;
		}
		else
		{
			ent->client->ps.stats[STAT_POWERUP1_ICON] = 0;
			ent->client->ps.stats[STAT_POWERUP1_TIMER] = 0;
		}
		if (ent->client->invincible_framenum > level.framenum)
		{
			ent->client->ps.stats[STAT_POWERUP2_ICON] = gi.imageindex ("p_invulnerability");
			ent->client->ps.stats[STAT_POWERUP2_TIMER] = (ent->client->invincible_framenum - level.framenum)/10;
		}
		else
		{
			ent->client->ps.stats[STAT_POWERUP2_ICON] = 0;
			ent->client->ps.stats[STAT_POWERUP2_TIMER] = 0;
		}
	}

	//FRAGS
	if ( ent->client->pers.save_data.team != TEAM_NONE )
		ent->client->ps.stats[STAT_FRAGS] = ent->client->resp.score;
	else
		ent->client->ps.stats[STAT_FRAGS] = 0;

	//
	// selected item
	//
	if (ent->client->pers.selected_item == -1)
		ent->client->ps.stats[STAT_SELECTED_ICON] = 0;
	else
		ent->client->ps.stats[STAT_SELECTED_ICON] = gi.imageindex (itemlist[ent->client->pers.selected_item].icon);

	ent->client->ps.stats[STAT_SELECTED_ITEM] = ent->client->pers.selected_item;

	//
	// layouts
	//
	ent->client->ps.stats[STAT_LAYOUTS] = 0;

	if (ent->client->pers.health <= 0 || level.intermissiontime || ent->client->showscores)
		ent->client->ps.stats[STAT_LAYOUTS] |= 1;
	if (ent->client->showinventory && ent->client->pers.health > 0)
		ent->client->ps.stats[STAT_LAYOUTS] |= 2;
	if (ent->client->showmenu || (ent->client->showid && ent->client->pers.save_data.hudid) || ent->client->showsets || ent->client->showoldscore)
		ent->client->ps.stats[STAT_LAYOUTS] |= 1;

	if (level.vote.vote_active)
		ent->client->ps.stats[STAT_VOTE_STRING] = CS_AIRACCEL-1;
	else
		ent->client->ps.stats[STAT_VOTE_STRING] = 0;

	ent->client->ps.stats[STAT_TIME_COUNT] = CS_AIRACCEL-2;

	//
	// current weapon if not shown
	//

	if (ent->client->pers.save_data.new_drops && ent->client->pers.weapon)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(ent->client->pers.weapon)] == 1)
		{
			if ((level.framenum>>2) & 1)
				ent->client->ps.stats[STAT_HELPICON] = gi.imageindex (ent->client->pers.weapon->icon);
			else
				ent->client->ps.stats[STAT_HELPICON] = 0;
		}
		else
			ent->client->ps.stats[STAT_HELPICON] = gi.imageindex (ent->client->pers.weapon->icon);
	}
	else
	{
		if ( (ent->client->pers.hand == CENTER_HANDED || ent->client->ps.fov > 91) && ent->client->pers.weapon)
			ent->client->ps.stats[STAT_HELPICON] = gi.imageindex (ent->client->pers.weapon->icon);
		else
			ent->client->ps.stats[STAT_HELPICON] = 0;
	}

	ent->client->ps.stats[STAT_SPECTATOR_CHASE] = 0;

	if (domination->value)
	{
		if (ent->client->pers.save_data.team == TEAM_A)
		{
			ent->client->ps.stats[STAT_RED_INTEAM] = gi.imageindex ("i_inteam");
			ent->client->ps.stats[STAT_BLUE_INTEAM] = 0;
		}
		else if (ent->client->pers.save_data.team == TEAM_B)
		{
			ent->client->ps.stats[STAT_BLUE_INTEAM] = gi.imageindex ("i_inteam");
			ent->client->ps.stats[STAT_RED_INTEAM] = 0;
		}
		else
		{
			ent->client->ps.stats[STAT_BLUE_INTEAM] = 0;
			ent->client->ps.stats[STAT_RED_INTEAM] = 0;
		}
		ent->client->ps.stats[STAT_RED_SCORE] = level.teamA_score;
		ent->client->ps.stats[STAT_BLUE_SCORE] = level.teamB_score;

		if (level.rune1_owner == TEAM_A)
			ent->client->ps.stats[STAT_RUNE1_ICON] = gi.imageindex ("i_runered");
		else if (level.rune1_owner == TEAM_B)
			ent->client->ps.stats[STAT_RUNE1_ICON] = gi.imageindex ("i_runeblue");
		else
			ent->client->ps.stats[STAT_RUNE1_ICON] = gi.imageindex ("i_runeneutral");

		if (level.rune2_owner == TEAM_A)
			ent->client->ps.stats[STAT_RUNE2_ICON] = gi.imageindex ("i_runered");
		else if (level.rune2_owner == TEAM_B)
			ent->client->ps.stats[STAT_RUNE2_ICON] = gi.imageindex ("i_runeblue");
		else
			ent->client->ps.stats[STAT_RUNE2_ICON] = gi.imageindex ("i_runeneutral");

		if (level.rune3_owner == TEAM_A)
			ent->client->ps.stats[STAT_RUNE3_ICON] = gi.imageindex ("i_runered");
		else if (level.rune3_owner == TEAM_B)
			ent->client->ps.stats[STAT_RUNE3_ICON] = gi.imageindex ("i_runeblue");
		else
			ent->client->ps.stats[STAT_RUNE3_ICON] = gi.imageindex ("i_runeneutral");
	}
	else
		G_SetHudList (ent);

	ent->client->ps.stats[STAT_TEAMS] = CS_GENERAL + (ent - g_edicts);
}

/*
===============
G_CheckChaseStats
===============
*/
void G_CheckChaseStats (edict_t *ent)
{
	int i;
	gclient_t *cl;

	for (i = 1; i <= maxclients->value; i++)
	{
		cl = g_edicts[i].client;
		if (!g_edicts[i].inuse || cl->chase_target != ent)
			continue;

		memcpy(cl->ps.stats, ent->client->ps.stats, sizeof(cl->ps.stats));
		G_SetSpectatorStats(g_edicts + i);
	}
}

/*
===============
G_SetSpectatorStats
===============
*/
void G_SetSpectatorStats (edict_t *ent)
{
	gclient_t *cl = ent->client;

	if (!cl->chase_target || cl->chase_target->inuse == false)
		G_SetStats (ent);
	else if ( cl->chase_target && ( cl->chase_mode == CHASE_INEYES || cl->chase_mode == CHASE_FOLLOWQD ) )
	{
		cl->ps.gunindex = cl->chase_target->client->ps.gunindex;
		cl->ps.gunframe = cl->chase_target->client->ps.gunframe;
		VectorCopy( cl->chase_target->client->ps.gunangles, cl->ps.gunangles );
		VectorCopy( cl->chase_target->client->ps.kick_angles, cl->ps.kick_angles );

		//score string when chasing player
		ent->client->ps.stats[STAT_TEAMS] = CS_GENERAL + (ent - g_edicts);
	}

	if (!domination->value)
		G_SetHudList (ent);

	// layouts are independant in spectator
	cl->ps.stats[STAT_LAYOUTS] = 0;
	if (cl->pers.health <= 0 || level.intermissiontime || cl->showscores)
		cl->ps.stats[STAT_LAYOUTS] |= 1;
	if (cl->showinventory && cl->pers.health > 0)
		cl->ps.stats[STAT_LAYOUTS] |= 2;
	if (cl->showmenu || (cl->showid && ent->client->pers.save_data.hudid) || cl->showsets || (cl->chase_target && cl->chase_target->inuse) || cl->showoldscore)
		cl->ps.stats[STAT_LAYOUTS] |= 1;
}
