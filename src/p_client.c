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
#include "tdm.h"
#include "m_player.h"
#include "p_menu.h"
#include "tdm_plugins_internal.h"
#ifdef PELLESC
#include <string.h>
char *strdup(const char *s1);
#endif

void SP_misc_teleporter_dest (edict_t *ent);

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/
void SP_info_player_start(edict_t *self)
{
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for deathmatch games
*/
void SP_info_player_deathmatch(edict_t *self)
{
	if (!deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}
	SP_misc_teleporter_dest (self);
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(void)
{
}


//=======================================================================

void player_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (!damage)
		return;
	if (!kick)
		return;
	if (!self->client)
		return;
	if (!other)
		return;
	// player pain is handled at the end of the frame in P_DamageFeedback
}


qboolean IsFemale (edict_t *ent)
{
	char		*info;

	if (!ent->client)
		return false;

	info = Info_ValueForKey (ent->client->pers.userinfo, "gender");
	if (info[0] == 'f' || info[0] == 'F')
		return true;
	return false;
}

qboolean IsNeutral (edict_t *ent)
{
	char		*info;

	if (!ent->client)
		return false;

	info = Info_ValueForKey (ent->client->pers.userinfo, "gender");
	if (info[0] != 'f' && info[0] != 'F' && info[0] != 'm' && info[0] != 'M')
		return true;
	return false;
}

void ClientObituary (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	int			mod;
	char		*message;
	char		*message2;
	char		message3[20];
	qboolean	ff;
	int	i;

	sprintf(message3," - TEAMMATE KILL!");					
	for (i=0; i<strlen(message3); i++)
			message3[i] |= 128;

	if (attacker->client && self->client)
	{
		if (attacker->client->pers.save_data.team == self->client->pers.save_data.team)
			meansOfDeath |= MOD_FRIENDLY_FIRE;
	}

	ff = meansOfDeath & MOD_FRIENDLY_FIRE;
	mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
	message = NULL;
	message2 = "";

	switch (mod)
	{
	case MOD_SUICIDE:
		message = "suicides";
		break;
	case MOD_FALLING:
		message = "cratered";
		break;
	case MOD_CRUSH:
		message = "was squished";
		break;
	case MOD_WATER:
		message = "sank like a rock";
		break;
	case MOD_SLIME:
		message = "melted";
		break;
	case MOD_LAVA:
		message = "does a back flip into the lava";
		break;
	case MOD_EXPLOSIVE:
	case MOD_BARREL:
		message = "blew up";
		break;
	case MOD_EXIT:
		message = "found a way out";
		break;
	case MOD_TARGET_LASER:
		message = "saw the light";
		break;
	case MOD_TARGET_BLASTER:
		message = "got blasted";
		break;
	case MOD_BOMB:
	case MOD_SPLASH:
	case MOD_TRIGGER_HURT:
		message = "was in the wrong place";
		break;
	}
	if (attacker == self)
	{
		switch (mod)
		{
		case MOD_HELD_GRENADE:
			message = "tried to put the pin back in";
			break;
		case MOD_HG_SPLASH:
		case MOD_G_SPLASH:
			if (IsNeutral(self))
				message = "tripped on its own grenade";
			else if (IsFemale(self))
				message = "tripped on her own grenade";
			else
				message = "tripped on his own grenade";
			break;
		case MOD_R_SPLASH:
			if (IsNeutral(self))
				message = "blew itself up";
			else if (IsFemale(self))
				message = "blew herself up";
			else
				message = "blew himself up";
			break;
		case MOD_BFG_BLAST:
			message = "should have used a smaller gun";
			break;
		default:
			if (IsNeutral(self))
				message = "killed itself";
			else if (IsFemale(self))
				message = "killed herself";
			else
				message = "killed himself";
			break;
		}
	}
	if (message)
	{
		if (self->client->pers.save_data.team == TEAM_A)
			gi.bprintf (ATTR_PRINT_MEDIUM|ATTR_UNDERLINE|ATTR_C_RED, "%s %s.\n", self->client->pers.netname, message);
		else
			gi.bprintf (ATTR_PRINT_MEDIUM|ATTR_UNDERLINE|ATTR_C_BLUE, "%s %s.\n", self->client->pers.netname, message);
		if (!domination->value && (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH))
		{
			self->client->resp.score--;
			if (self->client->pers.save_data.team == TEAM_B)
				level.teamB_score--;
			else if (self->client->pers.save_data.team == TEAM_A)
				level.teamA_score--;
			level.update_score = true;
		}
		self->enemy = NULL;
		return;
	}

	self->enemy = attacker;
	if (attacker && attacker->client)
	{
		switch (mod)
		{
		case MOD_BLASTER:
			message = "was blasted by";
			break;
		case MOD_SHOTGUN:
			message = "was gunned down by";
			break;
		case MOD_SSHOTGUN:
			message = "was blown away by";
			message2 = "'s super shotgun";
			break;
		case MOD_MACHINEGUN:
			message = "was machinegunned by";
			break;
		case MOD_CHAINGUN:
			message = "was cut in half by";
			message2 = "'s chaingun";
			break;
		case MOD_GRENADE:
			message = "was popped by";
			message2 = "'s grenade";
			break;
		case MOD_G_SPLASH:
			message = "was shredded by";
			message2 = "'s shrapnel";
			break;
		case MOD_ROCKET:
			message = "ate";
			message2 = "'s rocket";
			break;
		case MOD_R_SPLASH:
			message = "almost dodged";
			message2 = "'s rocket";
			break;
		case MOD_HYPERBLASTER:
			message = "was melted by";
			message2 = "'s hyperblaster";
			break;
		case MOD_RAILGUN:
			message = "was railed by";
			break;
		case MOD_BFG_LASER:
			message = "saw the pretty lights from";
			message2 = "'s BFG";
			break;
		case MOD_BFG_BLAST:
			message = "was disintegrated by";
			message2 = "'s BFG blast";
			break;
		case MOD_BFG_EFFECT:
			message = "couldn't hide from";
			message2 = "'s BFG";
			break;
		case MOD_HANDGRENADE:
			message = "caught";
			message2 = "'s handgrenade";
			break;
		case MOD_HG_SPLASH:
			message = "didn't see";
			message2 = "'s handgrenade";
			break;
		case MOD_HELD_GRENADE:
			message = "feels";
			message2 = "'s pain";
			break;
		case MOD_TELEFRAG:
			message = "tried to invade";
			message2 = "'s personal space";
			break;
		}
		if (message)
		{
			if (ff)
			{	
				if (self->client->pers.save_data.team == TEAM_A)
					gi.bprintf (ATTR_PRINT_MEDIUM|ATTR_UNDERLINE|ATTR_C_RED,"%s %s %s%s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2, message3);
				else
					gi.bprintf (ATTR_PRINT_MEDIUM|ATTR_UNDERLINE|ATTR_C_BLUE,"%s %s %s%s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2, message3);
			}
			else
			{
				if (self->client->pers.save_data.team == TEAM_A)
					gi.bprintf (ATTR_PRINT_MEDIUM|ATTR_BOLD|ATTR_C_RED,"%s %s %s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2);
				else
					gi.bprintf (ATTR_PRINT_MEDIUM|ATTR_BOLD|ATTR_C_BLUE,"%s %s %s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2);
			}
			if (!domination->value && (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH))
			{
				if (ff)
				{
					attacker->client->resp.score--;
					if (attacker->client->pers.save_data.team == TEAM_B)
						level.teamB_score--;
					else if (attacker->client->pers.save_data.team == TEAM_A)
						level.teamA_score--;
				}
				else
				{
					attacker->client->resp.score++;
					if (attacker->client->pers.save_data.team == TEAM_B)
						level.teamB_score++;
					else if (attacker->client->pers.save_data.team == TEAM_A)
						level.teamA_score++;
				}
				level.update_score = true;
			}
			return;
		}
	}

	if (self->client->pers.save_data.team == TEAM_A)
		gi.bprintf (ATTR_PRINT_MEDIUM|ATTR_UNDERLINE|ATTR_C_RED,"%s died.\n", self->client->pers.netname);
	else
		gi.bprintf (ATTR_PRINT_MEDIUM|ATTR_UNDERLINE|ATTR_C_BLUE,"%s died.\n", self->client->pers.netname);
	if (!domination->value && (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH))
	{
		self->client->resp.score--;
		if (self->client->pers.save_data.team == TEAM_B)
			level.teamB_score--;
		else if (self->client->pers.save_data.team == TEAM_A)
			level.teamA_score--;
		level.update_score = true;
	}
}


void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

void TossClientWeapon (edict_t *self)
{
	gitem_t		*item;
	edict_t		*drop;
	qboolean	quad;
	float		spread;

	if (!deathmatch->value)
		return;

	if (instagib->value)
		return;

	if (level.match_state != MATCH && level.match_state != OVERTIME && level.match_state != SUDDENDEATH)
		return;

	item = self->client->pers.weapon;
	if (! self->client->pers.inventory[self->client->ammo_index] )
		item = NULL;
	if (item && (strcmp (item->pickup_name, "Blaster") == 0))
		item = NULL;

	if (!((int)(dmflags->value) & DF_QUAD_DROP))
		quad = false;
	else
		quad = (self->client->quad_framenum > (level.framenum + GAMESECONDS(1)));

	if (item && quad)
		spread = 22.5;
	else
		spread = 0.0;

	if (item)
	{
		self->client->v_angle[YAW] -= spread;
		drop = Drop_Item (self, item);
		self->client->v_angle[YAW] += spread;
		drop->spawnflags = DROPPED_PLAYER_ITEM;
	}

	if (quad)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item (self, FindItemByClassname ("item_quad"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->quad_framenum - level.framenum);// * FRAMETIME;
		drop->think = G_FreeEdict;
	}
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	vec3_t		dir;

	if (attacker && attacker != world && attacker != self)
	{
		VectorSubtract (attacker->s.origin, self->s.origin, dir);
	}
	else if (inflictor && inflictor != world && inflictor != self)
	{
		VectorSubtract (inflictor->s.origin, self->s.origin, dir);
	}
	else
	{
		self->client->killer_yaw = self->s.angles[YAW];
		return;
	}

	if (dir[0])
		self->client->killer_yaw = 180/M_PI*atan2(dir[1], dir[0]);
	else {
		self->client->killer_yaw = 0;
		if (dir[1] > 0)
			self->client->killer_yaw = 90;
		else if (dir[1] < 0)
			self->client->killer_yaw = -90;
	}
	if (self->client->killer_yaw < 0)
		self->client->killer_yaw += 360;
	

}

/*
==================
player_die
==================
*/
void player_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	VectorClear (self->avelocity);

	self->takedamage = DAMAGE_YES;
	self->movetype = MOVETYPE_TOSS;

	self->s.modelindex2 = 0;	// remove linked weapon model

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;

	self->s.sound = 0;
	self->client->weapon_sound = 0;

	self->maxs[2] = -8;

//	self->solid = SOLID_NOT;
	self->svflags |= SVF_DEADMONSTER;

	if (!self->deadflag)
	{
		self->client->respawn_time = level.time + GAMESECONDS(5);
		LookAtKiller (self, inflictor, attacker);
		self->client->ps.pmove.pm_type = PM_DEAD;
		if (attacker->client)
		{
			if (attacker->client->pers.save_data.team != self->client->pers.save_data.team && (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH))
				self->client->resp.net++;
		}
		else if ((level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH))
			self->client->resp.net++;
		ClientObituary (self, inflictor, attacker);
		TossClientWeapon (self);
		if (deathmatch->value)
			Cmd_Help_f (self);		// show scores

		// clear inventory
		// this is kind of ugly, but it's how we want to handle keys in coop
		for (n = 0; n < game.num_items; n++)
			self->client->pers.inventory[n] = 0;
	}

	// remove powerups
	self->client->quad_framenum = 0;
	self->client->invincible_framenum = 0;
	self->client->breather_framenum = 0;
	self->client->enviro_framenum = 0;
	self->flags &= ~FL_POWER_ARMOR;

	if (self->health < -40)
	{	// gib
		gi.sound (self, CHAN_BODY, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		if (allow_gibs->value >= 1)
		{
			for (n= 0; n < 4; n++)
				ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
			if (allow_gibs->value == 2)
			{
				ThrowGib2 (self, "models/objects/gibs/player/chest.md2", damage, GIB_ORGANIC);
				ThrowGib2 (self, "models/objects/gibs/player/arm.md2", damage, GIB_ORGANIC);
				ThrowGib2 (self, "models/objects/gibs/player/arm.md2", damage, GIB_ORGANIC);
				ThrowGib2 (self, "models/objects/gibs/player/leg.md2", damage, GIB_ORGANIC);
				ThrowGib2 (self, "models/objects/gibs/player/feet.md2", damage, GIB_ORGANIC);
				for (n = 0; n < 2; n++)
					ThrowGib2 (self, "models/objects/gibs/player/intestine.md2", damage, GIB_ORGANIC);
				ThrowGib2 (self, "models/objects/gibs/player/brain.md2", damage, GIB_ORGANIC);
				ThrowGib2 (self, "models/objects/gibs/player/bone.md2", damage, GIB_ORGANIC);
			}
		}
		ThrowClientHead (self, damage);

		self->takedamage = DAMAGE_NO;
	}
	else
	{	// normal death
		if (!self->deadflag)
		{
			static int i;

			i = (i+1)%3;
			// start a death animation
			self->client->anim_priority = ANIM_DEATH;
			if (self->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				self->s.frame = FRAME_crdeath1-1;
				self->client->anim_end = FRAME_crdeath5;
			}
			else switch (i)
			{
			case 0:
				self->s.frame = FRAME_death101-1;
				self->client->anim_end = FRAME_death106;
				break;
			case 1:
				self->s.frame = FRAME_death201-1;
				self->client->anim_end = FRAME_death206;
				break;
			case 2:
				self->s.frame = FRAME_death301-1;
				self->client->anim_end = FRAME_death308;
				break;
			}
			gi.sound (self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (rand()%4)+1)), 1, ATTN_NORM, 0);
		}
	}

//	self->wait = level.time + GAMESECONDS(7);
	self->deadflag = DEAD_DEAD;

	gi.linkentity (self);
}

//=======================================================================

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
void InitClientPersistant (gclient_t *client)
{
	gitem_t				*item;
	client_save_data_t	save_data;

	save_data = client->pers.save_data;
	memset (&client->pers, 0, sizeof(client->pers));
	client->pers.save_data = save_data;

	item = FindItem("Blaster");
	client->pers.selected_item = ITEM_INDEX(item);
	client->pers.inventory[client->pers.selected_item] = 1;

	client->pers.health			= 100;
	client->pers.max_health		= 100;

	client->pers.max_bullets	= 200;
	client->pers.max_shells		= 100;
	client->pers.max_rockets	= 50;
	client->pers.max_grenades	= 50;
	client->pers.max_cells		= 200;
	client->pers.max_slugs		= 50;

	client->pers.connected = true;
	client->pers.mvdspec = false;

	//TDM++
	if (level.match_state == WARMUP && client->pers.save_data.team != TEAM_NONE && !instagib->value)
	{
		item = FindItem("Combat Armor");
		client->pers.inventory[ITEM_INDEX(item)] = 100;

		if (level.weapons & IS_SHOTGUN || level.weapons & IS_SSHOTGUN)
		{
			item = FindItem ("Shells");
			client->pers.inventory[ITEM_INDEX(item)] = client->pers.max_shells;
		}
		if (level.weapons & IS_MACHINEGUN || level.weapons & IS_CHAINGUN)
		{
			item = FindItem ("Bullets");
			client->pers.inventory[ITEM_INDEX(item)] = client->pers.max_bullets;
		}
		if (level.weapons & IS_HBLASTER || level.weapons & IS_BFG)
		{
			item = FindItem ("Cells");
			client->pers.inventory[ITEM_INDEX(item)] = client->pers.max_cells;
		}
		if (level.weapons & IS_RLAUNCHER)
		{
			item = FindItem ("Rockets");
			client->pers.inventory[ITEM_INDEX(item)] = client->pers.max_rockets;
		}
		if (level.weapons & IS_RAILGUN)
		{
			item = FindItem ("Slugs");
			client->pers.inventory[ITEM_INDEX(item)] = client->pers.max_slugs;
		}
		if (level.weapons & IS_GRENADE || level.weapons & IS_GLAUNCHER)
		{
			item = FindItem ("Grenades");
			client->pers.inventory[ITEM_INDEX(item)] = client->pers.max_grenades;
		}

		if (level.weapons & IS_SHOTGUN)
		{
			item = FindItem ("Shotgun");
			client->pers.inventory[ITEM_INDEX(item)] = 1;
		}
		if (level.weapons & IS_SSHOTGUN)
		{
			item = FindItem ("Super Shotgun");
			client->pers.inventory[ITEM_INDEX(item)] = 1;
		}
		if (level.weapons & IS_MACHINEGUN)
		{
			item = FindItem ("Machinegun");
			client->pers.inventory[ITEM_INDEX(item)] = 1;
		}
		if (level.weapons & IS_CHAINGUN)
		{
			item = FindItem ("Chaingun");
			client->pers.inventory[ITEM_INDEX(item)] = 1;
		}
		if (level.weapons & IS_GLAUNCHER)
		{
			item = FindItem ("Grenade Launcher");
			client->pers.inventory[ITEM_INDEX(item)] = 1;
		}
		if (level.weapons & IS_HBLASTER)
		{
			item = FindItem ("HyperBlaster");
			client->pers.inventory[ITEM_INDEX(item)] = 1;
		}
		if (level.weapons & IS_BFG)
		{
			item = FindItem ("BFG10K");
			client->pers.inventory[ITEM_INDEX(item)] = 1;
		}
		if (level.weapons & IS_RLAUNCHER)
		{
			item = FindItem ("Rocket Launcher");
			client->pers.inventory[ITEM_INDEX(item)] = 1;
		}
		if (level.weapons & IS_RAILGUN)
		{
			item = FindItem ("Railgun");
			client->pers.inventory[ITEM_INDEX(item)] = 1;
		}
	}
	else if ((level.match_state == WARMUP || level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH) && instagib->value && client->pers.save_data.team != TEAM_NONE)
	{
		if (level.match_state == WARMUP)
		{
		    if (!((int)dmflags->value & DF_NO_ARMOR))
		    {
			item = FindItem("Combat Armor");
			client->pers.inventory[ITEM_INDEX(item)] = 100;
		    }
		}
		item = FindItem ("Slugs");
		client->pers.inventory[ITEM_INDEX(item)] = client->pers.max_slugs;
		item = FindItem ("Railgun");
		client->pers.inventory[ITEM_INDEX(item)] = 1;
	}
	//TDM++

	client->pers.weapon = item;
}


void InitClientResp (gclient_t *client)
{
	memset (&client->resp, 0, sizeof(client->resp));
	client->resp.enterframe = level.framenum;
	client->resp.avgping = client->ping;
}

/*
==================
SaveClientData

Some information that should be persistant, like health, 
is still stored in the edict structure, so it needs to
be mirrored out to the client structure before all the
edicts are wiped.
==================
*/
void SaveClientData (void)
{
	int		i;
	edict_t	*ent;

	for (i=0 ; i<game.maxclients ; i++)
	{
		ent = &g_edicts[1+i];
		if (!ent->inuse)
			continue;
		game.clients[i].pers.health = ent->health;
		game.clients[i].pers.max_health = ent->max_health;
		game.clients[i].pers.savedFlags = (ent->flags & (FL_GODMODE|FL_NOTARGET|FL_POWER_ARMOR));
	}
}

void FetchClientEntData (edict_t *ent)
{
	ent->health = ent->client->pers.health;
	ent->max_health = ent->client->pers.max_health;
	ent->flags |= ent->client->pers.savedFlags;
}

//======================================================================


void InitBodyQue (void)
{
	int		i;
	edict_t	*ent;

	level.body_que = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++)
	{
		ent = G_Spawn();
		ent->classname = "bodyque";
	}
}

void body_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int	n;

	if (self->health < -40)
	{
		gi.sound (self, CHAN_BODY, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		if (allow_gibs->value >= 1)
		{
			for (n= 0; n < 4; n++)
				ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
			if (allow_gibs->value == 2)
			{
				ThrowGib2 (self, "models/objects/gibs/player/chest.md2", damage, GIB_ORGANIC);
				ThrowGib2 (self, "models/objects/gibs/player/arm.md2", damage, GIB_ORGANIC);
				ThrowGib2 (self, "models/objects/gibs/player/arm.md2", damage, GIB_ORGANIC);
				ThrowGib2 (self, "models/objects/gibs/player/leg.md2", damage, GIB_ORGANIC);
				ThrowGib2 (self, "models/objects/gibs/player/feet.md2", damage, GIB_ORGANIC);
				for (n = 0; n < 2; n++)
					ThrowGib2 (self, "models/objects/gibs/player/intestine.md2", damage, GIB_ORGANIC);
				ThrowGib2 (self, "models/objects/gibs/player/brain.md2", damage, GIB_ORGANIC);
				ThrowGib2 (self, "models/objects/gibs/player/bone.md2", damage, GIB_ORGANIC);
			}
		}
		self->s.origin[2] -= 48;
		ThrowClientHead (self, damage);
		self->takedamage = DAMAGE_NO;
	}
}

//TDM++
void BodyQueAnim (edict_t *body)
{
	if ((body->s.frame >= FRAME_crdeath1 && body->s.frame < FRAME_crdeath5) ||
		(body->s.frame >= FRAME_death101 && body->s.frame < FRAME_death106) ||
		(body->s.frame >= FRAME_death201 && body->s.frame < FRAME_death206) ||
		(body->s.frame >= FRAME_death301 && body->s.frame < FRAME_death308))
	{
		body->s.frame++;
		body->think = BodyQueAnim;
		body->nextthink = level.time + GAMESECONDS(FRAMETIME);
	}
	else
	{
		body->think = NULL;
		body->nextthink = 0;
	}

}
//TDM--

void CopyToBodyQue (edict_t *ent)
{
	edict_t		*body;

	// grab a body que and cycle to the next one
	body = &g_edicts[(int)maxclients->value + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	// FIXME: send an effect on the removed body

	gi.unlinkentity (ent);

	gi.unlinkentity (body);
	body->s = ent->s;
	
	//TDM++
	if ((body->s.frame >= FRAME_crdeath1 && body->s.frame < FRAME_crdeath5) ||
		(body->s.frame >= FRAME_death101 && body->s.frame < FRAME_death106) ||
		(body->s.frame >= FRAME_death201 && body->s.frame < FRAME_death206) ||
		(body->s.frame >= FRAME_death301 && body->s.frame < FRAME_death308))
	{
		body->think = BodyQueAnim;
		body->nextthink = level.time + GAMESECONDS(FRAMETIME);
	}
	//TDM--

	body->s.number = body - g_edicts;

	body->svflags = ent->svflags;
	VectorCopy (ent->mins, body->mins);
	VectorCopy (ent->maxs, body->maxs);
	VectorCopy (ent->absmin, body->absmin);
	VectorCopy (ent->absmax, body->absmax);
	VectorCopy (ent->size, body->size);
	body->solid = ent->solid;
	body->clipmask = ent->clipmask;
	body->owner = ent->owner;
	body->movetype = ent->movetype;

	body->die = body_die;
	body->takedamage = DAMAGE_YES;

	gi.linkentity (body);
}


void respawn (edict_t *self, qboolean was_dead, qboolean telefrag)
{
	if (was_dead)
		CopyToBodyQue (self);
	self->svflags &= ~SVF_NOCLIENT;
	PutClientInServer( self, telefrag );

	// add a teleportation effect
	self->s.event = EV_PLAYER_TELEPORT;

	// hold in place briefly
	self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->client->ps.pmove.pm_time = 14;

	self->client->respawn_time = level.time;
}

/* 
 * only called when pers.spectator changes
 * note that resp.spectator should be the opposite of pers.spectator here
 */
void spectator_respawn (edict_t *ent, qboolean autom)
{
	// clear client on respawn
	ent->client->pers.save_data.team = TEAM_NONE;
	ent->client->resp.score = ent->client->pers.score = 0;

	PutClientInServer( ent, true );

	if (autom)
		gi.bprintf (ATTR_PRINT_HIGH|ATTR_BOLD, "%s has been moved to the sidelines.\n", ent->client->pers.netname);
	else
		gi.bprintf (ATTR_PRINT_HIGH|ATTR_BOLD, "%s has moved to the sidelines.\n", ent->client->pers.netname);
}

//==============================================================

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/
float PlayersRangeFromSpot( edict_t *ent, edict_t *spot )
{
	edict_t	*player;
	float	bestplayerdistance;
	vec3_t	v;
	int		n;
	float	playerdistance;


	bestplayerdistance = 99999;

	for (n = 1; n <= maxclients->value; n++)
	{
		player = &g_edicts[n];

		if (!player->inuse)
			continue;

		if ( player->client->pers.save_data.team == TEAM_NONE || ent->client->pers.save_data.team == TEAM_NONE )
			continue;
			
		if ( player->client->pers.save_data.team == ent->client->pers.save_data.team )
			continue;

		if (player->movetype == MOVETYPE_NOCLIP)
			continue;

		if (player->health <= 0)
			continue;

		if ( player == ent )
			continue;

		VectorSubtract (spot->s.origin, player->s.origin, v);
		playerdistance = VectorLength (v);

		if (playerdistance < bestplayerdistance)
			bestplayerdistance = playerdistance;
	}

	return bestplayerdistance;
}

/*
================
SelectRandomTeamDeathmatchSpawnPoint

================
*/
edict_t *SelectRandomTeamDeathmatchSpawnPoint( edict_t *ent )
{
	int selected = 0, i;
	trace_t tr;
	vec3_t mins = { -16.0f, -16.0f, -1.0f };
	vec3_t maxs = { 16.0f, 16.0f, 1.0f };

	for( i=0; i<level.numSpawnPoints-1; i++ ) //numSpawnPoints-1 tries
	{
		vec3_t start, end;

		selected = ranfr( 1, level.numSpawnPoints-1 );

		VectorCopy( level.spawnPoints[selected].ent->s.origin, start );
		start[2] += 9;
		VectorCopy( start, end );

		tr = gi.trace( start, mins, maxs, end, level.spawnPoints[selected].ent, MASK_SHOT );
		if ( tr.fraction < 1.0 || tr.startsolid || tr.allsolid ) //possible telefrag
			continue;
		else
			break;
	}

	return level.spawnPoints[selected].ent;
}

/*
================
SelectRandomTeamDeathmatchSpawnPoint

go to random spawn point but with higher probabiliy to the farther one from other team players
uses roulette wheel method
================
*/
edict_t *SelectRandomTeamDeathmatchSpawnPoint2( edict_t *ent )
{
	int	count = 0, i;
	unsigned int lot, lot_sum = 0, range_sum = 0;

	for ( i=1; i<level.numSpawnPoints; i++ ) // level.spawnPoints[0] is info_player_start
	{
		count++;
		level.spawnPoints[i].range_from_players = (unsigned int)PlayersRangeFromSpot( ent, level.spawnPoints[i].ent );
		range_sum += level.spawnPoints[i].range_from_players;
	}

	if ( !count )
		return NULL;

	lot = ranfr( 0, range_sum );

	i=1;
	do
	{
		lot_sum += level.spawnPoints[i].range_from_players;
		i++;
	} while( lot_sum < lot && i<level.numSpawnPoints );

	return level.spawnPoints[i-1].ent;
}

/*
================
SelectFarthestDeathmatchSpawnPoint

================
*/
edict_t *SelectFarthestTeamDeathmatchSpawnPoint( edict_t *ent )
{
	edict_t	*bestspot;
	float	bestdistance, bestplayerdistance;
	int i;


	bestspot = NULL;
	bestdistance = 0;
	for ( i=1; i<level.numSpawnPoints; i++ )
	{
		bestplayerdistance = PlayersRangeFromSpot( ent, level.spawnPoints[i].ent );

		if ( bestplayerdistance > bestdistance )
		{
			bestspot = level.spawnPoints[i].ent;
			bestdistance = bestplayerdistance;
		}
	}

	if ( bestspot )
		return bestspot;

	// if there is a player just spawned on each and every start spot
	// we have no choice to turn one into a telefrag meltdown
	bestspot = level.spawnPoints[0].ent;

	return bestspot;
}

void SelectSpawnPoint( edict_t *ent, vec3_t origin, vec3_t angles )
{
	edict_t	*spot = NULL;

	if ( (int)(dmflags->value) & DF_SPAWN_NEWTDM )
		spot = SelectRandomTeamDeathmatchSpawnPoint2( ent );
	else if ( (int)(dmflags->value) & DF_SPAWN_FARTHEST)
		spot = SelectFarthestTeamDeathmatchSpawnPoint( ent );
	else
		spot = SelectRandomTeamDeathmatchSpawnPoint( ent );

	// find a single player start spot
	if (!spot)
	{
		while ((spot = G_Find (spot, FOFS(classname), "info_player_start")) != NULL)
		{
			if (!game.spawnpoint[0] && !spot->targetname)
				break;

			if (!game.spawnpoint[0] || !spot->targetname)
				continue;

			if (Q_stricmp(game.spawnpoint, spot->targetname) == 0)
				break;
		}

		if (!spot)
		{
			if (!game.spawnpoint[0])
			{	// there wasn't a spawnpoint without a target, so use any
				spot = G_Find (spot, FOFS(classname), "info_player_start");
			}
			if (!spot)
				gi.error ("Couldn't find spawn point %s\n", game.spawnpoint);
		}
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);
}


/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/

void PutClientInServer (edict_t *ent, qboolean telefrag)
{
	vec3_t	mins = {-16, -16, -24};
	vec3_t	maxs = {16, 16, 32};
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	client_persistant_t	saved;
	client_respawn_t	resp;
	char		userinfo[MAX_INFO_STRING];
	qboolean mvd;

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if (sv_spawnrandom->value)
	{
		qboolean	found;

		if (ent->client->pers.save_data.team == TEAM_NONE)
			found = findspawnpoint(ent, vec3_origin, vec3_origin, spawn_origin, spawn_angles);
		else
			found = findspawnpoint(ent, mins, maxs, spawn_origin, spawn_angles);
		if (!found)
		{
			if (telefrag == false)
				TDM_SelectSpawnPoint (ent, spawn_origin, spawn_angles);
			else
				SelectSpawnPoint (ent, spawn_origin, spawn_angles);
		}
	}
	else
	{
		if (telefrag == false)
			TDM_SelectSpawnPoint (ent, spawn_origin, spawn_angles);
		else
			SelectSpawnPoint (ent, spawn_origin, spawn_angles);
	}
	index = ent-g_edicts-1;
	client = ent->client;

	// deathmatch wipes most client data every spawn
	resp = client->resp;
	memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
	mvd = client->pers.mvdspec;
	InitClientPersistant (client);
	ClientUserinfoChanged (ent, userinfo);

	// clear everything but the persistant data
	saved = client->pers;
	memset (client, 0, sizeof(*client));
	client->pers = saved;
	client->pers.mvdspec = mvd;
	if (client->pers.health <= 0)
		InitClientPersistant(client);
	client->resp = resp;

	client->clientNum = ent - g_edicts - 1;

	// copy some data from the client to the entity
	FetchClientEntData (ent);

	// clear entity values
	ent->groundentity = NULL;
	ent->client = &game.clients[index];
	ent->takedamage = DAMAGE_AIM;
	ent->movetype = MOVETYPE_WALK;
	ent->viewheight = 22;
	ent->inuse = true;
	ent->classname = "player";
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->deadflag = DEAD_NO;
	ent->air_finished = level.time + GAMESECONDS(12);
	ent->clipmask = MASK_PLAYERSOLID;
	ent->model = "players/male/tris.md2";
	ent->pain = player_pain;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags &= ~FL_NO_KNOCKBACK;
	ent->svflags &= ~SVF_DEADMONSTER;

	VectorCopy (mins, ent->mins);
	VectorCopy (maxs, ent->maxs);
	VectorClear (ent->velocity);

	// clear playerstate values
	memset (&ent->client->ps, 0, sizeof(client->ps));

	client->ps.pmove.origin[0] = spawn_origin[0]*8;
	client->ps.pmove.origin[1] = spawn_origin[1]*8;
	client->ps.pmove.origin[2] = spawn_origin[2]*8;

	if ((int)dmflags->value & DF_FIXED_FOV)
	{
		client->ps.fov = 90;
	}
	else
	{
		client->ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));
		if (client->ps.fov < 1)
			client->ps.fov = 90;
		else if (client->ps.fov > 160)
			client->ps.fov = 160;
	}

	client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);

	// clear entity state values
	ent->s.effects = 0;
	ent->s.modelindex = 255;		// will use the skin specified model
	ent->s.modelindex2 = 255;		// custom gun model
	// sknum is player num and weapon number
	// weapon number will be added in changeweapon
	ent->s.skinnum = ent - g_edicts - 1;

	ent->s.frame = 0;
	VectorCopy (spawn_origin, ent->s.origin);
	ent->s.origin[2] += 1;	// make sure off ground
	VectorCopy (ent->s.origin, ent->s.old_origin);

	// set the delta angle
	for (i=0 ; i<3 ; i++)
	{
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);
	}

	ent->s.angles[PITCH] = 0;
	ent->s.angles[YAW] = spawn_angles[YAW];
	ent->s.angles[ROLL] = 0;
	VectorCopy (ent->s.angles, client->ps.viewangles);
	VectorCopy (ent->s.angles, client->v_angle);

	// spawn a spectator
	if (client->pers.save_data.team == TEAM_NONE)
	{
		client->chase_target = NULL;

		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;

		Write_ConfigString2(ent);

		gi.linkentity (ent);
		return;
	}
	else
	{
		if (sv_spawn_invincible->value)
		{
			ent->client->spawn_invincible = true;
			ent->client->invincible_framenum = level.framenum + (int)sv_spawn_invincible->value;
		}
	}

	if (telefrag)
	{
		if (!KillBox (ent))
		{	// could't spawn in?
		}
	}

	gi.linkentity (ent);

	// force the current weapon up
	client->newweapon = client->pers.weapon;
	ChangeWeapon (ent);
}

