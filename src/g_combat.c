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

// g_combat.c

#include "g_local.h"

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage (edict_t *targ, edict_t *inflictor)
{
	vec3_t	dest;
	trace_t	trace;

// bmodels need special checking because their origin is 0,0,0
	if (targ->movetype == MOVETYPE_PUSH)
	{
		VectorAdd (targ->absmin, targ->absmax, dest);
		VectorScale (dest, 0.5, dest);
		trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
		if (trace.fraction == 1.0)
			return true;
		if (trace.ent == targ)
			return true;
		return false;
	}
	
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, targ->s.origin, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;


	return false;
}


/*
============
Killed
============
*/
void Killed (edict_t *targ, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (targ->health < -999)
		targ->health = -999;

	targ->enemy = attacker;

	if (targ->movetype == MOVETYPE_PUSH || targ->movetype == MOVETYPE_STOP || targ->movetype == MOVETYPE_NONE)
	{	// doors, triggers, etc
		targ->die (targ, inflictor, attacker, damage, point);
		return;
	}

	targ->die (targ, inflictor, attacker, damage, point);
}


/*
================
SpawnDamage
================
*/
void SpawnDamage (int type, vec3_t origin, vec3_t normal, int damage)
{
	if (damage > 255)
		damage = 255;
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (type);
//	gi.WriteByte (damage);
	gi.WritePosition (origin);
	gi.WriteDir (normal);
	gi.multicast (origin, MULTICAST_PVS);
}


/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack
point		point at which the damage is being inflicted
normal		normal vector from that point
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_ENERGY			damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_BULLET			damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/
static int CheckPowerArmor (edict_t *ent, vec3_t point, vec3_t normal, int damage, int dflags)
{
	gclient_t	*client;
	int			save;
	int			power_armor_type;
	int			index=0;
	int			damagePerCell;
	int			pa_te_type;
	int			power=0;
	int			power_used;

	if (!damage)
		return 0;

	client = ent->client;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	if (client)
	{
		power_armor_type = PowerArmorType (ent);
		if (power_armor_type != POWER_ARMOR_NONE)
		{
			index = ITEM_INDEX(FindItem("Cells"));
			power = client->pers.inventory[index];
		}
	}
	else
		return 0;

	if (power_armor_type == POWER_ARMOR_NONE)
		return 0;
	if (!power)
		return 0;

	if (power_armor_type == POWER_ARMOR_SCREEN)
	{
		vec3_t		vec;
		float		dot;
		vec3_t		forward;

		// only works if damage point is in front
		AngleVectors (ent->s.angles, forward, NULL, NULL);
		VectorSubtract (point, ent->s.origin, vec);
		VectorNormalize (vec);
		dot = DotProduct (vec, forward);
		if (dot <= 0.3)
			return 0;

		damagePerCell = 1;
		pa_te_type = TE_SCREEN_SPARKS;
		damage = damage / 3;
	}
	else
	{
		damagePerCell = 2;
		pa_te_type = TE_SHIELD_SPARKS;
		damage = (2 * damage) / 3;
	}

	save = power * damagePerCell;
	if (!save)
		return 0;
	if (save > damage)
		save = damage;

	SpawnDamage (pa_te_type, point, normal, save);
	ent->powerarmor_time = level.time + GAMESECONDS(0.2);

	power_used = save / damagePerCell;

	if (client)
		client->pers.inventory[index] -= power_used;
	return save;
}

static int CheckArmor (edict_t *ent, vec3_t point, vec3_t normal, int damage, int te_sparks, int dflags)
{
	gclient_t	*client;
	int			save;
	int			index;
	gitem_t		*armor;

	if (!damage)
		return 0;

	client = ent->client;

	if (!client)
		return 0;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	index = ArmorIndex (ent);
	if (!index)
		return 0;

	armor = GetItemByIndex (index);

	if (dflags & DAMAGE_ENERGY)
		save = ceil(((gitem_armor_t *)armor->info)->energy_protection*damage);
	else
		save = ceil(((gitem_armor_t *)armor->info)->normal_protection*damage);
	if (save >= client->pers.inventory[index])
		save = client->pers.inventory[index];

	if (!save)
		return 0;

	client->pers.inventory[index] -= save;
	SpawnDamage (te_sparks, point, normal, save);

	return save;
}

qboolean CheckTeamDamage (edict_t *targ, edict_t *attacker)
{
	if (!targ->client || !attacker->client)
		return false;
	if (targ == attacker)
		return false;
	if (targ->client->pers.save_data.team == attacker->client->pers.save_data.team)
		return true;
	return false;
}

