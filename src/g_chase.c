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

void StopChasing(edict_t *ent)
{
	ent->client->chase_mode = CHASE_FREE;
	ent->client->clientNum = ent - g_edicts - 1;
	ent->client->chase_target = NULL;
	ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	ent->client->ps.gunindex = 0;
	ent->client->ps.gunframe = 0;
	VectorClear( ent->client->ps.gunangles );
	VectorClear( ent->client->ps.kick_angles );
}

void SetChaseHudScore( edict_t *ent )
{
	int diff;
	struct gclient_s *cl_target = ( ent->client->chase_target && ent->client->chase_target->inuse ) ? ent->client->chase_target->client : NULL;
	char text[32];

	if ( cl_target && cl_target->pers.save_data.team != TEAM_NONE )
	{
		diff = cl_target->pers.save_data.team == TEAM_A ? ( level.teamA_score - level.teamB_score ) : ( level.teamB_score - level.teamA_score );
		Com_sprintf( text, sizeof( text ), "%s%d (%d)%d:%d", diff > 0 ? "+" : "", diff, cl_target->resp.score, cl_target->pers.save_data.team == TEAM_A ? level.teamA_score : level.teamB_score, cl_target->pers.save_data.team == TEAM_A ? level.teamB_score : level.teamA_score );
	}
	else
		Com_sprintf( text, sizeof( text ), "%d:%d", level.teamA_score, level.teamB_score);

	gi.WriteByte( svc_configstring );
	gi.WriteShort( CS_GENERAL + (ent - g_edicts) );
	gi.WriteString( text );
	gi.unicast( ent, true );
}

void UpdateChaseCam(edict_t *ent)
{
	vec3_t o, ownerv, goal;
	edict_t *targ;
	vec3_t forward, right;
	trace_t trace;
	int i;
	vec3_t oldgoal;
	vec3_t angles;

	/*is our chase target gone?*/
	if (!ent->client->chase_target->inuse || ent->client->chase_target->client->pers.save_data.team == TEAM_NONE)
	{
		edict_t *old = ent->client->chase_target;
		ChaseNext(ent);
		if (ent->client->chase_target == old)
		{
			StopChasing( ent );
			SetChaseHudScore( ent );
			return;
		}
	}

	if ( ent->client->update_chase )
		SetChaseHudScore( ent );

	targ = ent->client->chase_target;

	VectorCopy(targ->s.origin, ownerv);
	VectorCopy(ent->s.origin, oldgoal);

	ownerv[2] += targ->viewheight;

	if (ent->client->chase_mode == CHASE_BEHIND)
	{
		VectorCopy(targ->client->v_angle, angles);
		if (angles[PITCH] > 56)
			angles[PITCH] = 56;
		AngleVectors (angles, forward, right, NULL);
		VectorNormalize(forward);
		VectorMA(ownerv, -60, forward, o);

		if (o[2] < targ->s.origin[2] + 20)
			o[2] = targ->s.origin[2] + 20;

		/*jump animation lifts*/
		if (!targ->groundentity)
			o[2] += 16;

		trace = gi.trace(ownerv, vec3_origin, vec3_origin, o, targ, MASK_SOLID);

		VectorCopy(trace.endpos, goal);

		VectorMA(goal, 2, forward, goal);

		/*pad for floors and ceilings*/
		VectorCopy(goal, o);
		o[2] += 6;
		trace = gi.trace(goal, vec3_origin, vec3_origin, o, targ, MASK_SOLID);
		if (trace.fraction < 1)
		{
			VectorCopy(trace.endpos, goal);
			goal[2] -= 6;
		}

		VectorCopy(goal, o);
		o[2] -= 6;
		trace = gi.trace(goal, vec3_origin, vec3_origin, o, targ, MASK_SOLID);
		if (trace.fraction < 1)
		{
			VectorCopy(trace.endpos, goal);
			goal[2] += 6;
		}
		VectorCopy(goal, ent->s.origin);
	}
	else if (ent->client->chase_mode == CHASE_INEYES || ent->client->chase_mode == CHASE_FOLLOWQD)
	{
		VectorCopy(targ->client->v_angle, ent->client->v_angle);
		if ( !( game.server_features & GMF_CLIENTNUM ) )
		{
			VectorCopy(targ->client->v_angle, angles);
			angles[0] = 0;
			angles[2] = 0;
			AngleVectors (angles, forward, right, NULL);
			VectorNormalize(forward);
			VectorMA(ownerv, 16, forward, ownerv);
		}
		VectorCopy( ownerv, ent->s.origin );
	}

	if (targ->deadflag)
		ent->client->ps.pmove.pm_type = PM_DEAD;
	else
		ent->client->ps.pmove.pm_type = PM_FREEZE;

	for (i=0 ; i<3 ; i++)
		ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(targ->client->v_angle[i] - ent->client->resp.cmd_angles[i]);

	if (targ->deadflag)
	{
		ent->client->ps.viewangles[ROLL] = 40;
		ent->client->ps.viewangles[PITCH] = -15;
		ent->client->ps.viewangles[YAW] = targ->client->killer_yaw;
	}
	else
	{
		VectorCopy(targ->client->v_angle, ent->client->ps.viewangles);
		VectorCopy(targ->client->v_angle, ent->client->v_angle);
	}

	if ((!ent->client->showscores && !ent->client->showoldscore && !ent->client->showmenu && !ent->client->showsets &&
		!ent->client->showinventory && !ent->client->showhelp &&
		!(level.framenum & 31)) || ent->client->update_chase) {
		char s[1024];

		ent->client->update_chase = false;
		sprintf(s, "xv 0 yb -68 string \"Chasing '%s'\"",
			targ->client->pers.netname);
		gi.WriteByte (svc_layout);
		gi.WriteString (s);
		gi.unicast(ent, false);
	}


	ent->viewheight = 0;
	ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	gi.linkentity(ent);
}