/*
=====================
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.

A client has just connected to the server in 
deathmatch mode, so clear everything out before starting them.
=====================
*/
void ClientBegin (edict_t *ent)
{
	qboolean found=false;
	struct admin_s *admin;

	G_InitEdict (ent); //makes ent->inuse = true

	if (level.match_state == WARMUP || level.match_state == PREGAME || level.match_state == END)
		InitClientResp (ent->client);

	// locate ent at a spawn point
	PutClientInServer (ent, true);

	ent->client->resp.ingame = true;

	if ( !ent->client->pers.mvdspec )
		gi.bprintf (ATTR_PRINT_HIGH|ATTR_BOLD, "%s entered the server (players = %d, spectators = %d).\n", ent->client->pers.netname, level.num_A_players+level.num_B_players, level.num_spectators+1);

	// spawn in spectator mode
	if (level.intermissiontime)
	{
		MoveClientToIntermission (ent);
	}
	else if(ent->client->pers.save_data.team != TEAM_NONE)
	{
		// send effect
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_LOGIN);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		if (ent->client->pers.save_data.team == TEAM_A)
			Write_ConfigString2(ent);
		else if (ent->client->pers.save_data.team == TEAM_B)
			Write_ConfigString2(ent);
	}
	else if (ent->client->pers.save_data.team == TEAM_NONE && (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH))
	{
		if (!ent->client->chase_target || ent->client->chase_target->inuse == false)
			GetChaseTarget(ent);
	}

	if (ent->client->pers.save_data.is_admin == true)
	{
		for (admin = game.admin_first; admin; admin = admin->next)
		{
			if (strlen(admin->password))
			{
				if (!strcmp(admin->password, ent->client->pers.save_data.admin_password))
				{
					found = true;
					break;
				}
			}
		}
	}
	if (!found)
	{
		ent->client->pers.save_data.is_admin = false;
		ent->client->pers.save_data.admin_flags = 0;
		ent->client->pers.save_data.admin_password[0] = '\0';
		ent->client->pers.save_data.judge = false;
	}

	// make sure all view stuff is valid
	ClientEndServerFrame (ent);

	EventCall( EVENT_CLIENTBEGIN, 0, (long)ent );

}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/