void T_Damage (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, int mod)
{
	gclient_t	*client;
	int			take;
	int			save;
	int			asave;
	int			psave;
	int			te_sparks;

	if (!targ->takedamage)
		return;

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if (targ != attacker)
	{
		if (OnSameTeam (targ, attacker))
		{
			if ((int)(dmflags->value) & DF_NO_FRIENDLY_FIRE)
				damage = 0;
		}
	}
//TDM++
	else if (game.tp == 1)
		damage = 0;
//TDM--

	meansOfDeath = mod;

	client = targ->client;

	if (dflags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;

	VectorNormalize(dir);

	if (targ->flags & FL_NO_KNOCKBACK)
		knockback = 0;

// figure momentum add
	if (!(dflags & DAMAGE_NO_KNOCKBACK))
	{
		if ((knockback) && (targ->movetype != MOVETYPE_NONE) && (targ->movetype != MOVETYPE_BOUNCE) && (targ->movetype != MOVETYPE_PUSH) && (targ->movetype != MOVETYPE_STOP))
		{
			vec3_t	kvel;
			float	mass;

			if (targ->mass < 50)
				mass = 50;
			else
				mass = targ->mass;

			if (targ->client  && attacker == targ)
				VectorScale (dir, 1600.0 * (float)knockback / mass, kvel);	// the rocket jump hack...
			else
				VectorScale (dir, 500.0 * (float)knockback / mass, kvel);

			VectorAdd (targ->velocity, kvel, targ->velocity);
		}
	}

	take = damage;
	save = 0;

	// check for godmode
	if ( (targ->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION) )
	{
		take = 0;
		save = damage;
		SpawnDamage (te_sparks, point, normal, save);
	}

	// check for invincibility
	if ((client && client->invincible_framenum > level.framenum ) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		if (targ->pain_debounce_time < level.time)
		{
			gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
			targ->pain_debounce_time = level.time + GAMESECONDS(2);
		}
		take = 0;
		save = damage;
	}

	psave = CheckPowerArmor (targ, point, normal, take, dflags);
	take -= psave;

	asave = CheckArmor (targ, point, normal, take, te_sparks, dflags);
	take -= asave;

//TDM++
	if (game.tp == 3) //only teammate health protection
		take = 0;
//TDM--

	//treat cheat/powerup savings the same as armor
	asave += save;

// do the damage
	if (take)
	{
		int	after;

		if (client)
			SpawnDamage (TE_BLOOD, point, normal, take);
		else
			SpawnDamage (te_sparks, point, normal, take);

		if (targ->health > 0 && targ->client && attacker->client && targ != attacker && level.match_state != WARMUP)
		{
			after = targ->health - take;
			if (targ->client->pers.save_data.team != attacker->client->pers.save_data.team)
			{
				switch (mod)
				{
				case MOD_BLASTER:
					targ->client->pers.save_data.weaps[WEAP_BLASTER-1].dmr += damage;
					attacker->client->pers.save_data.weaps[WEAP_BLASTER-1].dmg += damage;
					if (after <=0)
					{
						targ->client->pers.save_data.weaps[WEAP_BLASTER-1].deaths++;
						attacker->client->pers.save_data.weaps[WEAP_BLASTER-1].kills++;
					}
				break;
				case MOD_SHOTGUN:
					targ->client->pers.save_data.weaps[WEAP_SHOTGUN-1].dmr += damage;
					attacker->client->pers.save_data.weaps[WEAP_SHOTGUN-1].dmg += damage;
					if (after <=0)
					{
						targ->client->pers.save_data.weaps[WEAP_SHOTGUN-1].deaths++;
						attacker->client->pers.save_data.weaps[WEAP_SHOTGUN-1].kills++;
					}
				break;
				case MOD_SSHOTGUN:
					targ->client->pers.save_data.weaps[WEAP_SUPERSHOTGUN-1].dmr += damage;
					attacker->client->pers.save_data.weaps[WEAP_SUPERSHOTGUN-1].dmg += damage;
					if (after <=0)
					{
						targ->client->pers.save_data.weaps[WEAP_SUPERSHOTGUN-1].deaths++;
						attacker->client->pers.save_data.weaps[WEAP_SUPERSHOTGUN-1].kills++;
					}
				break;
				case MOD_MACHINEGUN:
					targ->client->pers.save_data.weaps[WEAP_MACHINEGUN-1].dmr += damage;
					attacker->client->pers.save_data.weaps[WEAP_MACHINEGUN-1].dmg += damage;
					if (after <=0)
					{
						targ->client->pers.save_data.weaps[WEAP_MACHINEGUN-1].deaths++;
						attacker->client->pers.save_data.weaps[WEAP_MACHINEGUN-1].kills++;
					}
				break;
				case MOD_CHAINGUN:
					targ->client->pers.save_data.weaps[WEAP_CHAINGUN-1].dmr += damage;
					attacker->client->pers.save_data.weaps[WEAP_CHAINGUN-1].dmg += damage;
					if (after <=0)
					{
						targ->client->pers.save_data.weaps[WEAP_CHAINGUN-1].deaths++;
						attacker->client->pers.save_data.weaps[WEAP_CHAINGUN-1].kills++;
					}
				break;
				case MOD_HYPERBLASTER:
					targ->client->pers.save_data.weaps[WEAP_HYPERBLASTER-1].dmr += damage;
					attacker->client->pers.save_data.weaps[WEAP_HYPERBLASTER-1].dmg += damage;
					if (after <=0)
					{
						targ->client->pers.save_data.weaps[WEAP_HYPERBLASTER-1].deaths++;
						attacker->client->pers.save_data.weaps[WEAP_HYPERBLASTER-1].kills++;
					}
				break;
				case MOD_RAILGUN:
					targ->client->pers.save_data.weaps[WEAP_RAILGUN-1].dmr += damage;
					attacker->client->pers.save_data.weaps[WEAP_RAILGUN-1].dmg += damage;
					if (after <=0)
					{
						targ->client->pers.save_data.weaps[WEAP_RAILGUN-1].deaths++;
						attacker->client->pers.save_data.weaps[WEAP_RAILGUN-1].kills++;
					}
				break;
				case MOD_HANDGRENADE:
				case MOD_HG_SPLASH:
				case MOD_HELD_GRENADE:
					targ->client->pers.save_data.weaps[WEAP_GRENADES-1].dmr += damage;
					attacker->client->pers.save_data.weaps[WEAP_GRENADES-1].dmg += damage;
					if (after <=0)
					{
						targ->client->pers.save_data.weaps[WEAP_GRENADES-1].deaths++;
						attacker->client->pers.save_data.weaps[WEAP_GRENADES-1].kills++;
					}
				break;
				case MOD_G_SPLASH:
				case MOD_GRENADE:
					targ->client->pers.save_data.weaps[WEAP_GRENADELAUNCHER-1].dmr += damage;
					attacker->client->pers.save_data.weaps[WEAP_GRENADELAUNCHER-1].dmg += damage;
					if (after <=0)
					{
						targ->client->pers.save_data.weaps[WEAP_GRENADELAUNCHER-1].deaths++;
						attacker->client->pers.save_data.weaps[WEAP_GRENADELAUNCHER-1].kills++;
					}
				break;
				case MOD_ROCKET:
				case MOD_R_SPLASH:
					targ->client->pers.save_data.weaps[WEAP_ROCKETLAUNCHER-1].dmr += damage;
					attacker->client->pers.save_data.weaps[WEAP_ROCKETLAUNCHER-1].dmg += damage;
					if (after <=0)
					{
						targ->client->pers.save_data.weaps[WEAP_ROCKETLAUNCHER-1].deaths++;
						attacker->client->pers.save_data.weaps[WEAP_ROCKETLAUNCHER-1].kills++;
					}
				break;
				}
			}
			else
			{
				targ->client->pers.save_data.team_damr += damage;
				attacker->client->pers.save_data.team_damg += damage;
			}
		}

		targ->health = targ->health - take;
			
		if (targ->health <= 0)
		{
			if (client)
				targ->flags |= FL_NO_KNOCKBACK;
			Killed (targ, inflictor, attacker, take, point);
			return;
		}
	}

	if (client)
	{
		if (!(targ->flags & FL_GODMODE) && (take))
			targ->pain (targ, attacker, knockback, take);
	}
	else if (take)
	{
		if (targ->pain)
			targ->pain (targ, attacker, knockback, take);
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client)
	{
		client->damage_parmor += psave;
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		VectorCopy (point, client->damage_from);
	}
}


/*
============
T_RadiusDamage
============
*/
qboolean T_RadiusDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod)
{
	float	points;
	edict_t	*ent = NULL;
	vec3_t	v;
	vec3_t	dir;
	qboolean hit = false;

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (inflictor->s.origin, v, v);
		points = damage - 0.5 * VectorLength (v);
		if (ent == attacker)
			points = points * 0.5;
		if (points > 0)
		{
			if (CanDamage (ent, inflictor))
			{
				VectorSubtract (ent->s.origin, inflictor->s.origin, dir);
				if (ent->client && attacker->client && ent != attacker && !ent->deadflag && level.match_state != WARMUP)
				{
					if (ent->client->pers.save_data.team != attacker->client->pers.save_data.team)
						hit = true;
				}
				T_Damage (ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
			}
		}
	}
	return hit;
}