void ChaseNext(edict_t *ent)
{
	int i;
	edict_t *e;

	if (!ent->client->chase_target)
		return;

	i = ent->client->chase_target - g_edicts;
	do {
		i++;
		if (i > maxclients->value)
			i = 1;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (e->client->pers.save_data.team != TEAM_NONE)
			break;
	} while (e != ent->client->chase_target);

	ent->client->chase_target = e;

	if ( ent->client->chase_mode == CHASE_FOLLOWQD )
		ent->client->chase_mode = CHASE_INEYES;

	if ( ent->client->chase_mode == CHASE_FOLLOWQD || ent->client->chase_mode == CHASE_INEYES )
		ent->client->clientNum = ent->client->chase_target - g_edicts - 1;
	else
		ent->client->clientNum = ent - g_edicts - 1;

	ent->client->update_chase = true;
}

void ChasePrev(edict_t *ent)
{
	int i;
	edict_t *e;

	if (!ent->client->chase_target)
		return;

	i = ent->client->chase_target - g_edicts;
	do {
		i--;
		if (i < 1)
			i = maxclients->value;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (e->client->pers.save_data.team != TEAM_NONE)
			break;
	} while (e != ent->client->chase_target);

	ent->client->chase_target = e;

	if ( ent->client->chase_mode == CHASE_FOLLOWQD )
		ent->client->chase_mode = CHASE_INEYES;

	if ( ent->client->chase_mode == CHASE_FOLLOWQD || ent->client->chase_mode == CHASE_INEYES )
		ent->client->clientNum = ent->client->chase_target - g_edicts - 1;
	else
		ent->client->clientNum = ent - g_edicts - 1;

	ent->client->update_chase = true;
}

void GetChaseTarget(edict_t *ent)
{
	int i;
	edict_t *other;

	for (i = 1; i <= maxclients->value; i++)
	{
		other = g_edicts + i;
		if (other->inuse && other->client->pers.save_data.team != TEAM_NONE)
		{
			ent->client->clientNum = other - g_edicts - 1;
			ent->client->chase_mode = CHASE_INEYES;
			ent->client->chase_target = other;
			ent->client->update_chase = true;
			UpdateChaseCam(ent);
			return;
		}
	}
}

void ChangeChaseMode( edict_t *ent )
{
	if ( ent->client->chase_mode == CHASE_INEYES )
	{
		ent->client->ps.gunindex = 0;
		ent->client->ps.gunframe = 0;
		ent->client->clientNum = ent - g_edicts - 1;
		ent->client->chase_mode = CHASE_BEHIND;
	}
	else if ( ent->client->chase_mode == CHASE_BEHIND )
	{
		if ( level.items & IS_QD )
			ent->client->chase_mode = CHASE_FOLLOWQD;
		else
			ent->client->chase_mode = CHASE_INEYES;
		ent->client->clientNum = ent->client->chase_target - g_edicts - 1;
	}
	else if ( ent->client->chase_mode == CHASE_FOLLOWQD || ent->client->chase_mode == CHASE_BEHIND )
	{
		ent->client->chase_mode = CHASE_INEYES;
		ent->client->clientNum = ent->client->chase_target - g_edicts - 1;
	}

	switch( ent->client->chase_mode )
	{
		case CHASE_INEYES: gi.cprintf(ent, PRINT_HIGH, "\"In-eyes\" chase cam mode.\n"); break;
		case CHASE_BEHIND: gi.cprintf(ent, PRINT_HIGH, "\"From behind\" chase cam mode.\n"); break;
		case CHASE_FOLLOWQD: gi.cprintf(ent, PRINT_HIGH, "\"Follow quad\" chase cam mode.\n"); break;
	}
}