#define S_ID 2
#define S_HUD 4
#define S_NEWDROPS 8
#define S_AUTORECORD 16
#define S_AUTOSCREENSHOT 32
#define S_AUTOSCREENSHOTJPG 64

void ClientUserinfoChanged (edict_t *ent, char *userinfo)
{
	char	*s;
	char	name[16];
	char	stufftext[128];
	int		playernum, len;
	int		i, startups;
	int		j=0;
	qboolean skipadmin = false;
	qboolean skipreferee = false;
	struct admin_s *admin;

	s = Info_ValueForKey (userinfo, "spectator");
	if (s)
	{
		startups = atoi(s);
		if (startups & S_HUD)
			ent->client->pers.save_data.hudlist = 2;
		if (startups & S_ID)
			ent->client->pers.save_data.hudid = true;
		if (startups & S_NEWDROPS)
			ent->client->pers.save_data.new_drops = true;
		if (startups & S_AUTORECORD)
			ent->client->pers.save_data.autorecord = true;
		if (startups & S_AUTOSCREENSHOT)
			ent->client->pers.save_data.autoscreenshot = 1;
		if (startups & S_AUTOSCREENSHOTJPG)
			ent->client->pers.save_data.autoscreenshot = 2;
	}

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		strcpy (userinfo, "\\name\\badinfo\\skin\\male/grunt");
	}

	// set name
	if ( EventCall( EVENT_NAMECLIENTUSERINFOCHANGED, (unsigned int)userinfo, (long)ent ) == true )
	{
		s = Info_ValueForKey (userinfo, "name");
		if (!ent->client->pers.save_data.is_admin)
		{
			if (!Q_strncasecmp(s, "[admin]", 7))
				skipadmin = true;
			if (sv_referee_tag->string)
			{
				len = strlen(sv_referee_tag->string);
				if (len)
				{
					if (!Q_strncasecmp(s, sv_referee_tag->string, len))
						skipreferee = true;
				}
			}
		}

		if (ent->client->pers.save_data.is_admin)
		{
			for(admin = game.admin_first; admin; admin = admin->next)
			{
				if (!strcmp(ent->client->pers.save_data.admin_password, admin->password))
				{
					//				if (!admin->judge)
					//					s = strdup(admin->nick);
					break;
				}
			}
			if (Q_strncasecmp(s, "[admin]", 7) && !admin->judge)
			{
				s[8] = 0;
				Com_sprintf(name, 16, "[admin]%s", admin->nick);
				strncpy (ent->client->pers.netname, name, sizeof(ent->client->pers.netname)-1);
				if (ent->client->resp.ingame)
				{
					sprintf(stufftext, "name %s", name);
					gi.WriteByte(svc_stufftext);
					gi.WriteString(stufftext);
					gi.unicast(ent, false);
				}
				s = ent->client->pers.netname;
			}
			else if (Q_strncasecmp(s, sv_referee_tag->string, strlen(sv_referee_tag->string)) && admin->judge)
			{
				s = Info_ValueForKey (userinfo, "name");
				Com_sprintf(name, 16, "%s%s", sv_referee_tag->string, s);
				strncpy (ent->client->pers.netname, name, sizeof(ent->client->pers.netname)-1);
				if (ent->client->resp.ingame)
				{
					sprintf(stufftext, "name %s", name);
					gi.WriteByte(svc_stufftext);
					gi.WriteString(stufftext);
					gi.unicast(ent, false);
				}
				s = ent->client->pers.netname;
			}
		}

		if(strcmp(s, ent->client->pers.netname) || !strlen(ent->client->pers.netname) || skipadmin || skipreferee)
		{
			char *tempBuff;
			if (skipadmin)
				tempBuff = s+7;
			else if (skipreferee)
				tempBuff = s + strlen(sv_referee_tag->string);
			else
				tempBuff = s;

			for(i=0; i<strlen(tempBuff); i++)
			{
				if (i >= 15)
					break;
				if (tempBuff[i] != '~' && tempBuff[i] != '`' && tempBuff[i] != '%')
				{
					name[j] = tempBuff[i];
					j++;
				}
			}
			name[j] = '\0';

			if (name[0] == 0)
				Com_sprintf(name, 16, "whatismyname?");

			if (strlen(ent->client->pers.netname) && strcmp(name, ent->client->pers.netname) && strncmp(ent->client->pers.netname, "[admin]",7) && !skipadmin && !skipreferee)
			{
				Log(ent, LOG_CHANGE, name);
				if (sv_displaynamechange->value == 1)
					gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s changed name to %s.\n", ent->client->pers.netname, name);
			}
			strncpy (ent->client->pers.netname, name, sizeof(ent->client->pers.netname)-1);
			Info_SetValueForKey (userinfo, "name", name);
			if ((skipreferee || skipadmin) && ent->client->resp.ingame)
			{
				sprintf(stufftext, "name %s", name);
				gi.WriteByte(svc_stufftext);
				gi.WriteString(stufftext);
				gi.unicast(ent, false);
			}
		}
	}

	// set skin
	s = Info_ValueForKey (userinfo, "skin");

	playernum = ent-g_edicts-1;

	// combine name and skin into a configstring


	if (ent->client->pers.save_data.team == TEAM_A)
		gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s", ent->client->pers.netname, game.teamA_skin) );
	else if (ent->client->pers.save_data.team == TEAM_B)
		gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s", ent->client->pers.netname, game.teamB_skin) );
	else
		gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s", ent->client->pers.netname, s) );

	// fov
	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		ent->client->ps.fov = 90;
	}
	else
	{
		ent->client->ps.fov = atoi(Info_ValueForKey(userinfo, "fov"));
		if (ent->client->ps.fov < 1)
			ent->client->ps.fov = 90;
		else if (ent->client->ps.fov > 160)
			ent->client->ps.fov = 160;
	}

	// handedness
	s = Info_ValueForKey (userinfo, "hand");
	if (strlen(s))
	{
		ent->client->pers.hand = atoi(s);
	}

	// save off the userinfo in case we want to check something later
	strncpy (ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo)-1);
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
qboolean ClientConnect (edict_t *ent, char *userinfo)
{
	char			*value;
	unsigned long	IPMasked[4];
	char			IPString[25];
	char			*word;
	int				i, kropka=0;
	unsigned long	IP=0;

	IPString[0] = '\0';

	// check to see if they are on the banned IP list
	value = (Info_ValueForKey (userinfo, "ip"));
	if (value)
	{
		if (!strcmp(value, "loopback"))
			Com_sprintf(IPString, 25, "127.0.0.1:27900");
		else
			Com_sprintf(IPString, 25, "%s", value);
	}

	value = (Info_ValueForKey (userinfo, "name"));
	if (value)
	{
		if ( strchr( value, '%' ) != NULL )
		{
			Info_SetValueForKey(userinfo, "rejmsg", "Your name contain invalid character.");
			return false;
		}
	}

	if (!strlen(IPString))
	{
		Info_SetValueForKey(userinfo, "rejmsg", "Your IP address is unknow to the serwer. Try to restart Quake 2 and reconnect.");
		return false;
	}

	word = strtok(IPString, ".");
	for (i=0; i<3; i++)
	{
		if (!word)
			break;
		
		IPMasked[i] = atoi(word);
		IP = IP | (IPMasked[i]<<(8*i));
		word = strtok(NULL, ".");
	}
	if (word)
		IP |= atoi(word)<<24;

	if (Is_Baned(IP))
	{
		Info_SetValueForKey(userinfo, "rejmsg", "You are Banned.");
		return false;
	}

	// check for a password
	value = Info_ValueForKey (userinfo, "password");
	if (*password->string && strcmp(password->string, "none") && strcmp(password->string, value))
	{
		Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
		return false;
	}

	MyCPrintfInit(ent);

	ent->client = game.clients + (ent - g_edicts - 1);

	ent->client->pers.save_data.reconnected = false;
	memset (&ent->client->pers.save_data, 0, sizeof(ent->client->pers.save_data));
	ent->client->pers.save_data.ip = IP;
	ent->client->pers.save_data.hudid = true; //ID on by default
	ent->client->pers.save_data.team = TEAM_NONE;

	InitClientResp (ent->client);
	InitClientPersistant (ent->client);

	if ( game.server_features & GMF_MVDSPEC )
	{
		value = Info_ValueForKey( userinfo, "mvdspec" );
		if ( value[0] != 0 )
			ent->client->pers.mvdspec = true;
	}

	ClientUserinfoChanged (ent, userinfo);

	if (game.maxclients > 1 && !ent->client->pers.mvdspec)
		gi.bprintf (ATTR_PRINT_HIGH|ATTR_BOLD, "%s connected from %d.%d.%d.x.\n", ent->client->pers.netname, IPMasked[0], IPMasked[1], IPMasked[2]);

	ent->svflags = 0; // make sure we start with known default
	Log(ent, LOG_CONNECT, "connected");

	/* let plugins decide */
	if ( EventCall( EVENT_CLIENTCONNECT, 0, (long)ent ) == false )
		return false;
	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect (edict_t *ent)
{
	int		playernum;

	if (!ent->client)
		return;

	//TDM++
	Log(ent, LOG_CONNECT, "disconnected");

	if ( !ent->client->pers.mvdspec )
	{
		AddPlayerToOldScore(ent);
		SavePlayerData(ent);

		ent->client->pers.save_data.team = TEAM_NONE;

		CheckCaptain(ent);
		VoteCheckDisconnect(ent);
	}

	//TDM--

	ent->inuse = false;

	if ( !ent->client->pers.mvdspec || ent->client->pers.save_data.team != TEAM_NONE )
	{
		gi.bprintf (ATTR_PRINT_HIGH|ATTR_BOLD, "%s disconnected (players = %d, spectators = %d).\n", ent->client->pers.netname, level.num_A_players+level.num_B_players, level.num_spectators);

		// send effect
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_LOGOUT);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
	}

	gi.unlinkentity (ent);
	ent->s.effects = ent->s.event = ent->s.sound = ent->s.modelindex = 0; //TDM
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
	ent->classname = "disconnected";
	//TDM++
	ent->client->pers.save_data.ready_or_not = false;
	ent->client->pers.connected = false;
	//TDM--

	playernum = ent-g_edicts-1;
	gi.configstring (CS_PLAYERSKINS+playernum, "");
	EventCall( EVENT_CLIENTDISCONNECT, 0, (long)ent );
}


//==============================================================


edict_t	*pm_passent;

// pmove doesn't need to know about passent and contentmask
trace_t	PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	if (pm_passent->health > 0)
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
}

unsigned CheckBlock (void *b, int c)
{
	int	v,i;
	v = 0;
	for (i=0 ; i<c ; i++)
		v+= ((byte *)b)[i];
	return v;
}
void PrintPmove (pmove_t *pm)
{
	unsigned	c1, c2;

	c1 = CheckBlock (&pm->s, sizeof(pm->s));
	c2 = CheckBlock (&pm->cmd, sizeof(pm->cmd));
	Com_Printf ("sv %3i:%i %i\n", pm->cmd.impulse, c1, c2);
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void ClientThink (edict_t *ent, usercmd_t *ucmd)
{
	gclient_t	*client;
	edict_t	*other;
	int		i, j;
	pmove_t	pm;

	level.current_entity = ent;
	client = ent->client;

	if (level.paused && ent->movetype != MOVETYPE_NOCLIP)
	{
		client->ps.pmove.pm_type = PM_FREEZE;
		return;
	}

	if (level.intermissiontime )
	{
		client->ps.pmove.pm_type = PM_FREEZE;
		// can exit intermission after five seconds
		if (level.time > level.intermissiontime + GAMESECONDS(5) && (ucmd->buttons & BUTTON_ANY) && level.exitintermission == false )
		{
			level.exitintermission = true;
			level.exitintermissionblend = 0;
		}
		return;
	}

	pm_passent = ent;

	if (ent->client->chase_target && ent->client->chase_target->inuse)
	{

		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

	}
	else if (ent->client->chase_target && !ent->client->chase_target->inuse)
		UpdateChaseCam(ent);
	else {

		// set up for pmove
		memset (&pm, 0, sizeof(pm));

		if (ent->movetype == MOVETYPE_NOCLIP)
			client->ps.pmove.pm_type = PM_SPECTATOR;
		else if (ent->s.modelindex != 255)
			client->ps.pmove.pm_type = PM_GIB;
		else if (ent->deadflag)
			client->ps.pmove.pm_type = PM_DEAD;
		else
			client->ps.pmove.pm_type = PM_NORMAL;

		client->ps.pmove.gravity = sv_gravity->value;
		pm.s = client->ps.pmove;

		for (i=0 ; i<3 ; i++)
		{
			pm.s.origin[i] = ent->s.origin[i]*8;
			pm.s.velocity[i] = ent->velocity[i]*8;
		}

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
		{
			pm.snapinitial = true;
	//		gi.dprintf ("pmove changed!\n");
		}

		pm.cmd = *ucmd;

		pm.trace = PM_trace;	// adds default parms
		pm.pointcontents = gi.pointcontents;

		// perform a pmove
		gi.Pmove (&pm);

		// save results of pmove
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		for (i=0 ; i<3 ; i++)
		{
			ent->s.origin[i] = pm.s.origin[i]*0.125;
			ent->velocity[i] = pm.s.velocity[i]*0.125;
		}

		VectorCopy (pm.mins, ent->mins);
		VectorCopy (pm.maxs, ent->maxs);

		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

		if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) && (pm.waterlevel == 0))
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
			PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
		}

		ent->viewheight = pm.viewheight;
		ent->waterlevel = pm.waterlevel;
		ent->watertype = pm.watertype;
		ent->groundentity = pm.groundentity;
		if (pm.groundentity)
			ent->groundentity_linkcount = pm.groundentity->linkcount;

		if (ent->deadflag)
		{
			client->ps.viewangles[ROLL] = 40;
			client->ps.viewangles[PITCH] = -15;
			client->ps.viewangles[YAW] = client->killer_yaw;
		}
		else
		{
			VectorCopy (pm.viewangles, client->v_angle);
			VectorCopy (pm.viewangles, client->ps.viewangles);
		}

		gi.linkentity (ent);

		if (ent->movetype != MOVETYPE_NOCLIP)
			G_TouchTriggers (ent);

		// touch other objects
		for (i=0 ; i<pm.numtouch ; i++)
		{
			other = pm.touchents[i];
			for (j=0 ; j<i ; j++)
				if (pm.touchents[j] == other)
					break;
			if (j != i)
				continue;	// duplicated
			if (!other->touch)
				continue;
			other->touch (other, ent, NULL, NULL);
		}

	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// save light level the player is standing on for
	// monster sighting AI
	ent->light_level = ucmd->lightlevel;

	// fire weapon from final position if needed
	if (client->latched_buttons & BUTTON_ATTACK)
	{

		if (sv_spawn_invincible->value && (ent->deadflag & DEAD_NO))
		{
			if (ent->client->spawn_invincible)
			{
				ent->client->spawn_invincible = false;
				ent->client->invincible_framenum = 0;
			}
		}

		if (client->pers.save_data.team == TEAM_NONE)
		{
			client->latched_buttons = 0;

			if (client->chase_target && client->chase_target->inuse && !client->showmenu)
				ChangeChaseMode( ent );
			else
			{
				if (!client->showmenu)
					Cmd_Menu_f(ent);
				else
					menu_enter(ent, true);
			}
		}
		else if (!client->weapon_thunk)
		{
			client->weapon_thunk = true;
			Think_Weapon (ent);
		}
	}

	if (client->pers.save_data.team == TEAM_NONE)
	{
		if (ucmd->upmove >= 10)
		{
			if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD))
			{
				client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
				if (client->chase_target && client->chase_target->inuse)
					ChaseNext(ent);
			}
		}
		else
			client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
	}

	// update chase cam if being followed
	for (i = 1; i <= maxclients->value; i++)
	{
		other = g_edicts + i;
		if (other->inuse && other->client->chase_target == ent)
			UpdateChaseCam(other);
	}
}


/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame (edict_t *ent)
{
	gclient_t	*client;

	if (level.intermissiontime)
		return;

	client = ent->client;

	if ((level.match_state == WARMUP || level.match_state == PREGAME) && client->pers.save_data.team != TEAM_NONE)
	{
		if (client->buttons & BUTTON_ANY)
			client->resp.idle = 0;
		else
			client->resp.idle += GAMESECONDS(FRAMETIME);

		if (client->resp.idle >= GAMESECONDS(120))
		{
			client->resp.idle = 0;
			SpectateOrChase (ent, false);
		}
	}
	// run weapon animations if it hasn't been done by a ucmd_t
	if (!client->weapon_thunk && client->pers.save_data.team != TEAM_NONE)
		Think_Weapon (ent);
	else
		client->weapon_thunk = false;

	if (ent->deadflag && level.time > client->respawn_time-GAMESECONDS(4.3))
	{
		// wait for any button just going down
		if (level.time > client->respawn_time && ((int)dmflags->value & DF_FORCE_RESPAWN))
		{
				respawn(ent, true, true);
				client->latched_buttons = 0;
				gi.cprintf(ent, PRINT_HIGH, "Forcing respawn...\n");
		}
		else if (client->latched_buttons & BUTTON_ATTACK)
		{
			respawn(ent, true, true);
			client->latched_buttons = 0;
		}
		return;
	}

	client->latched_buttons = 0;
}
