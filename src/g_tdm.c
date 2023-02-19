#if defined __linux__ && defined UDSYSTEM //update stuff
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <curl/curl.h>
#define closesocket close
#endif

#include "g_local.h"
#include "tdm.h"
#include <time.h>
#include "m_player.h"
#include "p_menu.h"
#if defined __linux__
#include "sha1/sha.h"
#endif
#include "version.h"

#ifdef PELLESC
#include <string.h>
char *strdup(const char *s1);
#endif

//void (*bprintf) (int printlevel, char *fmt, ...);

/*
0. Handy Functions
	0.A - MyCPrintf
	0.B - LongCPrintf
	0.C - Random - Mitchell-Moore Algorithm
	0.D - Finding client by name or ID
	0.E - Strings functions
	0.F - Log
1. Commands
	1.A - Say, Talk
	1.B - Ban
	1.C - Voting
	1.D - Admin
	1.E - Team
	1.F - Misc
2. Main Functions
	2.A - Update_ServerInfo
	2.B - CheckProposes (voting)
	2.C - CheckMatchState
	2.D - CheckForUpdate - Linux only
	2.E - UpdateConfigStrings
	2.F - CheckCaptain
	2.G - Spawn Ranges (random spawns)
3. Client Functions
	3.A - Write_ConfigString
	3.B - TDM_SelectSpawnPoint (prevent spawnfrags)
	3.C - DeathmatchScoreboardMessage
	3.D - Old Scores
	3.E - Calc_AvgPing
	3.F - Player ID
	3.G - Save Player
4. Server Commands
	4.A - Admin
	4.B - Update - Linux only
5. Mapers Things
	5.A - Timer/Score
6. Domination
*/

/*
** 0.A - MyCPrintf - use this instead of using gi.*printf multiple times with small amount of text.
*/

char *IPLongToString(unsigned long IP)
{
	static char ret[20];

	Com_sprintf(ret, 20, "%d.%d.%d.%d", IP&0xFF, (IP>>8)&0xFF, (IP>>16)&0xFF, (IP>>24)&0xFF);
	return ret;
}

void MyCPrintfEnd(edict_t *ent, int type, int level);

void MyCPrintfInit(edict_t *ent)
{
	if (!ent->client)
		return;

	memset(&ent->client->mycprintf_outbuff, 0, sizeof(ent->client->mycprintf_outbuff));
	ent->client->mycprintf_outbuff[0] = '\0';
}

void MyCPrintf(edict_t *ent, int type, int level, char *text, ...)
{
	va_list			argptr;
	static char		string[1024];

	va_start (argptr, text);
	vsprintf (string, text, argptr);
	va_end (argptr);

	if (strlen(ent->client->mycprintf_outbuff) + strlen(string) > 470)
		MyCPrintfEnd(ent, type, level);

	strcat(ent->client->mycprintf_outbuff, string);
}


void MyCPrintfEnd(edict_t *ent, int type, int level)
{
	if (!strlen(ent->client->mycprintf_outbuff))
		return;

	if (type == 0) //cprintf
		gi.cprintf(ent, level, ent->client->mycprintf_outbuff);
	else if (type == 1) //centerprintf
		gi.centerprintf(ent, ent->client->mycprintf_outbuff);
	else if (type == 2) //bprintf
		gi.bprintf(level, ent->client->mycprintf_outbuff);

	MyCPrintfInit(ent);
}

/*
** 0.B - LongCPrintf - use this instead of using gi.cprintf multiple times with large amount of text.
** maplist, banlist etc.
*/

void AddToLongCPrintf(edict_t *ent, char *text, ...)
{
	va_list			argptr;
	static char		string[1024];
	int				len;

	va_start (argptr, text);
	vsprintf (string, text, argptr);
	va_end (argptr);

	if (ent->client->longlist_outbuff)
	{
		len = strlen(ent->client->longlist_outbuff);
		ent->client->longlist_outbuff = (char *)realloc(ent->client->longlist_outbuff, len + strlen(string) + 1);
		strcat(ent->client->longlist_outbuff, string);
		ent->client->longlist_outbuff_len = len + strlen(string) + 1;
	}
	else
	{
		ent->client->longlist_outbuff = (char *)malloc(strlen(string)+1);
		strcpy(ent->client->longlist_outbuff, string);
		ent->client->longlist_outbuff_len = strlen(string)+1;
	}
}

//This function is called one time per server frame (to avoid overflows).
//p_view.c - ClientEndServerFrame.
void LongCPrintf(edict_t *ent)
{
	if (ent->client->longlist_outbuff)
	{
		char	out[471];

		memset(&out, 0, sizeof(out));
		if (ent->client->longlist_outbuff_len > 470)
		{
			strncpy(out, ent->client->longlist_outbuff, 470);
			gi.cprintf(ent, PRINT_HIGH, out);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, ent->client->longlist_outbuff);

		if (ent->client->longlist_outbuff_len > 470)
		{
			ent->client->longlist_outbuff_len -= 470;
			memmove(ent->client->longlist_outbuff, ent->client->longlist_outbuff+470, ent->client->longlist_outbuff_len);
			ent->client->longlist_outbuff = (char*)realloc(ent->client->longlist_outbuff, ent->client->longlist_outbuff_len);
		}
		else
		{
			free(ent->client->longlist_outbuff);
			ent->client->longlist_outbuff = NULL;
			ent->client->longlist_outbuff_len = 0;
		}
	}
}

/*
** 0.C - Random - Mitchell-Moore Algorithm
*/

static	int	rgiState[2+55];

void init_mm( void )
{
	int *piState;
	int iState;
	time_t rawtime;
	struct tm * timeinfo;

	gi.dprintf("\nInitializing random number generator...");
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	piState	= &rgiState[2];

	piState[-2]	= 55 - 55;
	piState[-1]	= 55 - 24;

	piState[0]	= ((int) timeinfo->tm_sec) & ((1 << 30) - 1);
	piState[1]	= 1;
	for ( iState = 2; iState < 55; iState++ )
	{
		piState[iState] = (piState[iState-1] + piState[iState-2])
			& ((1 << 30) - 1);
	}
	gi.dprintf("OK\n");
}

int number_mm( void )
{
    int *piState;
    int iState1;
    int iState2;
    int iRand;

    piState		= &rgiState[2];
    iState1	 	= piState[-2];
    iState2	 	= piState[-1];
    iRand	 	= (piState[iState1] + piState[iState2])
			& ((1 << 30) - 1);
    piState[iState1]	= iRand;
    if ( ++iState1 == 55 )
	iState1 = 0;
    if ( ++iState2 == 55 )
	iState2 = 0;
    piState[-2]		= iState1;
    piState[-1]		= iState2;
    return iRand >> 6;
}

int ranfr( int from, int to )
{
// Random number from range
    if ( (to-from) < 1 )
            return from;
    return ((number_mm() % (to-from+1)) + from);
}


void Write_ConfigString2(edict_t *ent);

/*
** 0.D - Finding client by name or ID
*/

int get_client_num_from_edict( edict_t *ent )
{
	int ret;

	ret = ent - g_edicts - 1;

	if ( ret < 0 || ret >= game.maxclients )
		return -1;

	return ret;
}

int	find_client_num(edict_t *ent, char *text)
{
	edict_t *cl=NULL;
	int	i, num, j, len;
	int	cli=-1;
	char	string[1024];
	char	name[16];
	qboolean find_by_name=true;
	qboolean toomany=false;

	num = atoi(text);
	sprintf(string,"%d", num);

	if (!strcmp(text, string))
		find_by_name=false;
	else
	{
		for (j=0; j<strlen(text); j++)
			text[j] = LOWER(text[j]);
	}

	len = strlen(text);

	for (i=0 ; i<game.maxclients ; i++)
	{
		cl = &g_edicts[1+i];
		if (!cl->inuse || cl->client->pers.mvdspec)
			continue;

		if (find_by_name)
		{
			for (j=0; j<strlen(cl->client->pers.netname); j++)
				name[j] = LOWER(cl->client->pers.netname[j]);
			name[j] = '\0';
			if (!strcmp(text, name))
				return i;
			else if (!strncmp(text, name, len) && cli == -1)
				cli = i;
			else if (!strncmp(text, name, len))
			{
				if (!toomany)
				{
					toomany = true;
					gi.cprintf(ent, PRINT_HIGH, "%s\n", g_edicts[1+cli].client->pers.netname);
				}
				cli = i;
				gi.cprintf(ent, PRINT_HIGH, "%s\n", cl->client->pers.netname);
			}
		}
		else
		{
			if (i == num)
				return i;
		}
	}

	if (!toomany && find_by_name && cli != -1)
		return cli;

	return -1;
}

edict_t *find_client(edict_t *ent, char *text)
{
	edict_t *cl=NULL;
	int	i, num, j, len;
	int	cli=-1;
	char	string[1024];
	char	name[16];
	qboolean find_by_name=true;
	qboolean toomany=false;

	num = atoi(text);
	sprintf(string,"%d", num);

	if (!strcmp(text, string))
		find_by_name=false;
	else
	{
		for (j=0; j<strlen(text); j++)
			text[j] = LOWER(text[j]);
	}

	len = strlen(text);

	for (i=0 ; i<game.maxclients ; i++)
	{
		cl = &g_edicts[1+i];
		if (!cl->inuse || cl->client->pers.mvdspec)
			continue;

		if (find_by_name)
		{
			for (j=0; j<strlen(cl->client->pers.netname); j++)
				name[j] = LOWER(cl->client->pers.netname[j]);
			name[j] = '\0';
			if (!strcmp(text, name))
				return cl;
			if (!strncmp(text, name, len) && cli == -1)
				cli = i;
			else if (!strncmp(text, name, len))
			{
				if (!toomany)
				{
					toomany = true;
					gi.cprintf(ent, PRINT_HIGH, "%s\n", g_edicts[1+cli].client->pers.netname);
				}
				cli = i;
				gi.cprintf(ent, PRINT_HIGH, "%s\n", cl->client->pers.netname);
			}
		}
		else
		{
			if (i == num)
				return cl;
		}
	}

	if (!toomany && find_by_name && cli != -1)
		return &g_edicts[1+cli];

	return NULL;
}

/*
** 0.E - Strings functions
*/

char *read_line(FILE *f)
{
	static char line[MAX_STRING_CHARS];
	int d, c=0, i=0;

	line[0] = '\0';

	do
    {
		c = fgetc(f);

		if (c != '\n' && c != '\t' && c != '\v' && c != '\r' && c != EOF)
		{
			if (i == 0 && (c == ' ' || c == '\t'))
				continue;
			if (c == '/')
			{
				d = fgetc(f);
				if (d == '/')
				{
					if (i==0)
					{
						do
						{
							c = fgetc(f);
						} while (!feof(f) && c != 10);
						c = 0;
						continue;
					}
					else
						break;
				}
				else
					ungetc(d, f);
			}
			line[i] = c;
			i++;
			if (i > (1024-1))
				break;
		}
		else if (i != 0)
			break;
    }
    while (!feof(f));

	if (i>0)
		line[i] = '\0';
	else
		return NULL;

	return line;
}

char *read_word(char *in, int position)
{
	static char *word;
	int	i=0;
	char		line[MAX_STRING_CHARS];

	strcpy(line, in);

	word = strtok(line, "\n\r\t\v ");

	while (word != NULL)
	{
		if (i == position)
			return word;
		i++;
		word = strtok(NULL, "\n\r\t\v ");
	}

	return "";
}

char *format_time( time_t timestamp )
{
	static char ret[32];
	struct tm *tim;

	tim = gmtime( &timestamp );
	snprintf( ret, sizeof( ret )-1, "%2d.%02d.%02d %2d:%02d", tim->tm_mday, tim->tm_mon+1, tim->tm_year-100, tim->tm_hour, tim->tm_min );
	return ret;
}

#if defined __linux__

char *sha1sum( char *data, int data_len )
{
	static char out[(SHS_DIGESTSIZE*2)+1];
	static const char *hex = "0123456789abcdef";
	char sum[SHS_DIGESTSIZE];
	SHA_CTX ctx;
	int i, j;

	SHAInit( &ctx );
	SHAUpdate( &ctx, data, data_len );
	SHAFinal( sum, &ctx );

	for( i=0,j=0; j<SHS_DIGESTSIZE;)
	{
		out[i++] = hex[(sum[j]>>4)&0xf];
		out[i++] = hex[sum[j++]&0xf];
	}
	out[i] = 0;
	return out;
}

#endif

/*
** 0.F - Log
*/

void Log (edict_t *ent, int event, char *optional)
{
	FILE *f;
	cvar_t	*basedir, *gamedir;
	char	filename[256];
	time_t rawtime;
	struct tm * timeinfo;

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	strcpy(filename,basedir->string);
	if(strlen(gamedir->string))
	{
		strcat(filename,"/");
		strcat(filename,gamedir->string);
		strcat(filename,"/");
	}
	else
		strcat(filename,"/baseq2/");

	switch (event)
	{
	case LOG_CONNECT:
		if (strlen(sv_log_connect->string))
			strcat(filename, sv_log_connect->string);
		else
			return;

		f = fopen(filename, "a");

		if (!f)
			return;

		fprintf(f, "[%.2d:%.2d:%.2d %.2d.%.2d.%.2d] %s (%s) %s.\n", timeinfo->tm_hour, timeinfo->tm_min, 
																	timeinfo->tm_sec, timeinfo->tm_mday, 
																	timeinfo->tm_mon, timeinfo->tm_year+1900,
																	ent->client->pers.netname, IPLongToString(ent->client->pers.save_data.ip), optional);
		fclose(f);
	break;
	case LOG_ADMIN:
		if (strlen(sv_log_admin->string))
			strcat(filename, sv_log_admin->string);
		else
			return;

		f = fopen(filename, "a");

		if (!f)
			return;

		fprintf(f, "[%.2d:%.2d:%.2d %.2d.%.2d.%.2d] %s (%s) used admin command: %s.\n", timeinfo->tm_hour, timeinfo->tm_min, 
																	timeinfo->tm_sec, timeinfo->tm_mday, 
																	timeinfo->tm_mon, timeinfo->tm_year+1900,
																	ent->client->pers.netname, IPLongToString(ent->client->pers.save_data.ip),
																	optional);
		fclose(f);
	break;
	case LOG_CHANGE:
		if (strlen(sv_log_change->string))
			strcat(filename, sv_log_change->string);
		else
			return;

		f = fopen(filename, "a");

		if (!f)
			return;

		fprintf(f, "[%.2d:%.2d:%.2d %.2d.%.2d.%.2d] %s (%s) changed name to %s.\n", timeinfo->tm_hour, timeinfo->tm_min, 
																	timeinfo->tm_sec, timeinfo->tm_mday, 
																	timeinfo->tm_mon, timeinfo->tm_year+1900,
																	ent->client->pers.netname, IPLongToString(ent->client->pers.save_data.ip),
																	optional);
		fclose(f);
	break;
	case LOG_VOTE:
		if (strlen(sv_log_votes->string))
			strcat(filename, sv_log_votes->string);
		else
			return;

		f = fopen(filename, "a");

		if (!f)
			return;

		fprintf(f, "[%.2d:%.2d:%.2d %.2d.%.2d.%.2d] %s (%s) initiated a vote: %s.\n", timeinfo->tm_hour, timeinfo->tm_min, 
																	timeinfo->tm_sec, timeinfo->tm_mday, 
																	timeinfo->tm_mon, timeinfo->tm_year+1900,
																	ent->client->pers.netname, IPLongToString(ent->client->pers.save_data.ip),
																	optional);
		fclose(f);
	break;
	}
}

/*
** 1.A - Say, Talk
*/

struct	long_to_short
{
    char	*short_name;
    char	*long_name;
};

const struct long_to_short weapon_names [11] =
{
	{ "BL", "Blaster" },
	{ "SG", "Shotgun" }, 
	{ "SSG", "Super Shotgun" },
	{ "MG", "Machinegun" },
	{ "CG", "Chaingun" },
	{ "GR", "Grenades" },
	{ "GL", "Grenade Launcher" },
	{ "RL", "Rocket Launcher" },
	{ "HB", "HyperBlaster" },
	{ "RG", "Railgun" },
	{ "BFG", "BFG10K" },
};

void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j, k;
	float	dist;
	float	olddist;
	vec3_t	diff;
	edict_t	*e;
	edict_t	*other;
	char	*p;
	char	text[2048];
	char	out[2048];
	char	format[64];
	gitem_t		*it;
	gitem_t		*ps;
	gclient_t *cl;
	qboolean found;

	if (gi.argc () < 2 && !arg0)
		return;

	if (sv_obsmode->value == 2 && !ent->client->pers.save_data.is_admin && ent->client->pers.save_data.team == TEAM_NONE && (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH))
		return;
	else if (team || (sv_obsmode->value == 1 && !ent->client->pers.save_data.is_admin && ent->client->pers.save_data.team == TEAM_NONE && (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH)))
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if ( ent->client->resp.silenced == true && team == false )
	{
		gi.cprintf( ent, PRINT_HIGH, "You have been muted and can talk only to your teammates." );
		return;
	}

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	for(i=0,j=0; i<strlen(text); i++, j++)
	{
		if (text[i] == '%')
		{
			if (text[i+1] == 'h')
			{
				text[i] = '$';
				text[i+1] = 'h';
			}
			else if (text[i+1] == 'H')
			{
				text[i] = '$';
				text[i+1] = 'H';
			}
			else if (text[i+1] == 'a')
			{
				text[i] = '$';
				text[i+1] = 'a';
			}
			else if (text[i+1] == 'A')
			{
				text[i] = '$';
				text[i+1] = 'A';
			}
			else if (text[i+1] == 'w')
			{
				text[i] = '$';
				text[i+1] = 'w';
			}
			else if (text[i+1] == 'W')
			{
				text[i] = '$';
				text[i+1] = 'W';
			}
			else if (text[i+1] == 'l')
			{
				text[i] = '$';
				text[i+1] = 'l';
			}
			else if (text[i+1] == 'n')
			{
				text[i] = '$';
				text[i+1] = 'n';
			}
			else
				i++;
		}
		out[j] = text[i];
	}
	out[j]='\0';
	strcpy(text,out);

	olddist = 9999.0;

	for(i=0; i<strlen(text); i++)
	{
		if (text[i] == '$')
		{
			if (text[i+1] == 'h')
			{
				text[i] = '%';
				text[i+1] = 's';
				if (ent->health <= 0)
					sprintf(format, "dead");
				else
					sprintf(format, "%dH", ent->health);
				sprintf(out, text, format);
				strcpy(text, out);
			}
			else if (text[i+1] == 'H')
			{
				text[i] = '%';
				text[i+1] = 's';
				if (ent->health <= 0)
					sprintf(format, "dead");
				else
					sprintf(format, "%d health", ent->health);
				sprintf(out, text, format);
				strcpy(text, out);
			}
			else if (text[i+1] == 'a' || text[i+1] == 'A')
			{
				qboolean	fullname=false;
				qboolean	shield=false;
				qboolean	shieldon=false;

				if (ent->flags & FL_POWER_ARMOR)
					shieldon = true;

				if (text[i+1] == 'A')
					fullname = true;

				text[i] = '%';
				text[i+1] = 's';

				ps = FindItem("Power Shield");
				if (ent->client->pers.inventory[ITEM_INDEX(ps)] > 0)
					shield = true;
				if (shield)
					ps = FindItem("Cells");

				it = FindItem("Jacket Armor");
				if (ent->client->pers.inventory[ITEM_INDEX(it)] > 0)
				{
					if (shield && !fullname && shieldon)
						sprintf(format, "%i%s/%i%s", ent->client->pers.inventory[ITEM_INDEX(ps)], "PS", ent->client->pers.inventory[ITEM_INDEX(it)], "JA");
					else if (shield && !fullname)
						sprintf(format, "%s %s/%i%s", "PS", "OFF", ent->client->pers.inventory[ITEM_INDEX(it)], "JA");
					else if (shield && fullname && shieldon)
						sprintf(format, "Power Shield with %i cells and %i units of Jacket Armor", ent->client->pers.inventory[ITEM_INDEX(ps)], ent->client->pers.inventory[ITEM_INDEX(it)]);
					else if (shield && fullname)
						sprintf(format, "Power Shield OFF and %i units of Jacket Armor", ent->client->pers.inventory[ITEM_INDEX(it)]);
					else
						sprintf(format, "%i%s", ent->client->pers.inventory[ITEM_INDEX(it)], fullname ? " units of Jacket Armor" : "JA");
				}
				else
				{
					it = FindItem("Combat Armor");
					if (ent->client->pers.inventory[ITEM_INDEX(it)] > 0)
					{
						if (shield && !fullname && shieldon)
							sprintf(format, "%i%s/%i%s", ent->client->pers.inventory[ITEM_INDEX(ps)], "PS", ent->client->pers.inventory[ITEM_INDEX(it)], "YA");
						else if (shield && !fullname)
							sprintf(format, "%s %s/%i%s", "PS", "OFF", ent->client->pers.inventory[ITEM_INDEX(it)], "YA");
						else if (shield && fullname && shieldon)
							sprintf(format, "Power Shield with %i cells and %i units of Combat Armor", ent->client->pers.inventory[ITEM_INDEX(ps)], ent->client->pers.inventory[ITEM_INDEX(it)]);
						else if (shield && fullname)
							sprintf(format, "Power Shield OFF and %i units of Combat Armor", ent->client->pers.inventory[ITEM_INDEX(it)]);
						else
							sprintf(format, "%i%s", ent->client->pers.inventory[ITEM_INDEX(it)], fullname ? " units of Combat Armor" : "YA");
					}
					else
					{
						it = FindItem("Body Armor");
						if (ent->client->pers.inventory[ITEM_INDEX(it)] > 0)
						{
							if (shield && !fullname && shieldon)
								sprintf(format, "%i%s/%i%s", ent->client->pers.inventory[ITEM_INDEX(ps)], "PS", ent->client->pers.inventory[ITEM_INDEX(it)], "RA");
							else if (shield && !fullname)
								sprintf(format, "%s %s/%i%s", "PS", "OFF", ent->client->pers.inventory[ITEM_INDEX(it)], "RA");
							else if (shield && fullname && shieldon)
								sprintf(format, "Power Shield with %i cells and %i units of Body Armor", ent->client->pers.inventory[ITEM_INDEX(ps)], ent->client->pers.inventory[ITEM_INDEX(it)]);
							else if (shield && fullname)
								sprintf(format, "Power Shield OFF and %i units of Body Armor", ent->client->pers.inventory[ITEM_INDEX(it)]);
							else
								sprintf(format, "%i%s", ent->client->pers.inventory[ITEM_INDEX(it)], fullname ? " units of Body Armor" : "RA");
						}
						else if (shield && !fullname && shieldon)
							sprintf(format, "%i%s", ent->client->pers.inventory[ITEM_INDEX(ps)], "PS");
						else if (shield && !fullname)
							sprintf(format, "%s %s", "PS", "OFF");
						else if (shield && fullname && shieldon)
							sprintf(format, "Power Shield with %i cells", ent->client->pers.inventory[ITEM_INDEX(ps)]);
						else if (shield && fullname)
							sprintf(format, "Power Shield OFF");
						else
							sprintf(format, "%s", fullname ? "no armor" : "0A");
					}
				}
				sprintf(out, text, format);
				strcpy(text, out);
			}
			else if (text[i+1] == 'W')
			{
				text[i] = '%';
				text[i+1] = 's';
				
				if (ent->health > 0)
					sprintf(out, text, ent->client->pers.weapon->pickup_name);
				else
					sprintf(out, text, "no weapon");
				strcpy(text, out);
			}
			else if (text[i+1] == 'w')
			{
				text[i] = '%';
				text[i+1] = 's';

				if (ent->health > 0)
				{
					for(k=0; k < 11; k++)
					{
						if (!strcmp(ent->client->pers.weapon->pickup_name, weapon_names[k].long_name))
							break;
					}

					sprintf(out, text, weapon_names[k].short_name);
				}
				else
					sprintf(out, text, "no weapon");
				strcpy(text, out);
			}
			else if (text[i+1] == 'l')
			{
				text[i] = '%';
				text[i+1] = 's';

				found = false;

				for (k=1, e=g_edicts+k; k < globals.num_edicts; k++,e++)
				{
					if (!e->item)
						continue;
					if (e->spawnflags & DROPPED_ITEM)
						continue;		

					if (e->item->flags & IT_WEAPON || e->item->flags & IT_ARMOR || e->item->flags & IT_POWERUP || !e->item->flags)
					{
						if (e->item->flags & IT_AMMO)
							continue;
						if (!strcmp(e->item->pickup_name, "Armor Shard"))
							continue;

						if (!strcmp(e->item->pickup_name, "Health"))
							continue;

						VectorSubtract(ent->s.origin, e->s.origin, diff);
						dist = VectorLength(diff);

						if (dist < olddist && gi.inPVS(ent->s.origin, e->s.origin))
						{
							sprintf(format, "near the %s", e->item->pickup_name);
							found = true;
							olddist = dist;
						}
					}
				}

				if (found)
				{
					sprintf(out, text, format);
					strcpy(text, out);
				}
				else
				{
					gi.dprintf("BUG: %s at %s can't see any item.\n", ent->client->pers.netname, vtos(ent->s.origin));
					sprintf(out, text, "");
					strcpy(text, out);
				}
			}
			else if (text[i+1] == 'n')
			{
				vec3_t	diff_vec;
				float	dist=9999;
				int		closecl=-1;

				text[i] = '%';
				text[i+1] = 's';

				for (k=0 ; k<game.maxclients ; k++)
				{
					other = &g_edicts[1+k];
					if (!other->inuse)
						continue;

					if (other == ent)
						continue;

					if (other->client->pers.save_data.team == ent->client->pers.save_data.team)
					{
						VectorSubtract(ent->s.origin, other->s.origin, diff_vec);
						VectorNormalize(diff_vec);
						if (dist > VectorLength(diff_vec))
						{
							dist = VectorLength(diff_vec);
							closecl = k;
						}
					}
				}
				if (closecl >= 0)
				{
					for (k=0 ; k<game.maxclients ; k++)
					{
						other = &g_edicts[1+k];
						if (!other->inuse)
							continue;

						if (other == ent)
							continue;

						if (k == closecl)
						{
							sprintf(out, text, other->client->pers.netname);
							break;
						}
					}
				}
				else if (closecl == -1)
					sprintf(out, text, "no one");
				strcpy(text, out);
			}
		}
	}

	strcat(text, "\n");

	if (flood_msgs->value && !(level.paused) && !(team))
	{
		cl = ent->client;

		if (level.time < cl->flood_locktill)
		{
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds.\n", (int)(cl->flood_locktill - level.time));
				return;
		}
		i = cl->flood_whenhead - flood_msgs->value + 1;
			if (i < 0)
				i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
					if (cl->flood_when[i] && 
							level.time - cl->flood_when[i] < GAMESECONDS(flood_persecond->value)) {
						cl->flood_locktill = level.time + GAMESECONDS(flood_waitdelay->value);
						gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
								(int)flood_waitdelay->value);
						return;
					}
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (!team)
	{
		if (!sv_obsmode->value || (level.match_state != MATCH && level.match_state != OVERTIME && level.match_state != SUDDENDEATH) || ent->client->pers.save_data.is_admin || ent->client->pers.save_data.team != TEAM_NONE)
		{
			gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_GREEN, "%s", text);
			return;
		}
		else if (dedicated->value)
			gi.cprintf(NULL, PRINT_CHAT, "%s", text);
	}
	else if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team || (sv_obsmode->value == 1 && !ent->client->pers.save_data.is_admin && ent->client->pers.save_data.team == TEAM_NONE && (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH)))
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
	if (team || (sv_obsmode->value == 1 && !ent->client->pers.save_data.is_admin && ent->client->pers.save_data.team == TEAM_NONE && (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH)))
		gi.cprintf(ent, PRINT_CHAT, "%s", text);
}

void Cmd_Talk_f(edict_t *ent)
{
	char	*p;
	int		i,j;
	char	*q;
	edict_t *dest=NULL;
	char	text[2048];

	if (gi.argc() < 2)
	{
		Cmd_PlayerList_f(ent);
		gi.cprintf(ent, PRINT_HIGH, "USAGE: talk <player name or ID> <message>\n");
		return;
	}
	p = gi.argv(1);

	dest = find_client(ent, p);

	if (!dest)
	{
		gi.cprintf(ent, PRINT_HIGH, "Player not found.\n");
		return;
	}
	if (dest == ent)
	{
		gi.cprintf(ent, PRINT_HIGH, "Are you freak??\n");
		return;
	}
	if ((level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH) && dest->client->pers.save_data.team == TEAM_NONE && ent->client->pers.save_data.team != TEAM_NONE && !ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't talk with spectators.\n");
		return;
	}

	if ((level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH) && ent->client->pers.save_data.team == TEAM_NONE && dest->client->pers.save_data.team != TEAM_NONE && !ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't talk with players.\n");
		return;
	}

	q = gi.args();

	if (*q == '"')
	{
		q++;
		q[strlen(q)-1] = 0;
	}

	for(i=0, j=0; i<strlen(q); i++)
	{
		if (j > 2047)
			break;
		if (i > strlen(p))
		{
			text[j] = q[i];
			j++;
		}
	}
	text[j] = '\0';
	gi.cprintf(dest, PRINT_CHAT, "{%s}: %s\n", ent->client->pers.netname, text);
	gi.cprintf(ent, PRINT_CHAT, "{%s}: %s\n", ent->client->pers.netname, text);
}

/*
 ** 1.B - Ban
 */

void CheckBans(void)
{
	if (game.banToCheck)
	{
		if (game.banToCheck->whenRemove < game.servertime && game.banToCheck->whenRemove > 0)
		{
			UNLINK(game.banToCheck, game.ban_first, game.ban_last, next, prev);
			free(game.banToCheck);
			game.banToCheck = NULL;
			game.banToCheck = game.ban_first;
		}
		else if (game.banToCheck->next)
			game.banToCheck = game.banToCheck->next;
		else
			game.banToCheck = game.ban_first;
	}
}

qboolean Is_Baned(unsigned long ip)
{
	struct ban_s *check;
	char IPString[20];
	int	i;

	Com_sprintf(IPString, 20, "%d.%d.%d.%d", ip&0xFF, (ip>>8)&0xFF, (ip>>16)&0xFF, (ip>>24)&0xFF);

	for (check = game.ban_first; check; check = check->next)
	{
		if (!check->ip)
			continue;
		if (!strcmp(IPString, check->ip))
			return true;
		else
		{
			for (i=0; i<strlen(check->ip); i++)
			{
				if (i > strlen(IPString))
					break;
				if (check->ip[i] != IPString[i])
					break;
				if (check->ip[i] == '*')
					return true;
			}
		}
	}
	return false;
}

void WriteBanList(void)
{
	cvar_t	*basedir, *gamedir;
	char	filename[256];
	FILE	*f;
	int		i=0;
	char string[MAX_STRING_CHARS];
	struct ban_s *toWrite;

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);

	strcpy(filename,basedir->string);

	if(strlen(gamedir->string))
	{
		strcat(filename,"/");
		strcat(filename,gamedir->string);
		strcat(filename,"/ban");
		strcat(filename,port->string);
		strcat(filename,".lst");
	}
	else
	{
		strcat(filename,"/baseq2/ban");
		strcat(filename,port->string);
		strcat(filename,".lst");
	}

	f = fopen(filename,"w");

	if(!f)
	{
		gi.dprintf("Can't create %s!\n", filename);
		return;
	}
	else
	{
		for(toWrite = game.ban_first; toWrite; toWrite = toWrite->next)
		{
			if (strcmp(toWrite->ip, ""))
			{
				sprintf(string, "%s %s %s %ld %ld\n", toWrite->ip, toWrite->nick, toWrite->giver, toWrite->whenBaned, toWrite->whenRemove);
				fputs(string, f);
			}
		}
		fclose(f);
	}
}

void LoadBanList(void)
{
	cvar_t *basedir, *gamedir;
	char filename[256];
	FILE *f=NULL;
	int num=0, len;
	char *line;
	char *dayName;
	struct ban_s *newBan;

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);

	game.ban_first = NULL;
	game.ban_last = NULL;

	strcpy(filename,basedir->string);
	len = strlen(gamedir->string);
	if(len)
	{
		strcat(filename,"/");
		strcat(filename,gamedir->string);
		strcat(filename,"/ban");
		strcat(filename,port->string);
		strcat(filename,".lst");
		gi.dprintf("Loading ban list from %s...", filename);
		f = fopen(filename,"r");
		if (!f)
			gi.dprintf("file not found!\nTrying to load from ");
	}

	if (!f || !len)
	{
		strcpy(filename,basedir->string);
		strcat(filename,"/baseq2/ban");
		strcat(filename,port->string);
		strcat(filename,".lst");
		gi.dprintf("%s...", filename);

		f = fopen(filename,"r");
		if (!f)
		{
			gi.dprintf("File not found!\n");
			return;
		}
	}

	if (f)
	{
		do
		{
			line = read_line(f);
			if (!line)
				break;

			newBan = (struct ban_s*)malloc(sizeof(struct ban_s));
			if (!newBan)
				break;

			strcpy(newBan->ip, read_word(line, 0));
			strcpy(newBan->nick, read_word(line, 1));
			strcpy(newBan->giver, read_word(line, 2));
			if (strlen(newBan->giver))
				num++;
			dayName = read_word(line, 3);
			if (atoi(dayName))
			{
				newBan->whenBaned = atoi(dayName);
				newBan->whenRemove = atoi(read_word(line, 4));
			}
			else //for backward compatibility
			{
				newBan->whenBaned = game.servertime;
				newBan->whenRemove = -1;
			}
			LINK(newBan, game.ban_first, game.ban_last, next, prev);
		} while (!feof(f));
		fclose(f);
	}
	game.banToCheck = game.ban_first;
	gi.dprintf("Done (%d ban%s)\n", num, num > 1 ? "s" : "");
}

void Cmd_Unban_f(edict_t *ent)
{
	char	*p;
	qboolean	deleted=false;
	struct ban_s *unbanIP;

	if (ent)
	{
		if (!ent->client->pers.save_data.is_admin)
		{
			gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
			return;
		}
		else if (!(ent->client->pers.save_data.admin_flags & AD_BAN) && !(ent->client->pers.save_data.admin_flags & AD_ALL))
		{
			gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
			return;
		}
		if (ent->client->pers.save_data.judge)
		{
			gi.cprintf(ent, PRINT_HIGH, "Judges can't use this command.\n");
			return;
		}
	}

	if (gi.argc() < 2 || (!ent && gi.argc() < 3))
	{
		gi.cprintf(ent, PRINT_HIGH, "USAGE: unban <IP>.\n");
		return;
	}

	if (ent)
		p = gi.argv(1);
	else
		p = gi.argv(2);

	for(unbanIP = game.ban_first; unbanIP; unbanIP = unbanIP->next)
	{
		if (!strcmp(unbanIP->ip, p))
		{
			UNLINK(unbanIP, game.ban_first, game.ban_last, next, prev);
			free(unbanIP);
			deleted = true;
			break;
		}
	}
	if (!deleted)
		gi.cprintf(ent, PRINT_HIGH, "%s is not banned.\n",p);
	else
	{
		gi.cprintf(ent, PRINT_HIGH, "%s unbanned.\n",p);
		if (ent)
			Log(ent, LOG_ADMIN, "unban");
		WriteBanList();
		game.banToCheck = game.ban_first;
	}
}

void Cmd_Ban_f(edict_t *ent, unsigned long ip, char *forTime)
{
	qboolean	found_p=false;
	char	*p=NULL, *q=NULL, *ipCheck=NULL;
	edict_t	*cl=NULL;
	struct ban_s *newBan;
	int	i;

	if (!ip)
	{
		if (ent)
		{
			if (!ent->client->pers.save_data.is_admin)
			{
				gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
				return;
			}
			else if (!(ent->client->pers.save_data.admin_flags & AD_BAN) && !(ent->client->pers.save_data.admin_flags & AD_ALL))
			{
				gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
				return;
			}
			if (ent->client->pers.save_data.judge)
			{
				gi.cprintf(ent, PRINT_HIGH, "Judges can't use this command.\n");
				return;
			}
		}
		if (gi.argc() < 2 || (!ent && gi.argc() < 3))
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: ban <IP> (<time[m|h|d]>).\nExaple: \"ban 127.0.0.1 25m\" - 127.0.0.1 banned for 25 minutes.\n");
			return;
		}

		if (ent)
			p = gi.argv(1);
		else
			p = gi.argv(2);

		ipCheck = strdup(p);
		if (ipCheck)
		{
			i = 0;
			ipCheck = strtok(ipCheck, ".");
			while(ipCheck)
			{
				ipCheck = strtok(NULL, ".");
				i++;
			}
			free(ipCheck);
			if (i < 4)
			{
				gi.cprintf(ent, PRINT_HIGH, "Bad IP address format.\n");
				return;
			}
		}
		else
			return;

		if (gi.argc() >= 3 && ent)
			q = gi.argv(2);
		else if (gi.argc() >= 4 && !ent)
			q = gi.argv(3);
	}
	else
	{
		p = IPLongToString(ip);
		q = forTime;
	}

	for(i=0; i<game.maxclients; i++)
	{
		cl = &g_edicts[1+i];

		if (!cl->inuse)
			continue;

		if (cl->client->pers.save_data.ip == ip)
		{
			found_p = true;
			break;
		}
	}

	for(newBan = game.ban_first; newBan; newBan = newBan->next)
	{
		if (!strcmp(newBan->ip, p))
		{
			if (!ip)
				gi.cprintf(ent, PRINT_HIGH, "%s is already banned.\n", p);
			return;
		}
	}

	newBan = (struct ban_s*)malloc(sizeof(struct ban_s));
	if (!newBan)
	{
		gi.cprintf(ent, PRINT_HIGH, "Error while allocating new ban structure.\n");
		return;
	}

	if (found_p)
		strcpy(newBan->nick, cl->client->pers.netname);
	else
		strcpy(newBan->nick, "unspecified");

	strcpy(newBan->ip, p);

	if (ent)
		strcpy(newBan->giver, ent->client->pers.netname);
	else
		strcpy(newBan->giver, "CONSOLE");

	time (&newBan->whenBaned);
	if (q)
	{
		char *bannedFor, *qCopy;
		qCopy = strdup(q);
		if (qCopy)
		{
			bannedFor = strtok(qCopy, "mhd");
			if (!bannedFor)
			{
				newBan->whenRemove = atoi(q);
				if (!newBan->whenRemove)
					newBan->whenRemove = -1;
			}
			else if (atoi(bannedFor) > 0)
			{
				if (q[strlen(q)-1] == 'h')
					newBan->whenRemove = atoi(bannedFor)*3600;
				else if (q[strlen(q)-1] == 'd')
					newBan->whenRemove = atoi(bannedFor)*86400;
				else
					newBan->whenRemove = atoi(bannedFor)*60;
				newBan->whenRemove += newBan->whenBaned;
			}
			else
				newBan->whenRemove = -1;
			free(qCopy);
		}
		else
			newBan->whenRemove = -1;
	}
	else
		newBan->whenRemove = -1;
	LINK(newBan, game.ban_first, game.ban_last, next, prev);
	game.banToCheck = game.ban_first;

	if (!ip)
		gi.cprintf(ent, PRINT_HIGH, "You have banned %s.\n", newBan->ip);

	if (ent)
		Log(ent, LOG_ADMIN, "ban");

	WriteBanList();
}

void Cmd_Kickban_f(edict_t *ent)
{
	char		*p, *q=NULL;
	edict_t		*cl=NULL;
	char		text[80];
	int client_num = -1;

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else if (!(ent->client->pers.save_data.admin_flags & AD_BAN) && !(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}
	if (gi.argc() < 2)
	{
		gi.cprintf(ent, PRINT_HIGH, "USAGE: kickban <player name or ID> (<time[m|h|d]>).\n");
		return;
	}

	p = gi.argv(1);

	cl = find_client(ent, p);

	if (cl && cl == ent)
	{
		gi.cprintf(ent, PRINT_HIGH, "Do you realy wanna kick and ban yourself?\n");
		return;
	}
	else if (cl && ( client_num = get_client_num_from_edict( cl ) ) >= 0 )
	{
		if (cl->client->pers.save_data.is_admin && !cl->client->pers.save_data.judge && ent->client->pers.save_data.judge)
		{
			gi.cprintf(ent, PRINT_HIGH, "You can't kickban an admin!\n");
			return;
		}
		if (gi.argc() >= 3)
			q = gi.argv(2);
		if (ent->client->pers.save_data.judge) //referee can ban only for timelimit minutes
		{
			Com_sprintf(text, sizeof(text), "%dm", (int)timelimit->value);
			q = text;
		}
		Cmd_Ban_f(ent, cl->client->pers.save_data.ip, q);

		sprintf(text, "kick %d\n", client_num);
		gi.AddCommandString(text);
		Log(ent, LOG_ADMIN, "kickban");
	}
	else
		gi.cprintf(ent, PRINT_HIGH, "Player not found.\n");
}

void Cmd_Banlist_f(edict_t *ent)
{
	struct ban_s *list;
	char *p=NULL;

	if (ent)
	{
		if (!ent->client->pers.save_data.is_admin)
		{
			gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
			return;
		}
		else if (!(ent->client->pers.save_data.admin_flags & AD_BAN) && !(ent->client->pers.save_data.admin_flags & AD_ALL))
		{
			gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
			return;
		}
		if (ent->client->pers.save_data.judge)
		{
			gi.cprintf(ent, PRINT_HIGH, "Judges can't use this command.\n");
			return;
		}
	}

	if (gi.argc() >= 2 && ent)
		p = gi.argv(1);
	else if (gi.argc() >= 3 && !ent)
		p = gi.argv(2);

	if (ent)
	{
		Log(ent, LOG_ADMIN, "banlist");
		AddToLongCPrintf(ent, "Ban list:\n------ IP ------+------ Name ------+----- From -----+------ To ------\n");
	}
	else
		gi.cprintf(NULL, PRINT_HIGH, "Ban list:\n----------------\n");

	for(list = game.ban_first; list; list = list->next)
	{
		if (p)
		{
			if (!strncmp(p, list->ip, strlen(p)))
			{
				if (ent)
				{
					AddToLongCPrintf(ent, "%-15s | %-16s | %s | ", list->ip, list->nick, format_time( list->whenBaned) );
					AddToLongCPrintf(ent, "%s\n", list->whenRemove <= 0 ? "unspecified" : format_time( list->whenRemove ));
				}
				else
				{
					gi.cprintf(NULL, PRINT_HIGH, "   IP: %s\n Nick: %s\nAdmin: %s\nSince: %s", list->ip, list->nick, list->giver, format_time( list->whenBaned ));
					gi.cprintf(NULL, PRINT_HIGH, "   To: %s----------------\n", list->whenRemove <= 0 ? "unspecified\n" : format_time( list->whenRemove ));
				}
			}
		}
		else
		{
			if (ent)
			{
				AddToLongCPrintf(ent, "%-15s | %-16s | %s | ", list->ip, list->nick, format_time( list->whenBaned ));
				AddToLongCPrintf(ent, "%s\n", list->whenRemove <= 0 ? "unspecified" : format_time( list->whenRemove ));
			}
			else
			{
				gi.cprintf(NULL, PRINT_HIGH, "   IP: %s\n Nick: %s\nAdmin: %s\nSince: %s", list->ip, list->nick, list->giver, format_time( list->whenBaned ));
				gi.cprintf(NULL, PRINT_HIGH, "   To: %s----------------\n", list->whenRemove <= 0 ? "unspecified\n" : format_time( list->whenRemove ));
			}
		}
	}
}

void Cmd_IsBanned_f(edict_t *ent)
{
	struct ban_s *list;
	char *p;

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else if (!(ent->client->pers.save_data.admin_flags & AD_BAN) && !(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}
	if (ent->client->pers.save_data.judge)
	{
		gi.cprintf(ent, PRINT_HIGH, "Judges can't use this command.\n");
		return;
	}

	if (gi.argc() < 2)
	{
		gi.cprintf(ent, PRINT_HIGH, "USAGE: isbanned <IP>.\n");
		return;
	}

	p = gi.argv(1);

	Log(ent, LOG_ADMIN, "isbanned");

	for(list = game.ban_first; list; list = list->next)
	{
		if (!strcmp(p, list->ip))
		{
			gi.cprintf(ent, PRINT_HIGH, "   IP: %s\n Nick: %s\nAdmin: %s\nSince: %s", list->ip, list->nick, list->giver, ctime(&list->whenBaned));
			gi.cprintf(ent, PRINT_HIGH, "   To: %s----------------\n", list->whenRemove <= 0 ? "unspecified\n" : ctime(&list->whenRemove));
			return;
		}
	}
	gi.cprintf(ent, PRINT_HIGH, "%s is not banned.\n", p);
}
/*
 ** 1.C - Voting
 */

void Cmd_MapList_f(edict_t *ent)
{
	cvar_t	*basedir, *gamedir;
	char	filename[256];
	FILE	*f=NULL;

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
		f = fopen(filename,"r");
	}

	if(!f)
	{
		gi.cprintf(ent, PRINT_HIGH, "%s not found!\n", sv_maplist->string);
		return;
	}
	else
	{
		char *line=NULL;
		int i=1;
		do
		{
			line = read_line(f);
			if (!line)
				break;
			if (ent)
				AddToLongCPrintf(ent, "%3d: %s\n", i, line);
			else
				gi.cprintf(ent, PRINT_HIGH, "%3d: %s\n", i, line);
			i++;
		} while(!feof(f));
		fclose(f);
	}
}

char *IsValidMap(char *mapname, qboolean mapNum)
{
	cvar_t	*basedir, *gamedir;
	char	filename[256];
	FILE	*f=NULL;
	int thisMap;

	if (mapNum)
		thisMap = atoi(mapname);
	else
		thisMap = 0;

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
		f = fopen(filename,"r");

	}

	if(!f)
		return NULL;
	else
	{
		char *line=NULL;
		int i=1;
		do
		{
			line = read_line(f);
			if (!line)
				break;
			if (mapNum && i == thisMap)
			{
				fclose( f );
				return read_word(line, 0);
			}
			else
			{
				char *tempPtr = read_word(line, 0);
				if (tempPtr)
				{
					if (!strcmp(mapname, tempPtr))
					{
						fclose( f );
						return tempPtr;
					}
				}
			}

			i++;
		} while(!feof(f));
		fclose(f);
	}
	return NULL;
}

qboolean CheckSvConfigList (char *search)
{
	char	configs[1024];
	char	*point = configs;
	char	*token, copy[1024];
	qboolean	found = false;

	if (!strlen(sv_configlist->string))
		return found;

	strncpy(configs, sv_configlist->string, 1024);

	do
	{
		if (!point)
			break;

		strcpy(copy, point);

		token = strtok (copy, " ");
		if (token)
		{
			if (!strcmp(search, token))
			{
				found = true;
				break;
			}
		}
		else
			break;

		point = strstr(point, " ");
		if (point)
		{
			point++;
			if (!point)
				break;
		}
	} while(token);

	return found;
}

void Cmd_ConfigList_f (edict_t *ent)
{
	char	configs[1024];
	char	*point = configs;
	char	*token, copy[1024];
	qboolean	found = false;

	if (!strlen(sv_configlist->string))
	{
		gi.cprintf(ent, PRINT_HIGH, "Config list is empty.\n");
		return;
	}

	strncpy(configs, sv_configlist->string, 1024);

	MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Available configs are:\n");

	do
	{
		if (!point)
			break;

		strcpy(copy, point);

		token = strtok (copy, " ");
		if (token)
		{
			found = true;
			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "%s\n", token);
		}
		else
			break;

		point = strstr(point, " ");
		if (point)
		{
			point++;
			if (!point)
				break;
		}
	} while(token);

	if (!found)
		MyCPrintf(ent, CPRINTF, PRINT_HIGH, "None.\n");
	MyCPrintfEnd(ent, CPRINTF, PRINT_HIGH);
}

char *GetConfigFromList (int conf_num)
{
	char	configs[1024];
	char	*point = configs;
	char	*token, copy[1024];
	int		i=0;

	if (!strlen(sv_configlist->string))
		return NULL;

	strncpy(configs, sv_configlist->string, 1024);

	do
	{
		if (!point)
			break;

		strcpy(copy, point);

		token = strtok (copy, " ");
		if (token)
		{
			if (i == conf_num)
				return strdup(token);
			i++;
		}
		else
			break;

		point = strstr(point, " ");
		if (point)
		{
			point++;
			if (!point)
				break;
		}
	} while(token);

	return NULL;
}

void Cmd_Dmf_f (edict_t *ent)
{
	int	i;
	char	*p;
	qboolean	normal=true;

	if (ent->client->pers.save_data.is_admin && ((ent->client->pers.save_data.admin_flags & AD_SERVER) || (ent->client->pers.save_data.admin_flags & AD_ALL)))
	{
		if (gi.argc() < 2)
			normal = true;
		else
		{
			p = gi.argv(1);
			if (atoi(p) <= 0 && atoi(p) > 65535)
			{
				gi.cprintf(ent, PRINT_HIGH, "Invalid dmflags.\n");
				return;
			}
			else
			{
				Log(ent, LOG_ADMIN, "dmf");
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Dmflags changed to %s by %s.\n", p, ent->client->pers.netname);
				gi.cvar_set("dmflags", p);
				return;
			}
		}
	}

	if (normal)
	{
		for(i=1; i<NUM_DMFLAGS; i++)
		{
			if ((int)dmflags->value & dmflags_table[i].flags)
				MyCPrintf(ent, CPRINTF, PRINT_HIGH, "%-17s %5i: ENABLED\n", dmflags_table[i].name, (int)dmflags_table[i].flags);
			else
				MyCPrintf(ent, CPRINTF, PRINT_HIGH, "%-17s %5i: DISABLED\n", dmflags_table[i].name, (int)dmflags_table[i].flags);
		}
		MyCPrintfEnd(ent, CPRINTF, PRINT_HIGH);
	}
}

void CancelVote (edict_t *ent, qboolean subject_quit)
{
	int		i;
	edict_t	*cl;

	if (level.vote.voter || subject_quit)
	{
		if (subject_quit || level.vote.voter == ent || ent->client->pers.save_data.is_admin)
		{
			for (i=0 ; i<game.maxclients ; i++)
			{
				cl = &g_edicts[1+i];
				if (!cl->inuse)
					continue;

				cl->client->pers.save_data.vote_yon = VOTE_NONE;
				cl->client->pers.save_data.vote_change_count = 0;
			}

			if ( subject_quit )
				gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "Vote canceled because %s has quit.\n", g_edicts[1+level.vote.kick].client->pers.netname );
			else
				gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "Vote canceled by %s.\n", ent->client->pers.netname);
			level.vote.voter = NULL;
			level.vote.vote_active = false;
			level.vote.vote_what = 0;
			level.vote.vote_no = 0;
			level.vote.vote_yes = 0;
		}
	}
	else
		gi.cprintf(ent, PRINT_HIGH, "You can't do that.\n");
}

qboolean CanVote (edict_t *ent, char *what)
{
	if ((!allow_vote_kick->value && !strcmp(what, "kickuser"))
			|| (!allow_vote_dmf->value && (!strcmp(what, "dmf") || !strcmp(what, "dmflags")))
			|| (!allow_vote_tl->value && (!strcmp(what, "timelimit") || !strcmp(what, "tl")))
			|| (!allow_vote_bfg->value && !strcmp(what, "bfg"))
			|| (!allow_vote_powerups->value && !strcmp(what, "powerups"))
			|| (!allow_vote_fastweapons->value && !strcmp(what, "fastweapons"))
			|| (!allow_vote_map->value && !strcmp(what, "map"))
			|| (!allow_vote_tp->value && !strcmp(what, "tp"))
			|| (!allow_vote_hud->value && !strcmp(what, "hud"))
			|| (!allow_vote_hand3->value && !strcmp(what, "hand3")))
	{
		gi.cprintf(ent, PRINT_HIGH, "Vote %s is locked.\n", what);
		return false;
	}
	return true;
}

void Cmd_Vote_f (edict_t *ent)
{
	char	*p;
	char	*what=NULL;
	char	*option=NULL;
	edict_t *cl;
	int i;

	if ( ent->client->pers.mvdspec )
		return;
	
	if (level.vote.vote_active)
	{
		if (gi.argc() == 2)
		{
			if (!strcmp(gi.argv(1), "yes"))
			{
				Cmd_Vote_Yes_f (ent);
				return;
			}
			else if (!strcmp(gi.argv(1), "no"))
			{
				Cmd_Vote_No_f (ent);
				return;
			}
			else if (!strcmp(gi.argv(1), "cancel"))
			{
				CancelVote(ent,false);
				return;
			}
		}

		MyCPrintfInit(ent);
		MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Vote already in progress.\nProposal:\n");

		if (level.vote.vote_what & VOTE_KICK)
		{
			if ( g_edicts[1+level.vote.kick].client )
				MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Kick client %d (%s).\n", level.vote.kick, g_edicts[1+level.vote.kick].client->pers.netname);
		}
		if (level.vote.vote_what & VOTE_TIMELIMIT)
			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Change timelimit to %d.\n", level.vote.timelimit);
		if (level.vote.vote_what & VOTE_CONFIG)
			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Execute config: %s.\n", level.vote.config);
		if (level.vote.vote_what & VOTE_DMFLAGS)
		{
			qboolean	isvalidflag=false;

			for (i=1; i<NUM_DMFLAGS; i++)
			{
				if (dmflags_table[i].flags == level.vote.dmflags)
				{
					isvalidflag = true;
					break;
				}
			}
			if (isvalidflag)
			{
				if ((int)dmflags->value & level.vote.dmflags)
					MyCPrintf(ent, CPRINTF, PRINT_HIGH, "DISABLE dmflag %s?\n", dmflags_table[i].name);
				else
					MyCPrintf(ent, CPRINTF, PRINT_HIGH, "ENABLE dmflag %s?\n", dmflags_table[i].name);
			}
			else
				MyCPrintf(ent, CPRINTF, PRINT_HIGH, "ENABLE dmflag %d?\n", level.vote.dmflags);
		}
		if (level.vote.vote_what & VOTE_MAP)
			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Change the map to %s.\n", level.vote.map);
		if (level.vote.vote_what & VOTE_TP)
			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Set tp to %d.\n", level.vote.tp);
		if (level.vote.vote_what & VOTE_BFG)
			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Change the bfg to %d.\n", level.vote.bfg);
		if (level.vote.vote_what & VOTE_POWERUPS)
			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Change the powerups to %d.\n", level.vote.powerups);
		if (level.vote.vote_what & VOTE_FASTWEAPONS)
			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Change the fastweapons to %d.\n", level.vote.fastweapons);
		if (level.vote.vote_what & VOTE_HAND3 )
		{
			if ( level.vote.hand3 )
				MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Allow players to enable hand 3 mode.\n");
			else
				MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Disable hand 3 mode.\n");
		}
		if (level.vote.vote_what & VOTE_HUD)
		{
			if (level.vote.hud)
				MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Allow players to enable weapon list in the hud.\n");
			else
				MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Disable weapon list in the hud.\n");
		}
		MyCPrintf(ent, CPRINTF, PRINT_HIGH, "YES [%d] - NO [%d]\n", level.vote.vote_yes, level.vote.vote_no);
		MyCPrintfEnd(ent, CPRINTF, PRINT_HIGH);
		return;
	}

	if ((level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH) && ent->client->pers.save_data.team == TEAM_NONE)
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't vote during the match unless you are playing.\n");
		return;
	}

	if (gi.argc() == 2)
	{
		if (!strcmp(gi.argv(1), "kickuser"))
		{
			gi.cprintf(ent, PRINT_HIGH, "\n");
			Cmd_PlayerList_f(ent);
			gi.cprintf(ent, PRINT_HIGH, "\nUSAGE: vote kickuser <player name or ID>.\n");
			return;
		}
		else if (!strcmp(gi.argv(1), "map"))
		{
			gi.cprintf(ent, PRINT_HIGH, "\n");
			Cmd_MapList_f(ent);
			gi.cprintf(ent, PRINT_HIGH, "\nUSAGE: vote map <map name> or vote map <#map number>.\n");
			return;
		}
		else if (!strcmp(gi.argv(1), "powerups"))
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote powerups <0 or 1>.\n");
			return;
		}
		else if (!strcmp(gi.argv(1), "tl") || !strcmp(gi.argv(1), "timelimit"))
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote timelimit <time in minutes>.\n");
			return;
		}
		else if (!strcmp(gi.argv(1), "bfg"))
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote bfg <0 or 1>.\n");
			return;
		}
		else if (!strcmp(gi.argv(1), "fastweapons"))
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote fastweapons <0 or 1>.\n");
			return;
		}
		else if (!strcmp(gi.argv(1), "dmf") || !strcmp(gi.argv(1), "dmflags"))
		{
			Cmd_Dmf_f (ent);
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote dmflags <dmflags>.\n");
			return;
		}
		else if (!strcmp(gi.argv(1), "config"))
		{
			Cmd_ConfigList_f (ent);
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote config <filename>.\n");
			return;
		}
		else if (!strcmp(gi.argv(1), "tp"))
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote tp <1|2|3|4>.\n");
			return;
		}
		else if (!strcmp(gi.argv(1), "hud"))
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote hud <0 or 1>.\n");
			return;
		}
		else if (!strcmp(gi.argv(1), "hand3"))
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote hand3 <0 or 1>.\n");
			return;
		}
		else
		{
			gi.cprintf(ent, PRINT_HIGH, "vote: no such option.\n");
			return;
		}
	}

	if (gi.argc () < 3)
	{
		gi.cprintf(ent, PRINT_HIGH, "USAGE: vote <option> <value>.\nOptions are:\nmap <mapname>\nkickuser <playerID>\ntimelimit <time in minutes>\nbfg <1/0>\npowerups <1/0>\ndmflags <dmflags>\nfastweapons <1/0>\nconfig <filename>\ntp <1|2|3|4>\nhud <1/0>\nhand3 <1/0>\n");
		return;
	}

	p = gi.argv(1);

	what = (char *)malloc(strlen(p)+1);
	strcpy(what, p);

	p = gi.argv(2);

	option = (char *)malloc(strlen(p)+1);
	strcpy(option, p);

	for(i=0; i<strlen(what); i++)
		what[i] = LOWER(what[i]);
	what[i] = '\0';

	if (strcmp(what, "config"))
	{
		for(i=0; i<strlen(option); i++)
			option[i] = LOWER(option[i]);
		option[i] = '\0';
	}

	if (!CanVote(ent, what))
	{
		free(what);
		free(option);
		return;
	}

	if (!strcmp(what, "timelimit") || !strcmp(what, "tl"))
	{
		if (atoi(option) == timelimit->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "No changes. No proposal initiated.\n");
			free(what);
			free(option);
			return;
		}
		if (atoi(option) >= 0 && atoi(option) <= 180)
		{
			level.vote.timelimit = atoi(option);
			level.vote.vote_what = VOTE_TIMELIMIT;
			level.vote.vote_active = true;
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has initiated a vote!\nProposal: Set the timelimit to %d.\n", ent->client->pers.netname, level.vote.timelimit);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "Timelimit can't be higher than 180 and lower than 0.\n");
	}
	else if (!strcmp(what, "map"))
	{
		qboolean mapNum = false;
		char *proposalMapName;
		if (p[0] == '#' && strlen(p) > 1)
		{
			free(option);
			option = strdup(p+1);
			mapNum = true;
		}
		if ( ( proposalMapName = IsValidMap(option, mapNum) ) )
		{
			if (!strcmp(level.mapname, proposalMapName))
			{
				gi.cprintf(ent, PRINT_HIGH, "No changes. No proposal initiated.\n");
				free(what);
				free(option);
				return;
			}
			level.vote.vote_what = VOTE_MAP;
			strcpy(level.vote.map, proposalMapName);
			level.vote.vote_active = true;
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has initiated a vote!\nProposal: Change the map to %s.\n", ent->client->pers.netname,  level.vote.map);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "Map %s%s is not on the map list.\n", mapNum ? "no. " : "", option);
	}
	else if (!strcmp(what, "kickuser"))
	{
		int client_num = -1;
		edict_t *client=NULL;

		client = find_client(ent, option);
		if (client && ( client_num = get_client_num_from_edict( client ) ) >= 0 && client->inuse )
		{
			level.vote.kick = client_num;
			level.vote.vote_what = VOTE_KICK;
			level.vote.vote_active = true;
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has initiated a vote!\nProposal: Kick client %d (%s).\n", ent->client->pers.netname, level.vote.kick, client->client->pers.netname);
		}
		else
		{
			gi.cprintf(ent, PRINT_HIGH, "Player not found.\n");
			Cmd_PlayerList_f(ent);
			free(what);
			free(option);
			return;
		}
	}
	else if (!strcmp(what, "bfg"))
	{
		if (atoi(option) == allow_bfg->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "No changes. No proposal initiated.\n");
			free(option);
			free(what);
			return;
		}
		if (atoi(option) >= 0 && atoi(option) < 2)
		{
			if ((allow_bfg->value == 1 && atoi(option) == 1) || (allow_bfg->value == 0 && atoi(option) == 0))
			{
				gi.cprintf(ent, PRINT_HIGH, "Bfg is already %d.\n", atoi(option));
				free(what);
				free(option);
				return;
			}
			level.vote.bfg = atoi(option);
			level.vote.vote_what = VOTE_BFG;
			level.vote.vote_active = true;
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has initiated a vote!\nProposal: Change the bfg to %d.\n", ent->client->pers.netname, level.vote.bfg);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote bfg 0/1\n");
	}
	else if (!strcmp(what, "powerups"))
	{
		if (atoi(option) == allow_powerups->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "No changes. No proposal initiated.\n");
			free(option);
			free(what);
			return;
		}
		if (atoi(option) >= 0 && atoi(option) < 2)
		{
			if ((allow_powerups->value == 1 && atoi(option) == 1) || (allow_powerups->value == 0 && atoi(option) == 0))
			{
				gi.cprintf(ent, PRINT_HIGH, "Powerups are already %d.\n", atoi(option));
				free(option);
				free(what);
				return;
			}
			level.vote.powerups = atoi(option);
			level.vote.vote_what = VOTE_POWERUPS;
			level.vote.vote_active = true;
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has initiated a vote!\nProposal: Change the powerups to %d.\n", ent->client->pers.netname, level.vote.powerups);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote powerups 0/1.\n");
	}
	else if (!strcmp(what, "config"))
	{
		if (CheckSvConfigList (option))
		{
			level.vote.vote_what = VOTE_CONFIG;
			strcpy(level.vote.config, option);
			level.vote.vote_active = true;
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has initiated a vote!\nProposal: Execute config: %s.\n", ent->client->pers.netname, level.vote.config);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "Invalid config file name.\n");
	}
	else if (!strcmp(what, "dmf") || !strcmp(what, "dmflags"))
	{
		int			flag=atoi(option);
		qboolean	isvalidflag = false;

		for (i=1; i<NUM_DMFLAGS; i++)
		{
			if (flag == dmflags_table[i].flags && flag != 128)
			{
				isvalidflag = true;
				break;
			}
		}

		if (isvalidflag)
		{
			level.vote.dmflags = atoi(option);
			level.vote.vote_what = VOTE_DMFLAGS;
			level.vote.vote_active = true;
			MyCPrintf(ent, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD, "%s has initiated a vote!\n", ent->client->pers.netname);
			if ((int)dmflags->value & flag)
				MyCPrintf(ent, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD, "Proposal: DISABLE dmflag \"%s\".\n", dmflags_table[i].name);
			else
				MyCPrintf(ent, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD, "Proposal: ENABLE dmflag \"%s\".\n", dmflags_table[i].name);
			MyCPrintfEnd(ent, BPRINTF, ATTR_PRINT_HIGH|ATTR_BOLD);
		}
		else
		{
			gi.cprintf(ent, PRINT_HIGH, "Invalid dmflag.\n");
			Cmd_Dmf_f(ent);
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote dmflags <dmflags>.\n");
			free(option);
			free(what);
			return;
		}
	}
	else if (!strcmp(what, "fastweapons"))
	{
		if (atoi(option) == (int)fastweapons->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "No changes. No proposal initiated.\n");
			free(option);
			free(what);
			return;
		}
		if (atoi(option) >= 0 && atoi(option) < 2)
		{
			if ((fastweapons->value == 1 && atoi(option) == 1) || (fastweapons->value == 0 && atoi(option) == 0))
			{
				gi.cprintf(ent, PRINT_HIGH, "Fastweapons is already %d.\n", atoi(option));
				free(option);
				free(what);
				return;
			}
			level.vote.fastweapons = atoi(option);
			level.vote.vote_what = VOTE_FASTWEAPONS;
			level.vote.vote_active = true;
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has initiated a vote!\nProposal: Change the fastweapons to %d.\n", ent->client->pers.netname, level.vote.fastweapons);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote fastweapons 0/1.\n");
	}
	else if (!strcmp(what, "tp"))
	{
		if (atoi(option) == game.tp)
		{
			gi.cprintf(ent, PRINT_HIGH, "No changes. No proposal initiated.\n");
			free(option);
			free(what);
			return;
		}
		if (atoi(option) > 0 && atoi(option) < 5)
		{
			level.vote.tp = atoi(option);
			level.vote.vote_what = VOTE_TP;
			level.vote.vote_active = true;
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has initiated a vote!\nProposal: Set tp to %d.\n", ent->client->pers.netname, level.vote.tp);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "Tp value can't be different than 1, 2, 3 or 4.\n");
	}
	else if (!strcmp(what, "hud"))
	{
		if (atoi(option) == allow_hud->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "No changes. No proposal initiated.\n");
			free(option);
			free(what);
			return;
		}
		if (atoi(option) >= 0 && atoi(option) < 2)
		{
			if ((allow_hud->value == 1 && atoi(option) == 1) || (allow_hud->value == 0 && atoi(option) == 0))
			{
				gi.cprintf(ent, PRINT_HIGH, "Hud is already %d.\n", atoi(option));
				free(option);
				free(what);
				return;
			}
			level.vote.hud = atoi(option);
			level.vote.vote_what = VOTE_HUD;
			level.vote.vote_active = true;
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has initiated a vote!\nProposal: Change the hud to %d.\n", ent->client->pers.netname, level.vote.hud);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote hud 0/1.\n");
	}
	else if (!strcmp(what, "hand3"))
	{
		if (atoi(option) == allow_hand3->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "No changes. No proposal initiated.\n");
			free(option);
			free(what);
			return;
		}
		if (atoi(option) >= 0 && atoi(option) < 2)
		{
			if ((allow_hand3->value == 1 && atoi(option) == 1) || (allow_hand3->value == 0 && atoi(option) == 0))
			{
				gi.cprintf(ent, PRINT_HIGH, "Hand3 is already %d.\n", atoi(option));
				free(option);
				free(what);
				return;
			}
			level.vote.hand3 = atoi(option);
			level.vote.vote_what = VOTE_HAND3;
			level.vote.vote_active = true;
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has initiated a vote!\nProposal: Change the hand3 to %d.\n", ent->client->pers.netname, level.vote.hand3);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "USAGE: vote hand3 0/1.\n");
	}

	if (level.vote.vote_active)
	{
		char	voteoptions[128];

		level.vote.vote_time = level.time + GAMESECONDS(60);
		sprintf(voteoptions, "%s -> %s", what, option);
		Log(ent, LOG_VOTE, voteoptions);
		level.vote.voter = ent;
		level.vote.update_vote = true;	
		ent->client->pers.save_data.vote_yon = VOTE_YES;
		ent->client->pers.save_data.vote_change_count = 1;
		level.vote.vote_no = 0;
		level.vote.vote_yes = 1;
	}

	if (what)
		free(what);
	if (option)
		free(option);
}

void Cmd_Vote_Yes_f (edict_t *ent)
{
	if (!ent->client || ent->client->pers.mvdspec )
		return;

	if (!level.vote.vote_active)
	{
		gi.cprintf(ent, PRINT_HIGH, "There is no vote currently in progress!\n");
		return;
	}

	if (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH)
	{
		if (ent->client->pers.save_data.team == TEAM_NONE)
		{
			gi.cprintf(ent, PRINT_HIGH, "Only players can vote now.\n");
			return;
		}
	}

	if (ent->client->pers.save_data.vote_change_count >= 4)
		gi.cprintf(ent, PRINT_HIGH, "You can change your vote only 3 times.\n");

	if (ent->client->pers.save_data.vote_yon == VOTE_YES)
		gi.cprintf(ent, PRINT_HIGH, "You have already voted \"yes\".\n");
	else if (ent->client->pers.save_data.vote_yon == VOTE_NO)
	{
		ent->client->pers.save_data.vote_yon = VOTE_YES;
		level.vote.vote_no -= 1;
		level.vote.vote_yes += 1;
		ent->client->pers.save_data.vote_change_count++;
		gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "%s changed his vote to \"yes\".\n", ent->client->pers.netname);
		level.vote.update_vote = true;
	}
	else
	{
		ent->client->pers.save_data.vote_yon = VOTE_YES;
		level.vote.vote_yes += 1;
		ent->client->pers.save_data.vote_change_count++;
		gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "%s votes \"yes\".\n", ent->client->pers.netname);
		level.vote.update_vote = true;
	}
}

void Cmd_Vote_No_f (edict_t *ent)
{
	if (!ent->client || ent->client->pers.mvdspec )
		return;

	if (!level.vote.vote_active)
	{
		gi.cprintf(ent, PRINT_HIGH, "There is no vote currently in progress!\n");
		return;
	}

	if (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH)
	{
		if (ent->client->pers.save_data.team == TEAM_NONE)
		{
			gi.cprintf(ent, PRINT_HIGH, "Only players can vote now.\n");
			return;
		}
	}

	if (ent->client->pers.save_data.vote_change_count >= 4)
		gi.cprintf(ent, PRINT_HIGH, "You can change your vote only 3 times.\n");

	if (ent->client->pers.save_data.vote_yon == VOTE_NO)
		gi.cprintf(ent, PRINT_HIGH, "You have already voted \"no\".\n");
	else if (ent->client->pers.save_data.vote_yon == VOTE_YES)
	{
		ent->client->pers.save_data.vote_yon = VOTE_NO;
		level.vote.vote_no += 1;
		level.vote.vote_yes -= 1;
		ent->client->pers.save_data.vote_change_count++;
		gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "%s changed his vote to \"no\".\n", ent->client->pers.netname);
		level.vote.update_vote = true;
	}
	else
	{
		ent->client->pers.save_data.vote_yon = VOTE_NO;
		level.vote.vote_no += 1;
		ent->client->pers.save_data.vote_change_count++;
		gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "%s votes \"no\".\n", ent->client->pers.netname);
		level.vote.update_vote = true;
	}
}

void Cmd_VoteLock_f(edict_t *ent, int lock)
{
	char	*p;
	int		i;

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else if (!(ent->client->pers.save_data.admin_flags & AD_SERVER) && !(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}
	else
	{
		if (gi.argc() < 2)
		{
			if (lock == 0)
				gi.cprintf(ent, PRINT_HIGH, "USAGE: vlock <vote_option>.\n");
			else
				gi.cprintf(ent, PRINT_HIGH, "USAGE: vunlock <vote_option>.\n");
			return;
		}
		p = gi.argv(1);

		for(i=0; i<strlen(p); i++)
			p[i] = LOWER(p[i]);
		p[i]='\0';

		if (lock == 1)
			Log(ent, LOG_ADMIN, "vlock");
		else
			Log(ent, LOG_ADMIN, "unlock");

		if (!strcmp(p, "tl") || !strcmp(p, "timelimit"))
		{
			if (lock == 1)
				gi.cvar_set("allow_vote_tl", "1");
			else
				gi.cvar_set("allow_vote_tl", "0");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has %s \"timelimit\" vote option.\n", ent->client->pers.netname, lock == 0 ? "locked" : "unlocked");
		}
		else if (!strcmp(p, "dmf") || !strcmp(p, "dmflags"))
		{
			if (lock == 1)
				gi.cvar_set("allow_vote_dmf", "1");
			else
				gi.cvar_set("allow_vote_dmf", "0");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has %s \"dmflags\" vote option.\n", ent->client->pers.netname, lock == 0 ? "locked" : "unlocked");
		}
		else if (!strcmp(p, "bfg"))
		{
			if (lock == 1)
				gi.cvar_set("allow_vote_bfg", "1");
			else
				gi.cvar_set("allow_vote_bfg", "0");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has %s \"bfg\" vote option.\n", ent->client->pers.netname, lock == 0 ? "locked" : "unlocked");
		}
		else if (!strcmp(p, "powerups"))
		{
			if (lock == 1)
				gi.cvar_set("allow_vote_powerups", "1");
			else
				gi.cvar_set("allow_vote_powerups", "0");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has %s \"powerups\" vote option.\n", ent->client->pers.netname, lock == 0 ? "locked" : "unlocked");
		}
		else if (!strcmp(p, "fastweapons"))
		{
			if (lock == 1)
				gi.cvar_set("allow_vote_fastweapons", "1");
			else
				gi.cvar_set("allow_vote_fastweapons", "0");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has %s \"fastweapons\" vote option.\n", ent->client->pers.netname, lock == 0 ? "locked" : "unlocked");
		}
		else if (!strcmp(p, "config"))
		{
			if (lock == 1)
				gi.cvar_set("allow_vote_config", "1");
			else
				gi.cvar_set("allow_vote_config", "0");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has %s \"config\" vote option.\n", ent->client->pers.netname, lock == 0 ? "locked" : "unlocked");
		}
		else if (!strcmp(p, "map"))
		{
			if (lock == 1)
				gi.cvar_set("allow_vote_map", "1");
			else
				gi.cvar_set("allow_vote_map", "0");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has %s \"map\" vote option.\n", ent->client->pers.netname, lock == 0 ? "locked" : "unlocked");
		}
		else if (!strcmp(p, "kick"))
		{
			if (lock == 1)
				gi.cvar_set("allow_vote_kick", "1");
			else
				gi.cvar_set("allow_vote_kick", "0");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has %s \"kick\" vote option.\n", ent->client->pers.netname, lock == 0 ? "locked" : "unlocked");
		}
		else if (!strcmp(p, "tp"))
		{
			if (lock == 1)
				gi.cvar_set("allow_vote_tp", "1");
			else
				gi.cvar_set("allow_vote_tp", "0");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has %s \"tp\" vote option.\n", ent->client->pers.netname, lock == 0 ? "locked" : "unlocked");
		}
		else if (!strcmp(p, "hud"))
		{
			if (lock == 1)
				gi.cvar_set("allow_vote_hud", "1");
			else
				gi.cvar_set("allow_vote_hud", "0");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has %s \"hud\" vote option.\n", ent->client->pers.netname, lock == 0 ? "locked" : "unlocked");
		}
		else if (!strcmp(p, "hand3"))
		{
			if (lock == 1)
				gi.cvar_set("allow_vote_hand3", "1");
			else
				gi.cvar_set("allow_vote_hand3", "0");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "%s has %s \"hand3\" vote option.\n", ent->client->pers.netname, lock == 0 ? "locked" : "unlocked");
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "Unknown vote option.\n");
	}
}

void VoteCheckDisconnect (edict_t *ent)
{
	int client_num = get_client_num_from_edict( ent );

	if (level.vote.vote_active)
	{
		if (ent == level.vote.voter)
		{
			CancelVote(ent, false);
			return;
		}
		else if ( client_num >= 0 && ( level.vote.vote_what & VOTE_KICK ) && client_num == level.vote.kick )
		{
			CancelVote( ent, true );
			return;
		}
		else
		{
			if (ent->client->pers.save_data.vote_yon == VOTE_NO)
			{
				ent->client->pers.save_data.vote_yon = VOTE_NONE;
				ent->client->pers.save_data.vote_change_count = 0;
				level.vote.vote_no -= 1;
				level.vote.update_vote = true;
			}
			else if (ent->client->pers.save_data.vote_yon == VOTE_YES)
			{
				ent->client->pers.save_data.vote_yon = VOTE_NONE;
				ent->client->pers.save_data.vote_change_count = 0;
				level.vote.vote_yes -= 1;
				level.vote.update_vote = true;
			}
		}
	}
}

/*
 ** 1.D - Admin
 */

void Cmd_Admin_f(edict_t *ent)
{
	char	*p;
	qboolean is_admin=false;
	struct admin_s *admin;

	if ( ent->client->pers.mvdspec )
		return;

	if (ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are an admin already.\n");
		return;
	}

	if (gi.argc() < 2)
	{
		gi.cprintf(ent, PRINT_HIGH, "USAGE: admin <password>.\n");
		return;
	}

	p = gi.argv(1);

	if (!strcmp(p, ""))
	{
		gi.cprintf(ent, PRINT_HIGH, "USAGE: admin <password>.\n");
		return;
	}

	for (admin = game.admin_first; admin; admin = admin->next)
	{
		if (!strcmp(p, admin->password) && (admin->time > 0 || admin->time == -1))
		{
			is_admin = true;
			break;
		}
	}

	if (is_admin)
	{
		if (!admin->judge)
		{
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has become an admin.\n", ent->client->pers.netname);
			gi.cprintf(ent, PRINT_HIGH, "Admin mode on.\n");
			ent->client->pers.save_data.judge = false;
			ent->client->pers.save_data.admin_flags |= admin->flags;
		}
		else
		{
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has become a judge.\n", ent->client->pers.netname);
			gi.cprintf(ent, PRINT_HIGH, "Judge mode on.\n");
			ent->client->pers.save_data.judge = true;
			ent->client->pers.save_data.admin_flags = (int)sv_referee_flags->value;
		}
		ent->client->pers.save_data.is_admin = true;
		strcpy(ent->client->pers.save_data.admin_password, admin->password);
		if (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH)
			admin->used = true;
		ClientUserinfoChanged(ent, ent->client->pers.userinfo);
	}
	else
	{
		gi.cprintf(ent, PRINT_HIGH, "Invalid password.\n");
		return;
	}

	if (!(ent->client->pers.save_data.admin_flags & AD_ALL))
		MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Your allowed command are:\nkickuser, details, mute, ");
	if (ent->client->pers.save_data.admin_flags & AD_SERVER)
		MyCPrintf(ent, CPRINTF, PRINT_HIGH, "smap, dmf\n");
	if (ent->client->pers.save_data.admin_flags & AD_TEAMS)
		MyCPrintf(ent, CPRINTF, PRINT_HIGH, "readyteam, teamname, teamskin, pickplayer, kickplayer, lockteam, unlockteam\n");
	if (ent->client->pers.save_data.admin_flags & AD_BAN)
		MyCPrintf(ent, CPRINTF, PRINT_HIGH, "banlist, kickban, unban, ban\n");
	if (ent->client->pers.save_data.admin_flags & AD_MATCH)
		MyCPrintf(ent, CPRINTF, PRINT_HIGH, "start, break, tl, bfg, powerups, time\n");
	if (!(ent->client->pers.save_data.admin_flags & AD_ALL))
		MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Type acommands for details\n");

	if (admin->time > 0)
		MyCPrintf(ent, CPRINTF, PRINT_HIGH, "You can use your password for %d matches.\n", admin->time);

	if (!(ent->client->pers.save_data.admin_flags & AD_ALL) || admin->time > 0)
		MyCPrintfEnd(ent, CPRINTF, PRINT_HIGH);

	Log(ent, LOG_ADMIN, "admin");
}

void Cmd_Silence_f(edict_t *ent)
{
	char *p;
	edict_t *cl;
	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}

	if (gi.argc() < 2 )
	{
		gi.cprintf(ent, PRINT_HIGH, "USAGE: mute <player name or ID>.\n");
		return;
	}

	p = gi.argv(1);

	cl = find_client(ent, p);
	if (cl)
	{
		if ( cl->client->resp.silenced == true )
		{
			cl->client->resp.silenced = false;
			gi.bprintf( PRINT_HIGH, "%s has been UNmuted by %s.\n", cl->client->pers.netname, ent->client->pers.netname);
		}
		else
		{
			cl->client->resp.silenced = true;
			gi.bprintf( PRINT_HIGH, "%s has been muted by %s.\n", cl->client->pers.netname, ent->client->pers.netname);
		}
	}
	else
		gi.cprintf(ent, PRINT_HIGH, "Invalid player name or ID.\n");	
}

void Cmd_AdminList_f(edict_t *ent)
{
	int	i=0;
	char	flags[6];
	struct admin_s *admin;

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else if (!(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}
	
	Log(ent, LOG_ADMIN, "adminlist");
	AddToLongCPrintf(ent, "No | Name             | Password         | Flags | Time\n---+------------------+------------------+-------+------\n");

	for (admin = game.admin_first; admin; admin = admin->next)
	{
		if (strlen(admin->password))
		{
			flags[0]='\0';
			if (admin->flags & AD_ALL)
				strcat(flags, "A");
			else
			{
				if (admin->flags & AD_MATCH)
					strcat(flags, "M");
				if (admin->flags & AD_TEAMS)
					strcat(flags, "T");
				if (admin->flags & AD_SERVER)
					strcat(flags, "S");
				if (admin->flags & AD_BAN)
					strcat(flags, "B");
			}
			flags[strlen(flags)] = '\0';
			if (!admin->judge)
				AddToLongCPrintf(ent, "%-2d | %-16s | %-16s | %-5s | %-4d\n", i, admin->nick, admin->password, flags, admin->time);
			else
				AddToLongCPrintf(ent, "%-2d | REFEREE          | %-16s | %-5s | %-4d - REFEREE PASSWORD\n", i, admin->password, flags, admin->time);
		}
		i++;
	}
	AddToLongCPrintf(ent, "\nFlags: (A)ll, (M)atch, (S)erver, (T)eam, (B)an.\nTime: 0 = admin deleted, -1 = time not set.\n");
}

void WriteAdminList(void);
void Cmd_Noadmin_f(edict_t *ent);

void DeleteAdmin(struct admin_s *admin)
{
	int	i;
	edict_t *cl;

	if (!admin)
		return;

	if (strlen(admin->password))
	{
		for (i=0 ; i<game.maxclients ; i++)
		{
			cl = &g_edicts[1+i];
			if (!cl->inuse)
				continue;

			if (cl->client->pers.save_data.is_admin)
			{
				if (!strcmp(cl->client->pers.save_data.admin_password, admin->password))
					Cmd_Noadmin_f(cl);
			}
		}
	}
	UNLINK(admin, game.admin_first, game.admin_last, next, prev);
	free(admin);
}

void Cmd_Refcode_f(edict_t *ent)
{
	struct admin_s *admin, *adminList, *newAdmin;
	qboolean is_judge = false;
	char *newCode;
	int time = -1, i;
	edict_t *cl;

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else if (!(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}

	for (admin = game.admin_first; admin; admin = admin->next)
	{
		if (admin->judge)
		{
			is_judge = true;
			break;
		}
	}

	if (gi.argc() < 2)
	{
		if (!is_judge)
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: refcode <password> <time>.\n");
			return;
		}
		DeleteAdmin(admin);
		gi.cprintf(ent, PRINT_HIGH, "Refcode cleared.\n");
		return;
	}

	newCode = gi.argv(1);

	if (strlen(newCode) > 16 || strlen(newCode) < 6)
	{
		gi.cprintf(ent, PRINT_HIGH, "Password must be at last 6 and not more than 16 chars long.\n");
		return;
	}

	for(adminList = game.admin_first; adminList; adminList = adminList->next)
	{
		if (!strcmp(adminList->password, newCode))
		{
			gi.cprintf(ent, PRINT_HIGH, "The password is already in use, try another.\n");
			return;
		}
	}

	if (gi.argc() >= 3)
		time = atoi(gi.argv(2));

	if (time == 0 || time < -1)
	{
		gi.cprintf(ent, PRINT_HIGH, "Time must be greater than 0 or equal -1.\n");
		return;
	}

	if (is_judge)
	{
		for (i=0 ; i<game.maxclients ; i++)
		{
			cl = &g_edicts[1+i];
			if (!cl->inuse)
				continue;

			if (cl->client->pers.save_data.is_admin)
			{
				if (!strcmp(cl->client->pers.save_data.admin_password, admin->password))
					Cmd_Noadmin_f(cl);
			}
		}
		strncpy(admin->password, newCode, sizeof(admin->password));
		admin->time = time;
		admin->used = false;
		gi.cprintf(ent, PRINT_HIGH, "Refcode changed to %s.\n", newCode);
		return;
	}

	newAdmin = (struct admin_s*)malloc(sizeof(struct admin_s));
	if (!newAdmin)
	{
		gi.cprintf(ent, PRINT_HIGH, "Error while allocating new admin structure.\n");
		return;
	}
	memset(newAdmin, 0, sizeof(struct admin_s));
	newAdmin->judge = true;
	strncpy(newAdmin->password, newCode, sizeof(newAdmin->password));
	newAdmin->time = time;
	newAdmin->flags = (int)sv_referee_flags->value;
	LINK(newAdmin, game.admin_first, game.admin_last, next, prev);
	gi.cprintf(ent, PRINT_HIGH, "Refcode set to %s.\n", newCode);
}

void Cmd_Reftag_f(edict_t *ent)
{
	char *refTag;
	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else if (!(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}
	if (gi.argc() < 2)
	{
		if (sv_referee_tag->string)
			gi.cprintf(ent, PRINT_HIGH, "Referee tag is set to \"%s\".\n", sv_referee_tag->string);
		else
			gi.cprintf(ent, PRINT_HIGH, "Referee tag is not set.\n", sv_referee_tag->string);
		return;
	}

	refTag = gi.argv(1);
	if (strlen(refTag) < 1 || strlen(refTag) > 15)
	{
		gi.cprintf(ent, PRINT_HIGH, "Referee tag must be at last 1 and not more than 15 chars long\n", sv_referee_tag->string);
		return;
	}
	gi.cvar_set("sv_referee_tag", refTag);
	gi.cprintf(ent, PRINT_HIGH, "Referee tag set to %s.\n", sv_referee_tag->string);
}

void Cmd_Refflags_f(edict_t *ent)
{
	int i, iFlags;
	char *refFlags;

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else if (!(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}
	if (gi.argc() < 2)
	{
		if (sv_referee_flags->value)
		{
			char flags[5];
			memset(flags, 0, sizeof(flags));

			if ((int)sv_referee_flags->value & AD_ALL)
				flags[0] = 'A';
			else
			{
				if ((int)sv_referee_flags->value & AD_SERVER)
					strcat(flags, "S");
				if ((int)sv_referee_flags->value & AD_TEAMS)
					strcat(flags, "T");
				if ((int)sv_referee_flags->value & AD_BAN)
					strcat(flags, "B");
				if ((int)sv_referee_flags->value & AD_MATCH)
					strcat(flags, "M");
			}

			gi.cprintf(ent, PRINT_HIGH, "Referee flags are set to \"%s\".\n", flags);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "Referee flags are not set.\n");
		return;
	}

	refFlags = gi.argv(1);
	iFlags = 0;

	for (i=0; i<strlen(refFlags); i++)
	{
		if (refFlags[i] == 'a' || refFlags[i] == 'A')
		{
			iFlags |= AD_ALL;
			break;
		}
		if (refFlags[i] == 'b' || refFlags[i] == 'B')
			iFlags |= AD_BAN;
		if (refFlags[i] == 's' || refFlags[i] == 'S')
			iFlags |= AD_SERVER;
		if (refFlags[i] == 'm' || refFlags[i] == 'M')
			iFlags |= AD_MATCH;
		if (refFlags[i] == 't' || refFlags[i] == 'T')
			iFlags |= AD_TEAMS;
	}

	gi.cvar_set("sv_referee_flags", va("%d", iFlags));
	gi.cprintf(ent, PRINT_HIGH, "Referee flags set to %s (%d).\n", refFlags, iFlags);
}

void Cmd_DelAdmin_f(edict_t *ent)
{
	char	*p;
	char	string[1024];
	int		admin_num, i=0;
	struct admin_s *toDelete;
	qboolean found = false;

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else if (!(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}
	
	if (gi.argc() < 2)
	{
		Cmd_AdminList_f(ent);
		gi.cprintf(ent, PRINT_HIGH, "USAGE: deladmin <admin_num>.\n");
		return;
	}
	

	p = gi.argv(1);
	if (strlen(p) > 3)
	{
		Cmd_AdminList_f(ent);
		gi.cprintf(ent, PRINT_HIGH, "USAGE: deladmin <admin_num>.\n");
		return;
	}

	string[0] = '\0';
	admin_num = atoi(p);
	sprintf(string, "%d", admin_num);

	if (strcmp(string, p))
	{
		Cmd_AdminList_f(ent);
		gi.cprintf(ent, PRINT_HIGH, "USAGE: deladmin <admin_num>.\n");
		return;
	}

	for(toDelete = game.admin_first; toDelete; toDelete = toDelete->next)
	{
		if (i == admin_num)
		{
			found = true;
			break;
		}
		i++;
	}

	if (!found)
	{
		gi.cprintf(ent, PRINT_HIGH, "Admin not found.\n");
		return;
	}

	if (toDelete->time == 0)
	{
		gi.cprintf(ent, PRINT_HIGH, "This admin is already deleted.\n");
		return;
	}

	DeleteAdmin(toDelete);
	Log(ent, LOG_ADMIN, "deladmin");
	gi.cprintf(ent, PRINT_HIGH, "OK.\n");
	WriteAdminList();
}

void Cmd_Noadmin_f(edict_t *ent)
{
	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else
	{
		Log(ent, LOG_ADMIN, "noadmin");
		gi.cprintf(ent, PRINT_HIGH, "Admin mode off.\n");
		ent->client->pers.save_data.is_admin = false;
		ent->client->pers.save_data.admin_flags = 0;
		ent->client->pers.save_data.admin_password[0] = '\0';
		ent->client->pers.save_data.judge = false;
		ClientUserinfoChanged(ent, ent->client->pers.userinfo);
	}
}

void Cmd_Smap_f(edict_t *ent)
{
	char	*p;
	int		i;

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else if (!(ent->client->pers.save_data.admin_flags & AD_SERVER) && !(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}
	else
	{
		if (gi.argc() < 2)
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: smap <mapname>.\n");
			return;
		}
		p = gi.argv(1);

		for(i=0; i<strlen(p); i++)
			p[i] = LOWER(p[i]);
		p[i]='\0';

		if (IsValidMap(p, false))
		{
			Log(ent, LOG_ADMIN, "smap");
			strcpy(level.nextmap, p);
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Map changed to %s by %s.\n", level.nextmap, ent->client->pers.netname);
			EndDMLevel(true, false);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "Map %s not on the map list.\n", p);
	}
}

void Cmd_Break_f(edict_t *ent)
{
	int	i;
	edict_t	*cl;
	edict_t	*ento;
	struct admin_s *adminCheck;

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else if (!(ent->client->pers.save_data.admin_flags & AD_MATCH) && !(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}
	else
	{
		if (level.match_state != WARMUP)
		{
			Log(ent, LOG_ADMIN, "break");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Match ended by %s.\n", ent->client->pers.netname);
			level.match_state = WARMUP;
			level.teamA_score = 0;
			level.teamB_score = 0;
			level.teamA_numpauses = 0;
			level.teamB_numpauses = 0;

			level.paused = false;
			level.pauser = NULL;

			ClearDominationRunes();
			FreeOldScores();
			FreeSavedPlayers();

			for (adminCheck = game.admin_first; adminCheck; adminCheck = adminCheck->next)
			{
				if (strlen(adminCheck->password))
					adminCheck->used = false;
			}

			for (i=0 ; i<game.maxclients ; i++)
			{
				cl = &g_edicts[1+i];
				if (!cl->inuse)
					continue;

				cl->client->pers.save_data.ready_or_not = false;
				cl->client->resp.score = 0;
				cl->client->resp.net = 0;

				Reset_Accuracy(cl);

				if (cl->client->pers.save_data.team != TEAM_NONE )
				{
					char userinfo[MAX_INFO_STRING];

					if (cl->health > 0)
					{
						memcpy (userinfo, cl->client->pers.userinfo, sizeof(userinfo));
						InitClientPersistant(cl->client);
						cl->client->newweapon = FindItem ("Railgun");
						ChangeWeapon (cl);
						ClientUserinfoChanged (cl, userinfo);
					}
				}
				if (cl->client->pers.save_data.autorecord)
				{
					gi.WriteByte (svc_stufftext);
					gi.WriteString ("stop\n");
					gi.unicast(cl, true);
				}
			}
			level.update_score = true;
			if (game.teamA_locked || game.teamB_locked)
			{
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Teams unlocked!\n");
				game.teamB_locked = false;
				game.teamA_locked = false;
			}
			for (i=1, ento=g_edicts+i; i < globals.num_edicts; i++,ento++)
			{
				if (!ento->item)
					continue;

				if (ento->spawnflags & DROPPED_ITEM || ento->spawnflags & DROPPED_PLAYER_ITEM)
					G_FreeEdict(ento);

				SetRespawn(ento,1);
			}
		}
	}
}

void Cmd_Start_f(edict_t *ent)
{
	edict_t	*cl;
	int		i, a=0,b=0;

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else if (!(ent->client->pers.save_data.admin_flags & AD_MATCH) && !(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}
	else
	{
		if (level.match_state == WARMUP || level.match_state == PREGAME)
		{
			for (i=0 ; i<game.maxclients ; i++)
			{
				cl = &g_edicts[1+i];
				if (!cl->inuse)
					continue;

				if (cl->client->pers.save_data.team != TEAM_NONE)
				{
					if (cl->client->pers.save_data.team == TEAM_A)
						a++;
					else
						b++;
					cl->client->pers.save_data.ready_or_not = true;
				}
			}
			if (a && b)
			{
				Log(ent, LOG_ADMIN, "start");
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Match start forced by %s.\n", ent->client->pers.netname);
			}
		}
	}
}

void Cmd_Details_f(edict_t *ent)
{
	edict_t	*cl;
	int		i;

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else
	{
		AddToLongCPrintf(ent, "ID |      name       | score | net | team | ping |       IP\n---+-----------------+-------+-----+------+------+-----------------\n");
		for (i=0 ; i<game.maxclients ; i++)
		{
			cl = &g_edicts[1+i];
			if (!cl->inuse)
				continue;

			if (cl->client->pers.save_data.team == TEAM_A)
				AddToLongCPrintf(ent, "%-2d | %15s | %5d | %3d |   A  | %4d | %15s \n", i, cl->client->pers.netname, cl->client->resp.score, cl->client->resp.score-cl->client->resp.net, cl->client->ping, IPLongToString(cl->client->pers.save_data.ip));
			else if (cl->client->pers.save_data.team == TEAM_B)
				AddToLongCPrintf(ent, "%-2d | %15s | %5d | %3d |   B  | %4d | %15s \n", i, cl->client->pers.netname, cl->client->resp.score, cl->client->resp.score-cl->client->resp.net, cl->client->ping, IPLongToString(cl->client->pers.save_data.ip));
			else
				AddToLongCPrintf(ent, "%-2d | %15s | %5d | %3d |   S  | %4d | %15s \n", i, cl->client->pers.netname, cl->client->resp.score, cl->client->resp.score-cl->client->resp.net, cl->client->ping, IPLongToString(cl->client->pers.save_data.ip));
		}
		Log(ent, LOG_ADMIN, "details");
	}
}

void Cmd_Kickuser_f(edict_t *ent)
{
	char	*p;
	char	text[80];

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else
	{
		if (gi.argc() < 2)
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: kickuser <playerID or name>.\n\n");
			Cmd_Details_f(ent);
			return;
		}
		p = gi.argv(1);

		if (find_client_num(ent, p) > -1)
		{
			edict_t *cl;
			cl = find_client(ent, p);
			if (cl)
			{
				if (cl->client->pers.save_data.is_admin && !cl->client->pers.save_data.judge && ent->client->pers.save_data.judge)
				{
					gi.cprintf(ent, PRINT_HIGH, "You can't kick an admin!\n");
						return;
				}
			}
			sprintf(text, "kick %d\n", find_client_num(ent, p));
			Log(ent, LOG_ADMIN, "kuckuser");
			gi.AddCommandString(text);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "Invalid client ID\n");
	}
}

void WriteAdminList(void)
{
	cvar_t	*basedir, *gamedir;
	char	filename[256];
	FILE	*f;
	int		i=0;
	char string[MAX_STRING_CHARS];
	struct admin_s *toWrite;

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);

	strcpy(filename,basedir->string);

	if(strlen(gamedir->string))
	{
		strcat(filename,"/");
		strcat(filename,gamedir->string);
		if (strlen(sv_adminlist->string))
		{
			strcat(filename,"/");
			strcat(filename, sv_adminlist->string);
		}
		else
		{
			strcat(filename,"/admin");
			strcat(filename,port->string);
			strcat(filename,".lst");
		}
	}
	else
	{
		if (strlen(sv_adminlist->string))
		{
			strcat(filename,"/baseq2/");
			strcat(filename, sv_adminlist->string);
		}
		else
		{
			strcat(filename,"/baseq2/admin");
			strcat(filename,port->string);
			strcat(filename,".lst");
		}
	}
	
	f = fopen(filename,"w");

	if(!f)
	{
		gi.dprintf("Can't create %s!\n", filename);
		return;
	}
	else
	{
		for (toWrite = game.admin_first; toWrite; toWrite = toWrite->next)
		{
			if (strlen(toWrite->password))
			{
				if (toWrite->time == 0)
					continue;
				sprintf(string, "%s %s %d %d\n", toWrite->judge ? "referee" : toWrite->nick, toWrite->password, toWrite->time, toWrite->judge ? -1 : toWrite->flags);
				fputs(string, f);
			}
			else
				break;
		}
		fclose(f);
	}
}

void LoadAdminList(void)
{
	cvar_t	*basedir, *gamedir;
	char	filename[256];
	FILE	*f=NULL;
	int		i=0;
	int		num=0;
	struct admin_s *newAdmin;

	newAdmin = game.admin_first;
	while(newAdmin)
	{
		UNLINK(newAdmin, game.admin_first, game.admin_last, next, prev);
		free(newAdmin);
		newAdmin = game.admin_first;
	}

	game.admin_first = NULL;
	game.admin_last = NULL;

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);

	strcpy(filename,basedir->string);
	if(strlen(gamedir->string))
	{
		strcat(filename,"/");
		strcat(filename,gamedir->string);
		if (strlen(sv_adminlist->string))
		{
			strcat(filename,"/");
			strcat(filename, sv_adminlist->string);
		}
		else
		{
			strcat(filename,"/admin");
			strcat(filename,port->string);
			strcat(filename,".lst");
		}
		gi.dprintf("Loading admin list from %s...", filename);
		f = fopen(filename,"r");
		if (!f)
		    gi.dprintf("file not found!\nTrying to load from ");
	}

	if (!f || !strlen(gamedir->string))
	{
		strcpy(filename,basedir->string);
		if (strlen(sv_adminlist->string))
		{
			strcat(filename,"/baseq2/");
			strcat(filename, sv_adminlist->string);
		}
		else
		{
			strcat(filename,"/baseq2/admin");
			strcat(filename,port->string);
			strcat(filename,".lst");
		}
		gi.dprintf("%s...", filename);
		f = fopen(filename,"r");
		if (!f)
		{
		    gi.dprintf("file not found!\n");
		    return;
		}
	}

	if (f)
	{
		char	*line;
		do
		{
			line = read_line(f);
			if (!line)
				break;

			newAdmin = (struct admin_s*)malloc(sizeof(struct admin_s));
			memset(newAdmin, 0, sizeof(struct admin_s));

			strcpy(newAdmin->nick, read_word(line, 0));
			strcpy(newAdmin->password, read_word(line, 1));
			if (strlen(newAdmin->password))
				num++;
			newAdmin->time = atoi(read_word(line, 2));
			newAdmin->flags = atoi(read_word(line, 3));
			if (newAdmin->flags == -1)
			{
				newAdmin->flags = (int)sv_referee_flags->value;
				newAdmin->judge = true;
			}
			LINK(newAdmin, game.admin_first, game.admin_last, next, prev);
			i++;
		} while (!feof(f));
		fclose(f);
	}
	gi.dprintf("Done (%d admin%s)\n\n", num, num > 1 ? "s" : "");
}

void Cmd_GiveAdmin_f(edict_t *ent)
{
	char	*name;
	char	*password;
	char	*flags;
	char	*time;
	int		i;
	edict_t	*player=NULL;
	int		free_slot=-1;
	qboolean badflags=false;
	struct admin_s *newAdmin;

	if (!ent->client->pers.save_data.is_admin)
		return;
	if (!(ent->client->pers.save_data.admin_flags & AD_ALL))
		return;

	if (gi.argc() < 5)
	{
		gi.cprintf(ent, PRINT_HIGH, "USAGE: giveadmin <player name or ID> <password> <num. matches> <stbma>.\n");
		return;
	}

	name = gi.argv(1);
	player = find_client(ent, name);

	if (!player)
	{
		gi.cprintf(ent, PRINT_HIGH, "Invalid player name or ID.\n");
		return;
	}

	password = gi.argv(2);

	if (strlen(password) > 16 || strlen(password) < 6)
	{
		gi.cprintf(ent, PRINT_HIGH, "Password must be at last 6 and not more than 16 chars long.\n");
		return;
	}

	for(newAdmin = game.admin_first; newAdmin; newAdmin = newAdmin->next)
	{
		if (!strcmp(newAdmin->password, password))
		{
			gi.cprintf(ent, PRINT_HIGH, "The password is already in use, try another.\n");
			return;
		}
	}

	time = gi.argv(3);

	if (atoi(time) == 0)
	{
		gi.cprintf(ent, PRINT_HIGH, "Time must be greater than 0 or equal -1.\n");
		return;
	}

	flags = gi.argv(4);

	if (strlen(flags) > 5)
	{
		gi.cprintf(ent, PRINT_HIGH, "Too many server's flags.\n");
		return;
	}

	for (i=0; i<strlen(flags); i++)
	{
		if (flags[i] != 'a' && flags[i] != 'b' && flags[i] != 's' && flags[i] != 'm' && flags[i] != 't')
		{
			gi.cprintf(ent, PRINT_HIGH, "Bad allowed commands flag: %c.\nYou can use fallowing flags: a (all), b (ban), s (server), m (match), t (teams).\n", flags[i]);
			return;
		}
	}

	newAdmin = (struct admin_s*)malloc(sizeof(struct admin_s));
	if (!newAdmin)
	{
		gi.cprintf(ent, PRINT_HIGH, "Error while allocating new admin structure.\n");
		return;
	}
	memset(newAdmin, 0, sizeof(struct admin_s));

	for (i=0; i<strlen(flags); i++)
	{
		if (flags[i] == 'a')
		{
			newAdmin->flags |= AD_ALL;
			break;
		}
		if (flags[i] == 'b')
			newAdmin->flags |= AD_BAN;
		if (flags[i] == 's')
			newAdmin->flags |= AD_SERVER;
		if (flags[i] == 'm')
			newAdmin->flags |= AD_MATCH;
		if (flags[i] == 't')
			newAdmin->flags |= AD_TEAMS;
	}

	strcpy(newAdmin->nick, player->client->pers.netname);
	strcpy(newAdmin->password, password);
	newAdmin->time = atoi(time);

	if (!(newAdmin->flags & AD_ALL))
	{
		gi.cprintf(player, PRINT_CHAT, "%s has allowed you to use the following admin's commands:\n", ent->client->pers.netname);
		MyCPrintf(player, CPRINTF, PRINT_HIGH,"kickuser, details, ");
		if (newAdmin->flags & AD_SERVER)
			MyCPrintf(player, CPRINTF, PRINT_HIGH, "smap, dmf\n");
		if (newAdmin->flags & AD_TEAMS)
			MyCPrintf(player, CPRINTF, PRINT_HIGH, "readyteam, teamname, teamskin, pickplayer, kickplayer, lockteam, unlockteam\n");
		if (newAdmin->flags & AD_BAN)
			MyCPrintf(player, CPRINTF, PRINT_HIGH, "banlist, kickban, unban, ban\n");
		if (newAdmin->flags & AD_MATCH)
			MyCPrintf(player, CPRINTF, PRINT_HIGH, "start, break, tl, bfg, powerups, time\n");
		MyCPrintfEnd(player, CPRINTF, PRINT_HIGH);
	}
	else
		gi.cprintf(player, PRINT_CHAT, "%s has allowed you to use all admin's commands.\n", ent->client->pers.netname);

	MyCPrintf(player, CPRINTF, PRINT_HIGH, "Your admin password is \"%s\". Don't forget it!\n", newAdmin->password);
	if (newAdmin->time > 0)
		MyCPrintf(player, CPRINTF, PRINT_HIGH, "You can use the password for %d matches.\n", newAdmin->time);
	MyCPrintf(player, CPRINTF, PRINT_HIGH, "Type \"admin %s\" in console in order to become an admin.\n", newAdmin->password);
	MyCPrintfEnd(player, CPRINTF, PRINT_HIGH);
	gi.cprintf(ent, PRINT_HIGH, "You have allowed %s to use password: \"%s\"\n", player->client->pers.netname, newAdmin->password);

	LINK(newAdmin, game.admin_first, game.admin_last, next, prev);

	Log(ent, LOG_ADMIN, "giveadmin");

	WriteAdminList();
}

void Cmd_Pass_f (edict_t *ent)
{
	char	*p;

	if (!ent->client->pers.save_data.is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}
	else if (!(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}

	if (gi.argc() < 2)
	{
		if (strlen(password->string))
		{
			gi.cvar_set("password", "");
			gi.cprintf(ent, PRINT_HIGH, "Password disabled.\n");
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "USAGE: pass <password>, or pass to disable\n");
	}
	else
	{
		p = gi.argv(1);
		gi.cvar_set("password", p);
		gi.cprintf(ent, PRINT_HIGH, "Server password set to \"%s\".\n",p);
		Log(ent, LOG_ADMIN, "pass");
	}
}

void Cmd_Obsmode_f (edict_t *ent)
{
	qboolean	is_admin = false;
	char	*p;

	if (((ent->client->pers.save_data.admin_flags & AD_SERVER) || (ent->client->pers.save_data.admin_flags & AD_ALL)) && ent->client->pers.save_data.is_admin)
		is_admin = true;

	if (gi.argc() < 2)
	{
		if (sv_obsmode->value == 1)
			gi.cprintf(ent, PRINT_HIGH, "Spectators can talk only with other spectators during the match.\n");
		else if (sv_obsmode->value == 2)
			gi.cprintf(ent, PRINT_HIGH, "Spectators may not speak during match.\n");
		else
			gi.cprintf(ent, PRINT_HIGH, "Spectators can talk to players during match.\n");

		return;
	}
	else if (is_admin)
	{
		p = gi.argv(1);

		if (Q_stricmp(p, "speak") == 0)
			gi.cvar_set("sv_obsmode", "0");
		else if (Q_stricmp(p, "whisper") == 0)
			gi.cvar_set("sv_obsmode", "1");
		else if (Q_stricmp(p, "shutup") == 0)
			gi.cvar_set("sv_obsmode", "2");
		else
		{
			gi.cprintf(ent, PRINT_HIGH, "Usage: obsmode <speak|whisper|shutup>.\n");
			return;
		}
		Log(ent, LOG_ADMIN, "obsmode");
		gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Obsmode changed to \"%s\" by %s.\n", p, ent->client->pers.netname);
	}
	else
		gi.cprintf(ent, PRINT_HIGH, "You can't do that.\n");
}

void Cmd_Config_f (edict_t *ent)
{
	qboolean	is_admin = false;
	char	*p;

	if (((ent->client->pers.save_data.admin_flags & AD_SERVER) || (ent->client->pers.save_data.admin_flags & AD_ALL)) && ent->client->pers.save_data.is_admin)
		is_admin = true;

	if (!(ent->client->pers.save_data.admin_flags & AD_SERVER) && !(ent->client->pers.save_data.admin_flags & AD_ALL))
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't use this command.\n");
		return;
	}
	else if (!is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}

	if (gi.argc() < 2)
	{
		gi.cprintf(ent, PRINT_HIGH, "USAGE: config <filename>.\n");
		Cmd_ConfigList_f(ent);
		return;
	}

	p = gi.argv(1);

	if (CheckSvConfigList(p) == true)
	{
		char text[80];

		Log(ent, LOG_ADMIN, "config");
		Com_sprintf(text, sizeof(text), "exec %s", p);
		gi.AddCommandString(text);
	}
	else
	{
		gi.cprintf(ent, PRINT_HIGH, "Invalid config filename.\n");
		Cmd_ConfigList_f(ent);
	}
}

/*
** 1.E - Team
*/

void Cmd_Captain_f (edict_t *ent)
{
	char	*p;
	char	*q;
	int		i;
	int		team;
	edict_t	*cl;

	if (!ent->client)
		return;

	if (gi.argc() == 1)
	{
		if (game.teamA_captain && game.teamB_captain)
			gi.cprintf(ent, PRINT_HIGH, "Team %s captain is %s, team %s captain is %s.\n", game.teamA_name, game.teamA_captain->client->pers.netname, game.teamB_name, game.teamB_captain->client->pers.netname);
		else if (game.teamA_captain && !game.teamB_captain)
			gi.cprintf(ent, PRINT_HIGH, "Team %s captain is %s, team %s has no captain yet.\n", game.teamA_name, game.teamA_captain->client->pers.netname, game.teamB_name);
		else if (!game.teamA_captain && game.teamB_captain)
			gi.cprintf(ent, PRINT_HIGH, "Team %s has no captain yet, team %s captain is %s.\n", game.teamA_name, game.teamB_name, game.teamB_captain->client->pers.netname);
		else
			gi.cprintf(ent, PRINT_HIGH, "Team %s has no captain yet, team %s has no captain yet.\n", game.teamA_name, game.teamB_name);
		return;
	}

	if (((ent->client->pers.save_data.admin_flags & AD_TEAMS) || (ent->client->pers.save_data.admin_flags & AD_ALL)) && ent->client->pers.save_data.is_admin)
	{
		if (gi.argc() == 2)
		{
			team = ent->client->pers.save_data.team;
			if (team == TEAM_NONE)
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: captain <A or B> <player name or ID>.\n");
				return;
			}
			p = gi.argv(1);
		}
		else if (gi.argc() == 3)
		{
			q = gi.argv(1);

			for(i=0; i<strlen(q); i++)
				q[i] = LOWER(q[i]);
			q[i] = '\0';

			if (!strcmp("a", q))
				team = TEAM_A;
			else if (!strcmp("b", q))
				team = TEAM_B;
			else
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: captain <A or B> <player name or ID>.\n");
				return;
			}

			p = gi.argv(2);
		}
		else
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: captain <A or B> <player name or ID>.\n");
			return;
		}
	}
	else
	{
		if (game.teamA_captain != ent && game.teamB_captain != ent)
		{
			gi.cprintf(ent, PRINT_HIGH, "Only the team captain or an admin may choose a new captain..\n");
			return;
		}

		if (gi.argc() < 2 && ent->client->pers.save_data.team != TEAM_NONE)
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: captain <player name or ID>.\n");
			return;
		}

		if (ent->client->pers.save_data.team == TEAM_NONE)
		{
			gi.cprintf(ent, PRINT_HIGH, "You are not a team member.\n");
			return;
		}

		p = gi.args();

		team = ent->client->pers.save_data.team;
	}

	cl = find_client(ent, p);
	if (cl)
	{
		if (cl->client->pers.save_data.team == TEAM_NONE || cl->client->pers.save_data.team != team)
		{
			gi.cprintf(ent, PRINT_HIGH, "You can't choose this player!\n");
			return;
		}

		if ((cl == ent && ent == game.teamA_captain) || (cl == ent && ent == game.teamB_captain))
		{
			gi.cprintf(ent, PRINT_HIGH, "You are already the captain of your team!\n");
			return;
		}

		if (cl == game.teamA_captain || cl == game.teamB_captain)
		{
			gi.cprintf(ent, PRINT_HIGH, "%s is already the captain of team %s!\n", cl->client->pers.netname, cl->client->pers.save_data.team == TEAM_A ? game.teamA_name : game.teamB_name);
			return;
		}

		if (team == TEAM_A)
			game.teamA_captain = cl;
		else if (team == TEAM_B)
			game.teamB_captain = cl;

		gi.bprintf (ATTR_PRINT_HIGH|ATTR_BOLD, "%s was chosen to be captain of team %s by %s.\n", cl->client->pers.netname, team == TEAM_A ? game.teamA_name : game.teamB_name, ent->client->pers.netname);
	}
	else
		gi.cprintf(ent, PRINT_HIGH, "Invalid player name or ID.\n");
}

void Cmd_Join_f (edict_t *ent, int team)
{
	int		i;
	char	*p;

	if (!ent->client || ent->client->pers.mvdspec )
		return;

	if (team == TEAM_NONE)
	{
		if (gi.argc () < 2)
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: team <A or B>.\n");
			return;
		}
		p = gi.args();

		for(i=0; i<strlen(p); i++)
			p[i] = LOWER(p[i]);

		if (strcmp(p, "a") && strcmp(p, "A") && strcmp(p, "b") && strcmp(p, "B"))
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: team <A or B>.\n");
			return;
		}

		if ((ent->client->pers.save_data.team == TEAM_A && !strcmp("a", p)) || (ent->client->pers.save_data.team == TEAM_B && !strcmp("b", p)))
		{
			if (!strcmp("a", p))
				gi.cprintf(ent, PRINT_HIGH, "You are already on %s.\n", game.teamA_name);
			else
				gi.cprintf(ent, PRINT_HIGH, "You are already on %s.\n", game.teamB_name);
			return;
		}

		if ((!strcmp("a", p) && game.teamA_locked == true) || (!strcmp("b", p) && game.teamB_locked == true))
		{
			if (!strcmp("a", p))
				gi.cprintf(ent, PRINT_HIGH, "Team %s is locked!\n", game.teamA_name);
			else
				gi.cprintf(ent, PRINT_HIGH, "Team %s is locked!\n", game.teamB_name);
			return;
		}

		if ((ent->client->pers.save_data.team == TEAM_A || ent->client->pers.save_data.team == TEAM_NONE) && (!strcmp("b", p)))
		{
			ent->client->pers.save_data.team = TEAM_B;

//			CheckCaptain(ent);

			gi.bprintf (ATTR_PRINT_HIGH|ATTR_C_RED, "%s joined team %s.\n", ent->client->pers.netname, game.teamB_name);
			Write_ConfigString2(ent);
			Info_SetValueForKey (ent->client->pers.userinfo, "skin", game.teamB_skin);
			Info_SetValueForKey (ent->client->pers.userinfo, "spectator", "0");
			respawn(ent, false, true);
			return;
		}
		else if ((ent->client->pers.save_data.team == TEAM_B || ent->client->pers.save_data.team == TEAM_NONE) && (!strcmp("a", p)))
		{
			ent->client->pers.save_data.team = TEAM_A;

//			CheckCaptain(ent);

			gi.bprintf (ATTR_PRINT_HIGH|ATTR_C_BLUE, "%s joined team %s.\n", ent->client->pers.netname, game.teamA_name);
			Write_ConfigString2(ent);
			Info_SetValueForKey (ent->client->pers.userinfo, "skin", game.teamA_skin);
			Info_SetValueForKey(ent->client->pers.userinfo, "spectator", "0");
			respawn(ent, false, true);
			return;
		}
	}
	else
	{
		if ((team == TEAM_A && game.teamA_locked == true) || (team == TEAM_B && game.teamB_locked == true))
		{
			gi.cprintf(ent, PRINT_HIGH, "Team %s is locked!\n", team == TEAM_A ? game.teamA_name : game.teamB_name);
				return;
		}

		if (team == ent->client->pers.save_data.team)
		{
			switch (team)
			{
				case TEAM_A: gi.cprintf(ent, PRINT_HIGH, "You are already on %s.\n", game.teamA_name); break;
				case TEAM_B: gi.cprintf(ent, PRINT_HIGH, "You are already on %s.\n", game.teamB_name); break;
			}
			return;
		}
		else
		{
			switch (team)
			{
				case TEAM_A:
					gi.bprintf (ATTR_PRINT_HIGH|ATTR_C_RED, "%s joined team %s.\n", ent->client->pers.netname, game.teamA_name);
					ent->client->pers.save_data.team = TEAM_A;
//					CheckCaptain(ent);
					Write_ConfigString2(ent);
					Info_SetValueForKey (ent->client->pers.userinfo, "skin", game.teamA_skin);
					break;
				case TEAM_B:
					gi.bprintf (ATTR_PRINT_HIGH|ATTR_C_BLUE, "%s joined team %s.\n", ent->client->pers.netname, game.teamB_name);
					ent->client->pers.save_data.team = TEAM_B;
//					CheckCaptain(ent);
					Write_ConfigString2(ent);
					Info_SetValueForKey (ent->client->pers.userinfo, "skin", game.teamB_skin);
					break;
			}
			Info_SetValueForKey(ent->client->pers.userinfo, "spectator", "0");
			respawn(ent, false, true);
		}
	}
}

void Cmd_Ready_f(edict_t *ent, qboolean ready)
{
	int i;
	edict_t	*cl;
	int team_a=0;
	int team_b=0;

	if (ent->client->pers.save_data.ready_or_not == ready)
		return;

	if (level.match_state == WARMUP || level.match_state == PREGAME || level.match_state == COUNTDOWN)
	{
		if (ent->client->pers.save_data.team != TEAM_NONE)
		{
			for (i=0 ; i<game.maxclients ; i++)
			{
				cl = &g_edicts[1+i];
				if (!cl->inuse)
					continue;
			
				if (cl->client->pers.save_data.team == TEAM_A)
					team_a++;
				if (cl->client->pers.save_data.team == TEAM_B)
					team_b++;
			}

			if (!team_a || !team_b)
			{
				gi.cprintf(ent, PRINT_HIGH, "Cant ready until both team have players.\n");
				return;
			}

			ent->client->pers.save_data.ready_or_not = ready;
			if (ready == true)
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s is ready!\n", ent->client->pers.netname);
			else
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s is NOT ready!\n", ent->client->pers.netname);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "You are not on a team.\n");
	}
}

void Cmd_ReadyTeam_f(edict_t *ent)
{
	int i;
	edict_t	*cl;
	int team_a=0;
	int	ready_a=0;
	int	ready_b=0;
	int team_b=0;
	int	team=TEAM_NONE;
	char	*q;

	if (((ent->client->pers.save_data.admin_flags & AD_TEAMS) || (ent->client->pers.save_data.admin_flags & AD_ALL)) && ent->client->pers.save_data.is_admin)
	{
		if (gi.argc() == 2)
		{
			q = gi.argv(1);

			for(i=0; i<strlen(q); i++)
					q[i] = LOWER(q[i]);
			q[i] = '\0';

			if (!strcmp("a", q))
				team = TEAM_A;
			else if (!strcmp("b", q))
				team = TEAM_B;
			else
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: readyteam <A or B>.\n");
				return;
			}
		}
	}

	if (ent->client->pers.save_data.team != TEAM_NONE && team == TEAM_NONE)
	{
		if (game.teamA_captain != ent && game.teamB_captain != ent)
		{
			gi.cprintf(ent, PRINT_HIGH, "Only the team captain may ready the entire team.\n");
			return;
		}
		else
			team = ent->client->pers.save_data.team;
	}
	else if (team == TEAM_NONE)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not on a team.\n");
		return;
	}

	if (level.match_state == WARMUP || level.match_state == PREGAME)
	{
		for (i=0 ; i<game.maxclients ; i++)
		{
			cl = &g_edicts[1+i];
			if (!cl->inuse)
				continue;

			if (cl->client->pers.save_data.team == TEAM_A)
			{
				if (cl->client->pers.save_data.ready_or_not == true)
					ready_a++;
				team_a++;
			}
			else if (cl->client->pers.save_data.team == TEAM_B)
			{
				if (cl->client->pers.save_data.ready_or_not == true)
					ready_b++;
				team_b++;
			}
		}

		if (!team_a || !team_b)
		{
			gi.cprintf(ent, PRINT_HIGH, "Can't ready until both team have players.\n");
			return;
		}
		if ((team == TEAM_A && team_a == ready_a) || (team == TEAM_B && team_b == ready_b))
		{
			gi.cprintf(ent, PRINT_HIGH, "The team is ready.\n");
			return;
		}

		for (i=0 ; i<game.maxclients ; i++)
		{
			cl = &g_edicts[1+i];
			if (!cl->inuse)
				continue;

			if (cl->client->pers.save_data.team == team)
				cl->client->pers.save_data.ready_or_not = true;
		}
		if (team == TEAM_A)
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "Team %s is ready!\n", game.teamA_name);
		else
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "Team %s is ready!\n", game.teamB_name);
	}
}

void Cmd_Teamname_f(edict_t *ent)
{
	char	*p;
	char	*q;
	char	newteamname[MAX_QPATH];
	int		team;
	char	team_a_name[MAX_QPATH];
	char	team_b_name[MAX_QPATH];
	int		i;
	int		j=0;
	qboolean	admin=false;

	for(i=0; i<strlen(game.teamA_name); i++)
		team_a_name[i] = LOWER(game.teamA_name[i]);
	team_a_name[i]='\0';

	for(i=0; i<strlen(game.teamB_name); i++)
		team_b_name[i] = LOWER(game.teamB_name[i]);
	team_b_name[i]='\0';

	if (((ent->client->pers.save_data.admin_flags & AD_TEAMS) || (ent->client->pers.save_data.admin_flags & AD_ALL)) && ent->client->pers.save_data.is_admin)
	{
		if (gi.argc() == 3)
		{
			q = gi.argv(1);

			for(i=0; i<strlen(q); i++)
					q[i] = LOWER(q[i]);
			q[i] = '\0';

			if (!strcmp("a", q))
				team = TEAM_A;
			else if (!strcmp("b", q))
				team = TEAM_B;
			else
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: teamname <A or B> <new_name>.\n");
				return;
			}
			p = gi.argv(2);
		}
		else if (gi.argc() == 2)
		{
			team = ent->client->pers.save_data.team;
			if (team == TEAM_NONE)
			{
				gi.cprintf(ent, PRINT_HIGH, "You are not on a team.\n");
				return;
			}
			p = gi.argv(1);
		}
		else
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: teamname [<A or B>] <new_name>.\n");
			return;
		}
		if (strlen(p)>13)
		{
			gi.cprintf(ent, PRINT_HIGH, "Teamname is too long!\n");
			return;
		}
		else if (strlen(p)==0)
		{
			gi.cprintf(ent, PRINT_HIGH, "What is the teamname?\n");
			return;
		}

		for(i=0; i<strlen(p); i++)
		{
			newteamname[j] = p[i];
			j++;
			p[i] = LOWER(p[i]);
		}
		newteamname[j] = '\0';
		admin = true;
	}
	else
	{
		if (game.teamA_captain != ent && game.teamB_captain != ent)
		{
			gi.cprintf(ent, PRINT_HIGH, "Only a team captain may change teamname.\n");
			return;
		}

		if (gi.argc() < 2 && ent->client->pers.save_data.team != TEAM_NONE)
		{
			if (ent->client->pers.save_data.team == TEAM_A)
				gi.cprintf(ent, PRINT_HIGH, "teamname is \"%s\".\n", game.teamA_name);
			else
				gi.cprintf(ent, PRINT_HIGH, "teamname is \"%s\".\n", game.teamB_name);
			return;
		}
		else if (gi.argc() < 2)
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: teamname <name>.\n");
			return;
		}

		team = ent->client->pers.save_data.team;

		p = gi.argv(1);

		if (strlen(p)>13)
		{
			gi.cprintf(ent, PRINT_HIGH, "Teamname is too long!\n");
			return;
		}
		else if (strlen(p)==0)
		{
			gi.cprintf(ent, PRINT_HIGH, "What is the teamname?\n");
			return;
		}

		for(i=0; i<strlen(p); i++)
		{
			newteamname[j] = p[i];
			j++;
			p[i] = LOWER(p[i]);
		}
		newteamname[j] = '\0';

	}

	if (team == TEAM_NONE)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not on a team.\n");
		return;
	}

	if (team == TEAM_A)
	{
		if (!strcmp(p, team_b_name) || !strcmp(p, team_a_name))
		{
			gi.cprintf(ent, PRINT_HIGH, "This name is already in use.\n");
			return;
		}
		else
		{
			strcpy(game.teamA_name, newteamname);
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has changed team A name to %s.\n", ent->client->pers.netname, newteamname );
		}
	}
	else if (team == TEAM_B)
	{
		if (!strcmp(p, team_b_name) || !strcmp(p, team_a_name))
		{
			gi.cprintf(ent, PRINT_HIGH, "This name is already in use.\n");
			return;
		}
		else
		{
			strcpy(game.teamB_name, newteamname);
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has changed team B name to %s.\n", ent->client->pers.netname, newteamname );
		}
	}
}

void Cmd_Teamskin_f(edict_t *ent, qboolean from_function, char *skin)
{
	char	*p;
	char	*q;
	int		team;
	int		i, playernum;
	edict_t	*cl;
	qboolean	admin=false;
	qboolean	free_p = false;

	if (!ent->client)
		return;

	if (((ent->client->pers.save_data.admin_flags & AD_TEAMS) || (ent->client->pers.save_data.admin_flags & AD_ALL)) && ent->client->pers.save_data.is_admin && !from_function)
	{
		if (gi.argc() == 3)
		{
			q = gi.argv(1);

			for(i=0; i<strlen(q); i++)
				q[i] = LOWER(q[i]);
			q[i] = '\0';

			if (gi.argc() < 3)
			{
				if (!strcmp("a", q))
					gi.cprintf(ent, PRINT_HIGH, "teamskin is \"%s\".\n", game.teamA_skin);
				else if (!strcmp("b", q))
					gi.cprintf(ent, PRINT_HIGH, "teamskin is \"%s\".\n", game.teamB_skin);
				else
					gi.cprintf(ent, PRINT_HIGH, "USAGE: teamskin [<A or B>] <skinname>.\n");
				return;
			}

			if (!strcmp("a", q))
				team = TEAM_A;
			else if (!strcmp("b", q))
				team = TEAM_B;
			else
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: teamskin [<A or B>] <skinname>.\n");
				return;
			}

			p = gi.argv(2);
		}
		else if (gi.argc() == 2)
		{
			team = ent->client->pers.save_data.team;
			if (team == TEAM_NONE)
			{
				gi.cprintf(ent, PRINT_HIGH, "You are not on a team.\n");
				return;
			}
			p = gi.argv(1);
		}
		else
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: teamname [<A or B>] <new_name>.\n");
			return;
		}
		if (strlen(p)>15)
		{
			gi.cprintf(ent, PRINT_HIGH, "Skinname is too long!\n");
			return;
		}

		admin = true;
	}
	else if (!from_function)
	{
		if (gi.argc() < 2 && ent->client->pers.save_data.team != TEAM_NONE)
		{
			if (ent->client->pers.save_data.team == TEAM_A)
				gi.cprintf(ent, PRINT_HIGH, "teamskin is \"%s\".\n", game.teamA_skin);
			else
				gi.cprintf(ent, PRINT_HIGH, "teamskin is \"%s\".\n", game.teamB_skin);
			return;
		}
		else if (gi.argc() < 2)
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: teamskin <skinname>.\n");
			return;
		}

		if ( level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH || level.match_state == END )
		{
			gi.cprintf(ent, PRINT_HIGH, "Can't do that during the match!\n");
			return;
		}

		p = gi.argv(1);

		if (strlen(p)>15)
		{
			gi.cprintf(ent, PRINT_HIGH, "Skinname is too long!\n");
			return;
		}

		team = ent->client->pers.save_data.team;
	}
	else
	{
		if (game.teamA_captain != ent && game.teamB_captain != ent)
		{
			gi.cprintf(ent, PRINT_HIGH, "Only a team captain may change teamskin.\n");
			return;
		}

		p = strdup(skin);
		free_p = true;
		team = ent->client->pers.save_data.team;
	}

	if (team == TEAM_NONE)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not on a team.\n");
		if (free_p)
			free(p);
		return;
	}

	if (team == TEAM_A)
	{
		if (!strcmp(p, game.teamB_skin) || !strcmp(p, game.teamA_skin))
		{
			gi.cprintf(ent, PRINT_HIGH, "This skin is already in use.\n");
			if (free_p)
				free(p);
			return;
		}
		else
		{
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has changed team A skin to %s.\n", ent->client->pers.netname, p);
			strcpy(game.teamA_skin, p);
			for (i=0 ; i<game.maxclients ; i++)
			{
				cl = &g_edicts[1+i];

				if (!cl->inuse)
					continue;

				if (cl->client->pers.save_data.team != TEAM_A)
					continue;

				playernum = cl-g_edicts-1;
				gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s", cl->client->pers.netname, game.teamA_skin) );
				Info_SetValueForKey (cl->client->pers.userinfo, "skin", game.teamA_skin);			
			}
		}
	}
	else if (team == TEAM_B)
	{
		if (!strcmp(p, game.teamA_skin) || !strcmp(p, game.teamB_skin))
		{
			gi.cprintf(ent, PRINT_HIGH, "This skin is already in use.\n");
			if (free_p)
				free(p);
			return;
		}
		else
		{
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has changed team B skin to %s.\n", ent->client->pers.netname, p );
			strcpy(game.teamB_skin, p);
			for (i=0 ; i<game.maxclients ; i++)
			{
				cl = &g_edicts[1+i];

				if (!cl->inuse)
					continue;

				if (cl->client->pers.save_data.team != TEAM_B)
					continue;

				playernum = cl-g_edicts-1;
				gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s", cl->client->pers.netname, game.teamB_skin) );
				Info_SetValueForKey (cl->client->pers.userinfo, "skin", game.teamB_skin);
			}
		}
	}
	if (free_p)
		free(p);
}

void Cmd_Lockteam_f(edict_t *ent)
{
	int	team=TEAM_NONE,i;
	char	*p;

	if (!ent->client)
		return;

	if (((ent->client->pers.save_data.admin_flags & AD_TEAMS) || (ent->client->pers.save_data.admin_flags & AD_ALL)) && ent->client->pers.save_data.is_admin)
	{
		if (gi.argc() < 2)
		{
			team = ent->client->pers.save_data.team;
			if (team == TEAM_NONE)
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: lockteam <A or B>.\n");
				return;
			}
		}
		else if (gi.argc() == 2)
		{
			p = gi.argv(1);

			if (strlen(p) > 10)
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: lockteam <A or B>.\n");
				return;
			}

			for (i=0; i<strlen(p); i++)
				p[i] = LOWER(p[i]);
			p[i] = '\0';

			if (!strcmp("a", p))
				team = TEAM_A;
			else if (!strcmp("b", p))
				team = TEAM_B;
			else
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: lockteam <A or B>.\n");
				return;
			}
		}
	}
	else
	{
		if (game.teamA_captain != ent && game.teamB_captain != ent)
		{
			gi.cprintf(ent, PRINT_HIGH, "Only a team captain may lock a team.\n");
			return;
		}
		else
			team = ent->client->pers.save_data.team;
	}

	if (team == TEAM_NONE)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not on a team.\n");
		return;
	}


	if ((team == TEAM_A && game.teamA_locked == true) || (team == TEAM_B && game.teamB_locked == true))
	{
		gi.cprintf(ent, PRINT_HIGH, "Your team is already locked.\n");
		return;
	}

	if (team == TEAM_A)
	{
		game.teamA_locked = true;
		gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has locked team %s.\n", ent->client->pers.netname, game.teamA_name);
	}
	else if (team == TEAM_B)
	{
		game.teamB_locked = true;
		gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has locked team %s.\n", ent->client->pers.netname, game.teamB_name);
	}
}

void Cmd_Unlockteam_f(edict_t *ent)
{
	int	team=TEAM_NONE,i;
	char	*p;

	if (!ent->client)
		return;

	if (((ent->client->pers.save_data.admin_flags & AD_TEAMS) || (ent->client->pers.save_data.admin_flags & AD_ALL)) && ent->client->pers.save_data.is_admin)
	{
		if (gi.argc() < 2)
		{
			team = ent->client->pers.save_data.team;
			if (team == TEAM_NONE)
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: unlockteam <A or B>.\n");
				return;
			}
		}
		else if (gi.argc() == 2)
		{
			p = gi.argv(1);

			if (strlen(p) > 10)
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: unlockteam <A or B>.\n");
				return;
			}
			for (i=0; i<strlen(p); i++)
				p[i] = LOWER(p[i]);
			p[i] = '\0';

			if (!strcmp("a", p))
				team = TEAM_A;
			else if (!strcmp("b", p))
				team = TEAM_B;
			else
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: unlockteam <A or B>.\n");
				return;
			}
		}
	}
	else
	{		
		if (game.teamA_captain != ent && game.teamB_captain != ent)
		{
			gi.cprintf(ent, PRINT_HIGH, "Only a team captain may unlock a team.\n");
			return;
		}
		else
			team = ent->client->pers.save_data.team;
	}

	if (team == TEAM_NONE)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are not on a team.\n");
		return;
	}


	if ((team == TEAM_A && game.teamA_locked == false) || (team == TEAM_B && game.teamB_locked == false))
	{
		gi.cprintf(ent, PRINT_HIGH, "Your team is already unlocked.\n");
		return;
	}

	if (team == TEAM_A)
	{
		game.teamA_locked = false;
		gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has unlocked team %s.\n", ent->client->pers.netname, game.teamA_name);
	}
	else if (team == TEAM_B)
	{
		game.teamB_locked = false;
		gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has unlocked team %s.\n", ent->client->pers.netname, game.teamB_name);
	}
}

void Cmd_Pickplayer_f (edict_t *ent)
{
	char	*p;
	char	*q;
	int		i;
	int		team;
	edict_t	*cl;

	if (!ent->client)
		return;

	if (((ent->client->pers.save_data.admin_flags & AD_TEAMS) || (ent->client->pers.save_data.admin_flags & AD_ALL)) && ent->client->pers.save_data.is_admin)
	{
		if (gi.argc() == 2)
		{
			team = ent->client->pers.save_data.team;
			if (team == TEAM_NONE)
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: pickplayer <A or B> <player name or ID>.\n");
				return;
			}
			p = gi.argv(1);
		}
		else if (gi.argc() == 3)
		{
			q = gi.argv(1);

			for(i=0; i<strlen(q); i++)
				q[i] = LOWER(q[i]);
			q[i] = '\0';

			if (!strcmp("a", q))
				team = TEAM_A;
			else if (!strcmp("b", q))
				team = TEAM_B;
			else
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: pickplayer <A or B> <player name or ID>.\n");
				return;
			}

			p = gi.argv(2);
		}
		else
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: pickplayer <A or B> <player name or ID>.\n");
			return;
		}
	}
	else
	{
		if (game.teamA_captain != ent && game.teamB_captain != ent)
		{
			gi.cprintf(ent, PRINT_HIGH, "Only a team captain may pick players.\n");
			return;
		}

		if (gi.argc() < 2 && ent->client->pers.save_data.team != TEAM_NONE)
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: pickplayer <player name or ID>.\n");
			return;
		}

		if (ent->client->pers.save_data.team == TEAM_NONE)
		{
			gi.cprintf(ent, PRINT_HIGH, "You are not a team member.\n");
			return;
		}

		p = gi.args();

		team = ent->client->pers.save_data.team;
	}

	cl = find_client(ent, p);
	if ( cl && cl->client->pers.mvdspec == false )
	{
		if (cl->client->pers.save_data.team != TEAM_NONE)
		{
			gi.cprintf(ent, PRINT_HIGH, "You can't pick this player!\n");
			return;
		}

		cl->client->pers.save_data.team = team;



		gi.bprintf (ATTR_PRINT_HIGH|ATTR_BOLD, "%s was picked by %s.\n", cl->client->pers.netname, ent->client->pers.netname);
		if (cl->client->pers.save_data.team == TEAM_B)
		{
			Write_ConfigString2(cl);
			Info_SetValueForKey (cl->client->pers.userinfo, "skin", game.teamB_skin);
		}
		else
		{
			Write_ConfigString2(cl);
			Info_SetValueForKey (cl->client->pers.userinfo, "skin", game.teamA_skin);
		}
		Info_SetValueForKey(cl->client->pers.userinfo, "spectator", "0");
		respawn(cl, false, true);
		CheckSavedPlayer(cl);
	}
	else
		gi.cprintf(ent, PRINT_HIGH, "Invalid player name or ID.\n");
}

void Cmd_Kickplayer_f (edict_t *ent)
{
	char	*p;
	edict_t	*cl;

	if (!ent->client)
		return;

	if (((ent->client->pers.save_data.admin_flags & AD_TEAMS) || (ent->client->pers.save_data.admin_flags & AD_ALL)) && ent->client->pers.save_data.is_admin)
	{
		if (gi.argc() < 2)
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: kickplayer <player name or ID>.\n");
			return;
		}

		p = gi.argv(1);
	}
	else
	{
		if (game.teamA_captain != ent && game.teamB_captain != ent)
		{
			gi.cprintf(ent, PRINT_HIGH, "Only the team captain may kick players.\n");
			return;
		}

		if (gi.argc() < 2 && ent->client->pers.save_data.team != TEAM_NONE)
		{
			gi.cprintf(ent, PRINT_HIGH, "USAGE: kickplayer <player name or ID>.\n");
			return;
		}

		if (ent->client->pers.save_data.team == TEAM_NONE)
		{
			gi.cprintf(ent, PRINT_HIGH, "You are not a team member.\n");
			return;
		}

		p = gi.argv(1);
	}

	cl = find_client(ent, p);
	if (cl)
	{
		if (cl->client->pers.save_data.team == TEAM_NONE)
		{
			gi.cprintf(ent, PRINT_HIGH, "You can't kick this player!\n");
			return;
		}

		if ((cl->client->pers.save_data.team != ent->client->pers.save_data.team) && !ent->client->pers.save_data.is_admin)
		{
			gi.cprintf(ent, PRINT_HIGH, "The player is not on your team!\n");
			return;
		}	

		gi.bprintf (ATTR_PRINT_HIGH|ATTR_BOLD, "%s was kicked from team by %s.\n", cl->client->pers.netname, ent->client->pers.netname);
		SpectateOrChase(cl, false);
	}
	else
		gi.cprintf(ent, PRINT_HIGH, "Invalid player name or ID.\n");
}

/*
** 1.F - Misc
*/

void Cmd_Menu_f(edict_t *ent)
{
	Cmd_PutAway_f(ent);
	if (!ent->client->showmenu)
	{
		ent->client->showmenu = true;
		menu_display(ent, 0);
	}
	else
		ent->client->showmenu = false;
}

void Cmd_Vote_Stats_f(edict_t *ent, int index)
{
	char	*p;
	int		value;
	int		i;
	qboolean	is_admin=false;

	if (ent->client->pers.save_data.is_admin && ((ent->client->pers.save_data.admin_flags & AD_ALL) || (ent->client->pers.save_data.admin_flags & AD_MATCH)))
		is_admin = true;

	if (index == 0)
	{
		if (!is_admin)
			gi.cprintf(ent, PRINT_HIGH, "BFG is %d.\n", (int)allow_bfg->value);
		else
		{
			edict_t	*ento;
			gitem_t	*it;

			if (gi.argc() < 2)
			{
				gi.cprintf(ent, PRINT_HIGH, "BFG is %d.\n", (int)allow_bfg->value);
				return;
			}
			p = gi.argv(1);
			value = atoi(p);

			gi.cvar_set("allow_bfg", p);
			Log(ent, LOG_ADMIN, "bfg");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Allow BFG changed to %s by %s.\n", p, ent->client->pers.netname);
			for (i=1, ento=g_edicts+i; i < globals.num_edicts; i++,ento++)
			{
				if (!ento->item)
					continue;

				if (!strcmp(ento->item->pickup_name, "BFG10K"))
				{
					ento->s.event = EV_ITEM_RESPAWN;
					if (value == 0)
					{
						if (ento->spawnflags & DROPPED_ITEM)
							G_FreeEdict(ento);
						else
							SetRespawn (ento, 1);
					}
					else
					{
						if (ento->spawnflags & DROPPED_ITEM)
							continue;		
						else
							SetRespawn (ento, 1);
					}
				}
			}
			if (value == 0)
			{
				for (i=0 ; i<game.maxclients ; i++)
				{
					ento = &g_edicts[1+i];
					if (!ento->inuse)
						continue;
					
					it = FindItem("BFG10K");
					if (ento->client->pers.inventory[ITEM_INDEX(it)] > 0)
					{
						ento->client->pers.selected_item = ITEM_INDEX(FindItem("Blaster"));
						ento->client->pers.weapon = FindItem("Blaster");
						ento->client->newweapon = ento->client->pers.weapon;
						ChangeWeapon (ento);
						ento->client->pers.inventory[ITEM_INDEX(it)] = 0;
					}
				}
			}
		}
	}
	else if (index == 1)
	{
		if (!is_admin)
			gi.cprintf(ent, PRINT_HIGH, "TIMELIMIT is %d.\n", (int)timelimit->value);
		else
		{
			if (gi.argc() < 2)
			{
				gi.cprintf(ent, PRINT_HIGH, "TIMELIMIT is %d.\n", (int)timelimit->value);
				return;
			}

			p = gi.argv(1);

			gi.cvar_set("timelimit", p);

			Log(ent, LOG_ADMIN, "timelimit");

			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Timelimit changed to %s by %s.\n", p, ent->client->pers.netname);

			if (level.match_state == MATCH || level.match_state == OVERTIME)
			{
				level.countdown_framenum = level.framenum + (int)timelimit->value*600;
				level.start_framenum = level.framenum;
			}
		}
	}
	else if (index == 2)
	{
		if (!is_admin)
			gi.cprintf(ent, PRINT_HIGH, "POWERUPS are %d.\n", (int)allow_powerups->value);
		else
		{
			edict_t	*ento;
			gitem_t	*it;

			if (gi.argc() < 2)
			{
				gi.cprintf(ent, PRINT_HIGH, "POWERUPS are %d.\n", (int)allow_powerups->value);
				return;
			}
			p = gi.argv(1);
			value = atoi(p);

			gi.cvar_set("allow_powerups", p);
			Log(ent, LOG_ADMIN, "powerups");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Allow powerups changed to %s by %s.\n", p, ent->client->pers.netname);
			for (i=1, ento=g_edicts+i; i < globals.num_edicts; i++,ento++)
			{
				if (!ento->item)
					continue;

				if (!strcmp(ento->item->pickup_name, "Power Shield") || !strcmp(ento->item->pickup_name, "Quad Damage")  || !strcmp(ento->item->pickup_name, "Invulnerability"))
				{
					ento->s.event = EV_ITEM_RESPAWN;
					if (value == 0)
					{
						if (ento->spawnflags & DROPPED_ITEM)
							G_FreeEdict(ento);
						else
							SetRespawn (ento, 1);
					}
					else
					{
						if (ento->spawnflags & DROPPED_ITEM)
							continue;		
						else
							SetRespawn (ento, 1);
					}
				}
			}
			if (value == 0)
			{
				for (i=0 ; i<game.maxclients ; i++)
				{
					ento = &g_edicts[1+i];
					if (!ento->inuse)
						continue;

					if (ento->client->invincible_framenum > level.framenum)
						ento->client->invincible_framenum = 0;

					if (ento->client->quad_framenum > level.framenum)
						ento->client->quad_framenum = 0;

					it = FindItem("Power Shield");
					if (ento->client->pers.inventory[ITEM_INDEX(it)] > 0)
					{
						ento->client->pers.inventory[ITEM_INDEX(it)] = 0;
						ValidateSelectedItem (ento);
					}
					it = FindItem("Quad Damage");
					if (ento->client->pers.inventory[ITEM_INDEX(it)] > 0)
					{
						ento->client->pers.inventory[ITEM_INDEX(it)] = 0;
						ValidateSelectedItem (ento);
					}
					it = FindItem("Invulnerability");
					if (ento->client->pers.inventory[ITEM_INDEX(it)] > 0)
					{
						ento->client->pers.inventory[ITEM_INDEX(it)] = 0;
						ValidateSelectedItem (ento);
					}
				}
			}
		}
	}
	else if (index == 3)		
	{
		if (!is_admin)
			gi.cprintf(ent, PRINT_HIGH, "FASTWEAPONS are %d.\n", (int)fastweapons->value);
		else
		{
			if (gi.argc() < 2)
			{
				gi.cprintf(ent, PRINT_HIGH, "FASTWEAPONS are %d.\n", (int)fastweapons->value);
				return;
			}

			p = gi.argv(1);

			if (strcmp(p, "0") && strcmp(p, "1"))
			{
				gi.cprintf(ent, PRINT_HIGH, "FASTWEAPONS are %d.\n", (int)fastweapons->value);
				return;
			}
			gi.cvar_set("fastweapons", p);

			Log(ent, LOG_ADMIN, "fastweaps");

			if (atoi(p) == 1)
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Fastweapons enabled by %s.\n", ent->client->pers.netname);
			else
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Fastweapons disabled by %s.\n", ent->client->pers.netname);
		}
	}
	else if (index == 4)
	{
		if (!is_admin)
			gi.cprintf(ent, PRINT_HIGH, "GIBS are %d.\n", (int)allow_gibs->value);
		else
		{
			if (gi.argc() < 2)
			{
				gi.cprintf(ent, PRINT_HIGH, "GIBS are %d.\n", (int)allow_gibs->value);
				return;
			}

			p = gi.argv(1);

			if (strcmp(p, "0") && strcmp(p, "1") && strcmp(p, "2"))
			{
				gi.cprintf(ent, PRINT_HIGH, "GIBS are %d.\n", (int)allow_gibs->value);
				return;
			}
			gi.cvar_set("allow_gibs", p);

			Log(ent, LOG_ADMIN, "gibs");

			if (atoi(p) == 2)
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Extra gibs enabled by %s.\n", ent->client->pers.netname);
			else if (atoi(p) == 1)
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Gibs enabled by %s.\n", ent->client->pers.netname);
			else
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "Gibs disabled by %s.\n", ent->client->pers.netname);
		}
	}
	else if (index == 5)
	{
		if (!is_admin || (is_admin && gi.argc() < 2))
		{
			if (game.tp == 1)
				gi.cprintf(ent, PRINT_HIGH, "TP is 1 (Players cannot cause damage to themselves, their teammates, their armor, or their teammates' armor.)\n");
			else if (game.tp == 2)
				gi.cprintf(ent, PRINT_HIGH, "TP is 2 (No self or teammate health or armor protection. A player's frag total is decremented by 1 for each teammate kill.)\n");
			else if (game.tp == 3)
				gi.cprintf(ent, PRINT_HIGH, "TP is 3 (Players can cause damage to themselves and their own armor but can only damage teammates' armor.)\n");
			else if (game.tp == 4)
				gi.cprintf(ent, PRINT_HIGH, "TP is 4 (Players can cause damage to themselves and their own armor but cannot cause damage to teammates or their armor.)\n");
			return;
		}
		else
		{
			int		old_dmflags;
			char	new_dmflags[16];

			p = gi.argv(1);

			if (strcmp(p, "1") && strcmp(p, "2") && strcmp(p, "3") && strcmp(p, "4"))
			{
				gi.cprintf(ent, PRINT_HIGH, "Usage: tp <1|2|3|4>\n");
				return;
			}
			game.tp = atoi(p);

			Log(ent, LOG_ADMIN, "tp");

			new_dmflags[0]='\0';

			old_dmflags = (int)dmflags->value;

			if (game.tp == 1 || game.tp == 4)
				old_dmflags |= 256;
			else if ((game.tp == 2 || game.tp == 3) && (old_dmflags & 246))
				old_dmflags &= ~256;

			sprintf(new_dmflags, "%d", old_dmflags);
			gi.cvar_set("dmflags", new_dmflags);

			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_ORANGE, "TP changed to %s by %s.\n", p, ent->client->pers.netname);
		}
	}
}

void Cmd_Hold_f (edict_t *ent)
{
	qboolean	is_admin=false;
	int			i;
	edict_t		*cl;

	if (ent->client->pers.save_data.is_admin && ((ent->client->pers.save_data.admin_flags & AD_ALL) || (ent->client->pers.save_data.admin_flags & AD_MATCH)))
		is_admin=true;


	if (level.match_state != MATCH && level.match_state != OVERTIME && level.match_state != SUDDENDEATH)
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't do that now.\n");
		return;
	}

	if (!level.paused)
	{
		if (is_admin)
		{
			level.pause_time = -1;
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Game paused by %s.\n", ent->client->pers.netname);
			level.pauser = ent;
			level.paused = true;
			level.pauserType = PAUSER_ADMIN;
			Log(ent, LOG_ADMIN, "hold");

			for (i=0; i<game.maxclients; i++)
			{
				cl = &g_edicts[i+1];

				if (!cl->inuse)
					continue;

				gi.WriteByte (svc_stufftext);
				gi.WriteString ("play world/fusein.wav\n");
				gi.unicast(cl, true);
			}
		}
	}
	else if ((ent == level.pauser || is_admin) && (level.pause_time > GAMESECONDS(3) || level.pause_time == -1))
	{
		gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Game was resumed by %s.\n", ent->client->pers.netname);
		level.pause_time = GAMESECONDS(4);
	}
	else if (level.pauser)
		gi.cprintf(ent, PRINT_HIGH, "Game is already paused by %s.\n", level.pauser->client->pers.netname);
}

void Cmd_Time_f (edict_t *ent)
{
	qboolean	is_admin=false;
	qboolean	is_admin_in_game=false;
	edict_t		*cl;
	char		*time;
	int			left=0,i;

	for (i=0; i<game.maxclients; i++)
	{
		if (g_edicts[i+1].inuse && g_edicts[i+1].client)
		{
			if (g_edicts[i+1].client->pers.save_data.is_admin && ((g_edicts[i+1].client->pers.save_data.admin_flags & AD_ALL) || (g_edicts[i+1].client->pers.save_data.admin_flags & AD_MATCH)))
			{
				is_admin_in_game = true;
				break;
			}
		}
	}

	if (ent->client->pers.save_data.is_admin && ((ent->client->pers.save_data.admin_flags & AD_ALL) || (ent->client->pers.save_data.admin_flags & AD_MATCH)))
		is_admin = true;

	if (ent != game.teamA_captain && ent != game.teamB_captain && !is_admin)
	{
		gi.cprintf(ent, PRINT_HIGH, "Only team captain or admin can use this command.\n");
		return;
	}

	if (level.match_state != MATCH && level.match_state != OVERTIME && level.match_state != SUDDENDEATH)
	{
		gi.cprintf(ent, PRINT_HIGH, "You can't do that now.\n");
		return;
	}

	if (!level.paused)
	{
		if (is_admin && gi.argc() >= 2)
		{
			time = gi.argv(1);
			if (atoi(time) < 0)
			{
				gi.cprintf(ent, PRINT_HIGH, "USAGE: time <0...>.\n");
				return;
			}
			if (atoi(time) == 0)
				level.pause_time = -1;
			else
				level.pause_time = GAMESECONDS(atoi(time));
			level.pauserType = PAUSER_ADMIN;
		}
		else if (is_admin)
		{
			Cmd_Hold_f(ent);
			return;
		}
		else if (timelimit->value >= 15)
		{
			if ((ent == game.teamA_captain && (level.teamA_numpauses < 3 || (level.teamA_numpauses < 1 && is_admin_in_game))) || (ent == game.teamB_captain && (level.teamA_numpauses < 3 || (level.teamB_numpauses < 1 && is_admin_in_game))))
			{
				level.pause_time = GAMESECONDS(60);
				if (ent == game.teamA_captain)
				{
					level.pauserType = PAUSER_ACAPTAIN;
					level.teamA_numpauses++;
					left = 3 - level.teamA_numpauses;
				}
				else
				{
					level.pauserType = PAUSER_BCAPTAIN;
					level.teamB_numpauses++;
					left = 3 - level.teamB_numpauses;
				}
			}
			else
			{
				gi.cprintf(ent, PRINT_HIGH, "Time out call limit reached.\n");
				return;
			}
		}
		else
		{
			gi.cprintf(ent, PRINT_HIGH, "You can timeout only in clanwars.\n");
			return;
		}

		if (level.pause_time == -1)
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Game paused by %s.\n", ent->client->pers.netname);
		else if (is_admin)
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Time out called by %s.\n", ent->client->pers.netname);
		else
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Time out called by %s. (%d calls left)\n", ent->client->pers.netname, left);
		level.pauser = ent;
		level.paused = true;

		for (i=0; i<game.maxclients; i++)
		{
			cl = &g_edicts[i+1];

			if (!cl->inuse)
				continue;

			gi.WriteByte (svc_stufftext);
			gi.WriteString ("play world/fusein.wav\n");
			gi.unicast(cl, true);
		}
	}
	else if (level.pause_time > GAMESECONDS(3) || level.pause_time == -1)
	{
		if ((level.pauserType == PAUSER_ACAPTAIN && ent == game.teamA_captain) || (level.pauserType == PAUSER_BCAPTAIN && ent == game.teamB_captain) || is_admin)
		{
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Game was resumed by %s.\n", ent->client->pers.netname);
			level.pause_time = GAMESECONDS(4);
		}
	}
	else if (level.pauser)
		gi.cprintf(ent, PRINT_HIGH, "Game is already paused by %s.\n", level.pauser->client->pers.netname);

}

void Reset_Accuracy(edict_t *ent)
{
	int	i;
	client_save_data_t *cl;

	cl = &ent->client->pers.save_data;

	for (i=0; i<11; i++)
		cl->weaps[i].hits = cl->weaps[i].shots = cl->weaps[i].dmg = cl->weaps[i].dmr = cl->weaps[i].kills = cl->weaps[i].deaths = 0;

	cl->quaddamage = cl->invulnerability = cl->megahealth = cl->ammopack = cl->ja = cl->ca = cl->ba = cl->ps = cl->adrenaline =0;
}

void Cmd_Items_f (edict_t *ent)
{
	edict_t	*cl=NULL;
	char	*p;
	client_save_data_t *save_data;
	gclient_t *cli;
	int		qd_s=0, iv_s=0, mh_s=0, ja_s=0, ca_s=0, ba_s=0, ps_s=0, i;
	int		qd_o=0, iv_o=0, mh_o=0, ja_o=0, ca_o=0, ba_o=0, ps_o=0, eff, eff1;


	if (gi.argc() < 2)
		cl = ent;
	else
	{
		p = gi.argv(1);

		cl = find_client(ent, p);
	}

	if (cl)
	{
		save_data = &cl->client->pers.save_data;

		cli = cl->client;

		if (cl->client->pers.save_data.team == TEAM_NONE)
		{
			gi.cprintf(ent, PRINT_HIGH, "\nNo items info for %s.\n", cl->client->pers.netname);
			return;
		}
		else
			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "\nItems info for %s:\n\nItem            | %-14s | %-14s\n----------------+----------------+----------------\n",
			cli->pers.netname,
			save_data->team == TEAM_A ? game.teamA_name : game.teamB_name, save_data->team == TEAM_A ? game.teamB_name : game.teamA_name);
		for (i=0; i<game.maxclients; i++)
		{
			if (g_edicts[i+1].inuse == false)
				continue;

			if (level.items & IS_MH)
			{
				if (g_edicts[i+1].client->pers.save_data.team == cl->client->pers.save_data.team)
					mh_s += g_edicts[i+1].client->pers.save_data.megahealth;
				else
					mh_o += g_edicts[i+1].client->pers.save_data.megahealth;
			}
			if (level.items & IS_PS)
			{
				if (g_edicts[i+1].client->pers.save_data.team == cl->client->pers.save_data.team)
					ps_s += g_edicts[i+1].client->pers.save_data.ps;
				else
					ps_o += g_edicts[i+1].client->pers.save_data.ps;
			}
			if (level.items & IS_BA)
			{
				if (g_edicts[i+1].client->pers.save_data.team == cl->client->pers.save_data.team)
					ba_s += g_edicts[i+1].client->pers.save_data.ba;
				else
					ba_o += g_edicts[i+1].client->pers.save_data.ba;
			}
			if (level.items & IS_CA)
			{
				if (g_edicts[i+1].client->pers.save_data.team == cl->client->pers.save_data.team)
					ca_s += g_edicts[i+1].client->pers.save_data.ca;
				else
					ca_o += g_edicts[i+1].client->pers.save_data.ca;
			}
			if (level.items & IS_JA)
			{
				if (g_edicts[i+1].client->pers.save_data.team == cl->client->pers.save_data.team)
					ja_s += g_edicts[i+1].client->pers.save_data.ja;
				else
					ja_o += g_edicts[i+1].client->pers.save_data.ja;
			}
			if (level.items & IS_QD)
			{
				if (g_edicts[i+1].client->pers.save_data.team == cl->client->pers.save_data.team)
					qd_s += g_edicts[i+1].client->pers.save_data.quaddamage;
				else
					qd_o += g_edicts[i+1].client->pers.save_data.quaddamage;
			}
			if (level.items & IS_IV)
			{
				if (g_edicts[i+1].client->pers.save_data.team == cl->client->pers.save_data.team)
					iv_s += g_edicts[i+1].client->pers.save_data.invulnerability;
				else
					iv_o += g_edicts[i+1].client->pers.save_data.invulnerability;
			}
		}
		if (level.items & IS_QD)
		{
			if (qd_o > 0)
			    eff1 = (qd_o*100)/(qd_s+qd_o);
			else
			    eff1 = 0;
			if (qd_s > 0)
			    eff = (qd_s*100)/(qd_s+qd_o);
			else
			    eff = 0;
			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Quad Damage     | (%2d) %2d - %3d%c%c | %2d - %3d%c%c\n",save_data->quaddamage, qd_s, eff, 37, 37, qd_o, eff1, 37, 37);
		}
		if (level.items & IS_IV)
		{
			if (iv_o > 0)
			    eff1 = (iv_o*100)/(iv_s+iv_o);
			else
			    eff1 = 0;
			if (iv_s > 0)
			    eff = (iv_s*100)/(iv_s+iv_o);
			else
			    eff = 0;

			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Invulnerability | (%2d) %2d - %3d%c%c | %2d - %3d%c%c\n",save_data->invulnerability, iv_s, eff, 37, 37, iv_o, eff1, 37, 37);
		}
		if (level.items & IS_PS)
		{
			if (ps_o > 0)
			    eff1 = (ps_o*100)/(ps_s+ps_o);
			else
			    eff1 = 0;
			if (ps_s > 0)
			    eff = (ps_s*100)/(ps_s+ps_o);
			else
			    eff = 0;

			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Power Shield    | (%2d) %2d - %3d%c%c | %2d - %3d%c%c\n",save_data->ps, ps_s, eff, 37, 37, ps_o, eff1, 37, 37);
		}
		if (level.items & IS_BA)
		{
			if (ba_o > 0)
			    eff1 = (ba_o*100)/(ba_s+ba_o);
			else
			    eff1 = 0;
			if (ba_s > 0)
			    eff = (ba_s*100)/(ba_s+ba_o);
			else
			    eff = 0;

			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Body Armor      | (%2d) %2d - %3d%c%c | %2d - %3d%c%c\n",save_data->ba, ba_s, eff, 37, 37, ba_o, eff1, 37, 37);
		}
		if (level.items & IS_MH)
		{
			if (mh_o > 0)
			    eff1 = (mh_o*100)/(mh_s+mh_o);
			else
			    eff1 = 0;
			if (mh_s > 0)
			    eff = (mh_s*100)/(mh_s+mh_o);
			else
			    eff = 0;

			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Mega Health     | (%2d) %2d - %3d%c%c | %2d - %3d%c%c\n",save_data->megahealth, mh_s, eff, 37, 37, mh_o, eff1, 37, 37);
		}
		if (level.items & IS_CA)
		{
			if (ca_o > 0)
			    eff1 = (ca_o*100)/(ca_s+ca_o);
			else
			    eff1 = 0;
			if (ca_s > 0)
			    eff = (ca_s*100)/(ca_s+ca_o);
			else
			    eff = 0;

			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Combat Armor    | (%2d) %2d - %3d%c%c | %2d - %3d%c%c\n",save_data->ca, ca_s, eff, 37, 37, ca_o, eff1, 37, 37);
		}
		if (level.items & IS_JA)
		{
			if (ja_o > 0)
			    eff1 = (ja_o*100)/(ja_s+ja_o);
			else
			    eff1 = 0;
			if (ja_s > 0)
			    eff = (ja_s*100)/(ja_s+ja_o);
			else
			    eff = 0;

			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Jacket Armor    | (%2d) %2d - %3d%c%c | %2d - %3d%c%c\n",save_data->ja, ja_s, eff, 37, 37, ja_o, eff1, 37, 37);
		}
		MyCPrintfEnd(ent, CPRINTF, PRINT_HIGH);
	}
}

void Cmd_Accuracy_f (edict_t *ent, qboolean from_function)
{
	int		accuracy, i;
	edict_t	*cl=NULL;
	char	*p;
	qboolean shot=false;
	client_save_data_t *save_data;
	int		damg=0, damr=0;


	if (!from_function)
	{
		if (gi.argc() < 2)
			cl = ent;
		else
		{
			p = gi.argv(1);

			cl = find_client(ent, p);
		}
	}
	else
		cl = ent;

	if (cl)
	{
		save_data = &cl->client->pers.save_data;

		for (i=0; i<11; i++)
		{
			if(save_data->weaps[i].shots > 0 || save_data->weaps[i].dmr > 0)
			{
				shot = true;
				break;
			}
		}
		if (shot)
		{
			MyCPrintf(ent, CPRINTF, PRINT_HIGH, "\nAccuracy info for %s:\n\nWeapon           |  acc | shots | hits | kills | deaths | dmg g | dmg r\n-----------------+------+-------+------+-------+--------+-------+-------\n", cl->client->pers.netname);
			for (i=0; i<11; i++)
			{
				if (save_data->weaps[i].shots > 0 || save_data->weaps[i].dmr > 0)
				{
					if (save_data->weaps[i].shots == 0)
						accuracy = 0;
					else
						accuracy = (save_data->weaps[i].hits*100)/save_data->weaps[i].shots;

					MyCPrintf(ent, CPRINTF, PRINT_HIGH, "%-16s | %3d%c%c | %5d | %4d | %5d | %6d | %5d | %5d\n", itemlist[7+i].pickup_name, accuracy, 37, 37, save_data->weaps[i].shots, save_data->weaps[i].hits, save_data->weaps[i].kills, save_data->weaps[i].deaths, save_data->weaps[i].dmg, save_data->weaps[i].dmr);
					damg += save_data->weaps[i].dmg;
					damr += save_data->weaps[i].dmr;
				}
			}
			if (damg || damr)
			{
				MyCPrintf(ent, CPRINTF, PRINT_HIGH, "\nDamage: %10d/%-5d\n", damg, damr);
			}
			if (save_data->team_damg || save_data->team_damr)
			{
				MyCPrintf(ent, CPRINTF, PRINT_HIGH, "Team Damage: %5d/%-5d\n", save_data->team_damg, save_data->team_damr);
			}
			MyCPrintfEnd(ent, CPRINTF, PRINT_HIGH);
		}
		else
			gi.cprintf(ent, PRINT_HIGH, "%s didn't shoot a thing!\n", cl->client->pers.netname);
	}
	else
		gi.cprintf(ent, PRINT_HIGH, "Player not found.\n");
}

void Cmd_Sets_f (edict_t *ent)
{
	char	string[1400];

	ent->client->showscores = false;
	ent->client->showoldscore = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
	ent->client->showmenu = false;

	if (!ent->client->showsets)
		ent->client->showsets = true;
	else
	{
		ent->client->showsets = false;
		if (ent->client->chase_target)
			ent->client->update_chase = true;
		return;
	}

	Com_sprintf (string, sizeof(string), "xv 64 yv 8 string2 \"%s vs %s\" yv 16 xv 64 string \"%s:\" xv %i string2 \"%s\" yv 24 xv 64 string \"%s:\" xv %i string2 \"%s\" xv 64 yv 32 string \"Map: %s\" xv %i string2 \"(%s)\" xv 64 yv 40 string \"DMflags:\" xv 132 string2 \"%i\" xv 64 yv 48 string \"Timelimit:\" xv 152 string2 \"%i\" xv 64 yv 56 string \"Tp:\" xv 96 string2 \"%i\" xv 64 yv 64 string \"Powerups:\" xv 144 string2 \"%i\" xv 64 yv 72 string \"Bfg:\" xv 104 string2 \"%i\" xv 64 yv 88 string \"Good luck and have fun!\"", game.teamA_name, game.teamB_name, game.teamA_name, 16+64+strlen(game.teamA_name)*8, game.teamA_skin, game.teamB_name, 16+64+strlen(game.teamB_name)*8, game.teamB_skin, level.mapname, 64+48+strlen(level.mapname)*8, level.level_name, (int)dmflags->value, (int)timelimit->value, game.tp, (int)allow_powerups->value, (int)allow_bfg->value);

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
	gi.unicast (ent, false);
}

void Cmd_Chase_f(edict_t *ent)
{
	if (ent->client->pers.save_data.team == TEAM_NONE)
	{
		if (ent->client->chase_target && ent->client->chase_target->inuse)
			ChaseNext(ent);
		else
			GetChaseTarget(ent);
	}
	else
		gi.cprintf(ent, PRINT_HIGH, "You must leave team first.\n");
}

/*
** 2.A - Update_ServerInfo
*/

void Update_ServerInfo(void)
{
	char	text[32];
	int		mins, secs;

	if (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH)
	{
		mins = (level.countdown_framenum - level.framenum)/600;
		secs = ((level.countdown_framenum - level.framenum)-(mins*600))/10;

		Com_sprintf(text, sizeof(text), "%d", level.teamA_score);
		if (strlen(score_a->string) && strcmp(score_a->string, text))
			gi.cvar_set("#Score_A", text);

		Com_sprintf(text, sizeof(text), "%d", level.teamB_score);
		if (strlen(score_b->string) && strcmp(score_b->string, text))
			gi.cvar_set("#Score_B", text);

		Com_sprintf(text, sizeof(text), "%d:%02d", mins, secs);
		if (strlen(timeleft->string) && strcmp(timeleft->string, text))
			gi.cvar_set("#Time_Left", text);
	}
	else if (level.match_state == WARMUP)
	{
		if (strlen(timeleft->string) && strcmp(timeleft->string, "WARMUP"))
			gi.cvar_set("#Time_Left", "WARMUP");
		if (strlen(score_a->string) && strcmp(score_a->string, "WARMUP"))
			gi.cvar_set("#Score_A", "WARMUP");
		if (strlen(score_b->string) && strcmp(score_b->string, "WARMUP"))
			gi.cvar_set("#Score_B", "WARMUP");
	}
	else if (level.match_state == PREGAME)
	{
		if (strlen(timeleft->string) && strcmp(timeleft->string, "PREGAME"))
			gi.cvar_set("#Time_Left", "PREGAME");
		if (strlen(score_a->string) && strcmp(score_a->string, "PREGAME"))
			gi.cvar_set("#Score_A", "PREGAME");
		if (strlen(score_b->string) && strcmp(score_b->string, "PREGAME"))
			gi.cvar_set("#Score_B", "PREGAME");
	}
	else if (level.match_state == COUNTDOWN)
	{
		if (strlen(timeleft->string) && strcmp(timeleft->string, "COUNTDOWN"))
			gi.cvar_set("#Time_Left", "COUNTDOWN");
		if (strlen(score_a->string) && strcmp(score_a->string, "COUNTDOWN"))
			gi.cvar_set("#Score_A", "COUNTDOWN");
		if (strlen(score_b->string) && strcmp(score_b->string, "COUNTDOWN"))
			gi.cvar_set("#Score_B", "COUNTDOWN");
	}
	else if (level.match_state == END)
	{
		Com_sprintf(text, sizeof(text), "%d", level.teamA_score);
		if (strlen(score_a->string) && strcmp(score_a->string, text))
			gi.cvar_set("#Score_A", text);

		Com_sprintf(text, sizeof(text), "%d", level.teamB_score);
		if (strlen(score_b->string) && strcmp(score_b->string, text))
			gi.cvar_set("#Score_B", text);

		Com_sprintf(text, sizeof(text), "Winner: %s", level.teamA_score > level.teamB_score ? game.teamA_name : game.teamB_name);
		if (strlen(timeleft->string) && strcmp(timeleft->string, text))
			gi.cvar_set("#Time_Left", text);
	}
}

void VoteFailed( void )
{
	int i;
	edict_t	*ent;

	gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "Vote failed.\n");

	for (i=0 ; i<game.maxclients ; i++)
	{
		ent = &g_edicts[1+i];
		if (!ent->inuse)
			continue;

		ent->client->pers.save_data.vote_yon = VOTE_NONE;
		ent->client->pers.save_data.vote_change_count = 0;
	}

	memset(&level.vote, 0, sizeof(vote_struct));
}

void VotePassed( void )
{
	gitem_t	*it;
	edict_t *ent;
	int i;

	MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "Vote passed.\n");

	if (level.vote.vote_what & VOTE_KICK)
	{
		if (g_edicts[level.vote.kick].inuse)
		{
			char text[80];
			sprintf(text, "kick %d\n", level.vote.kick);
			gi.AddCommandString(text);
		}
	}

	if (level.vote.vote_what & VOTE_TIMELIMIT)
	{
		char text[33];

		Com_sprintf(text, sizeof(text), "%d", level.vote.timelimit);
		gi.cvar_set("timelimit", text);

		if (level.match_state == MATCH || level.match_state == OVERTIME)
		{
			level.countdown_framenum = level.framenum + (int)timelimit->value*600;
			level.start_framenum = level.framenum;
		}

		MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "Timelimit changed to %d.\n", level.vote.timelimit);
	}

	if (level.vote.vote_what & VOTE_FASTWEAPONS)
	{
		char text[33];

		Com_sprintf(text, sizeof(text), "%d", level.vote.fastweapons);
		gi.cvar_set("fastweapons", text);

		MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "Fastweapons changed to %d.\n", level.vote.fastweapons);
	}

	if (level.vote.vote_what & VOTE_DMFLAGS)
	{
		int		old_dmflags;
		char	new_dmflags[16];
		qboolean	enableflag=true;

		for (i=1; i<NUM_DMFLAGS; i++)
		{
			if (dmflags_table[i].flags == level.vote.dmflags)
				break;
		}

		new_dmflags[0]='\0';

		old_dmflags = (int)dmflags->value;

		if (old_dmflags & level.vote.dmflags)
		{
			old_dmflags &= ~level.vote.dmflags;
			enableflag = false;
		}
		else
		{
			if ( level.vote.dmflags == DF_SPAWN_NEWTDM )
				old_dmflags &= ~DF_SPAWN_FARTHEST;
			else if ( level.vote.dmflags == DF_SPAWN_FARTHEST )
				old_dmflags &= ~DF_SPAWN_NEWTDM;
			old_dmflags |= level.vote.dmflags;
		}

		sprintf(new_dmflags, "%d", old_dmflags);
		gi.cvar_set("dmflags", new_dmflags);

		if (enableflag)
			MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "Dmflag %s is ENABLED. Current dmflags are: %s.\n", dmflags_table[i].name, new_dmflags);
		else
			MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "Dmflag %s is DISABLED. Current dmflags are: %s.\n", dmflags_table[i].name, new_dmflags);
		if ((level.vote.dmflags & 1) || (level.vote.dmflags & 2) || (level.vote.dmflags & 2048))
			MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "DMFLAGS will be changed for the next game.\n");
	}

	if (level.vote.vote_what & VOTE_TP)
	{
		int		old_dmflags;
		char	new_dmflags[16];

		new_dmflags[0]='\0';

		old_dmflags = (int)dmflags->value;

		game.tp = level.vote.tp;
		if (game.tp == 1 || game.tp == 4)
			old_dmflags |= 256;
		else if ((game.tp == 2 || game.tp == 3) && (old_dmflags & 256))
			old_dmflags &= ~256;

		sprintf(new_dmflags, "%d", old_dmflags);
		gi.cvar_set("dmflags", new_dmflags);
		MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "TP mode changed to %d.\n", game.tp);
	}

	if (level.vote.vote_what & VOTE_BFG)
	{
		char text[33];

		Com_sprintf(text, sizeof(text), "%d", level.vote.bfg);
		gi.cvar_set("allow_bfg", text);

		MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "BFG changed to %d.\n", level.vote.bfg);

		for (i=1, ent=g_edicts+i; i < globals.num_edicts; i++,ent++)
		{
			if (!ent->item)
				continue;

			if (!strcmp(ent->item->pickup_name, "BFG10K"))
			{
				if (level.vote.bfg == 0)
				{
					if (ent->spawnflags & DROPPED_ITEM)
						G_FreeEdict(ent);
					else
						SetRespawn (ent, 1);
				}
				else
				{
					if (ent->spawnflags & DROPPED_ITEM)
						continue;
					else
						SetRespawn (ent, 1);
				}
			}
		}
		if (level.vote.bfg == 0)
		{
			gitem_t	*it;

			for (i=0 ; i<game.maxclients ; i++)
			{
				ent = &g_edicts[1+i];
				if (!ent->inuse)
					continue;

				it = FindItem("BFG10K");
				if (ent->client->pers.inventory[ITEM_INDEX(it)] > 0)
				{
					ent->client->pers.selected_item = ITEM_INDEX(FindItem("Blaster"));
					ent->client->pers.weapon = FindItem("Blaster");
					ent->client->newweapon = ent->client->pers.weapon;
					ChangeWeapon (ent);
					ent->client->pers.inventory[ITEM_INDEX(it)] = 0;
				}
			}
		}
	}

	if (level.vote.vote_what & VOTE_POWERUPS)
	{
		char	text[33];

		Com_sprintf(text, sizeof(text), "%d", level.vote.powerups);
		gi.cvar_set("allow_powerups", text);

		MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "POWERUPS changed to %d.\n", level.vote.powerups);

		for (i=1, ent=g_edicts+i; i < globals.num_edicts; i++,ent++)
		{
			if (!ent->item)
				continue;

			if (!strcmp(ent->item->pickup_name, "Power Shield") || !strcmp(ent->item->pickup_name, "Quad Damage")  || !strcmp(ent->item->pickup_name, "Invulnerability"))
			{
				if (level.vote.powerups == 0)
				{
					if (ent->spawnflags & DROPPED_ITEM)
						G_FreeEdict(ent);
					else
						SetRespawn (ent, 1);
				}
				else
				{
					if (ent->spawnflags & DROPPED_ITEM)
						continue;
					else
						SetRespawn (ent, 1);
				}
			}
		}
		if (level.vote.powerups == 0)
		{
			for (i=0 ; i<game.maxclients ; i++)
			{
				ent = &g_edicts[1+i];
				if (!ent->inuse)
					continue;

				if (ent->client->invincible_framenum > level.framenum)
					ent->client->invincible_framenum = 0;

				if (ent->client->quad_framenum > level.framenum)
					ent->client->quad_framenum = 0;

				it = FindItem("Power Shield");
				if (ent->client->pers.inventory[ITEM_INDEX(it)] > 0)
				{
					ent->client->pers.inventory[ITEM_INDEX(it)] = 0;
					ValidateSelectedItem (ent);
				}
				it = FindItem("Quad Damage");
				if (ent->client->pers.inventory[ITEM_INDEX(it)] > 0)
				{
					ent->client->pers.inventory[ITEM_INDEX(it)] = 0;
					ValidateSelectedItem (ent);
				}
				it = FindItem("Invulnerability");
				if (ent->client->pers.inventory[ITEM_INDEX(it)] > 0)
				{
					ent->client->pers.inventory[ITEM_INDEX(it)] = 0;
					ValidateSelectedItem (ent);
				}
			}
		}
	}

	if (level.vote.vote_what & VOTE_HUD)
	{
		char text[33];

		MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "HUD changed to %d.\n", level.vote.hud);

		Com_sprintf(text, sizeof(text), "%d", level.vote.hud);

		gi.cvar_set("allow_hud", text);

		for (i=0 ; i<game.maxclients ; i++)
		{
			ent = &g_edicts[1+i];
			if (!ent->inuse)
				continue;

			ent->client->pers.save_data.hudlist = 0;
		}
	}

	if (level.vote.vote_what & VOTE_HAND3)
	{
		char text[33];
		MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "HAND3 changed to %d.\n", level.vote.hand3);
		Com_sprintf(text, sizeof(text), "%d", level.vote.hand3);

		gi.cvar_set("allow_hand3", text);
	}

	if (level.vote.vote_what & VOTE_CONFIG)
	{
		char text[80];

		MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "Execing %s.\n", level.vote.config);
		MyCPrintfEnd(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE);
		sprintf(text, "exec %s\n", level.vote.config);
		gi.AddCommandString(text);
	}
	else if (level.vote.vote_what & VOTE_MAP)
	{
		MyCPrintf(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE, "Changing map to %s.\n", level.vote.map);
		MyCPrintfEnd(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE);
		strcpy(level.nextmap, level.vote.map);
		EndDMLevel(true, false);
	}
	else
		MyCPrintfEnd(level.vote.voter, BPRINTF, ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_ORANGE);

	memset(&level.vote, 0, sizeof(vote_struct));
	level.vote.vote_active = false;

	for (i=0 ; i<game.maxclients ; i++)
	{
		ent = &g_edicts[1+i];
		if (!ent->inuse)
			continue;

		ent->client->pers.save_data.vote_yon = VOTE_NONE;
		ent->client->pers.save_data.vote_change_count = 0;
	}
}

/*
** 2.B - CheckProposes (voting)
*/

void CheckProposes(void)
{
	if (!level.vote.vote_active)
		return;

	if (level.vote.vote_time <= level.time)
	{
		if ( level.vote.vote_yes > 0 && level.vote.vote_yes > level.vote.vote_no )
			VotePassed();
		else
			VoteFailed();
	}
	else
	{
		int i;
		int num_clients=0;
		edict_t	*ent;
		gitem_t	*it;

		if (level.vote.update_vote == true)
		{
			char text[1400];
			text[0]='\0';

			if (level.vote.multivote)
				Com_sprintf (text, sizeof(text), "Multivote initiated, type vote for details. YES [%d] - NO [%d]", level.vote.vote_yes, level.vote.vote_no);
			else
			{
				if (level.vote.vote_what & VOTE_KICK)
				{
					if (g_edicts[level.vote.kick+1].inuse)
						Com_sprintf (text, sizeof(text), "Kick %s (client %d)? YES [%d] - NO [%d]", g_edicts[1+level.vote.kick].client->pers.netname, level.vote.kick, level.vote.vote_yes, level.vote.vote_no);
				}
				else if (level.vote.vote_what & VOTE_TIMELIMIT)
					Com_sprintf (text, sizeof(text), "Change timelimit to %d? YES [%d] - NO [%d]", level.vote.timelimit, level.vote.vote_yes, level.vote.vote_no);
				else if (level.vote.vote_what & VOTE_CONFIG)
					Com_sprintf (text, sizeof(text), "Execute config: %s? YES [%d] - NO [%d]", level.vote.config, level.vote.vote_yes, level.vote.vote_no);
				else if (level.vote.vote_what & VOTE_DMFLAGS)
				{
					qboolean	isvalidflag=false;

					for (i=1; i<NUM_DMFLAGS; i++)
					{
						if (dmflags_table[i].flags == level.vote.dmflags)
						{
							isvalidflag = true;
							break;
						}
					}
					if (isvalidflag)
					{
						if ((int)dmflags->value & level.vote.dmflags)
							Com_sprintf (text, sizeof(text), "DISABLE dmflag %s? YES [%d] - NO [%d]", dmflags_table[i].name, level.vote.vote_yes, level.vote.vote_no);
						else
							Com_sprintf (text, sizeof(text), "ENABLE dmflag %s? YES [%d] - NO [%d]", dmflags_table[i].name, level.vote.vote_yes, level.vote.vote_no);
					}
					else
						Com_sprintf (text, sizeof(text), "ENABLE dmflag %d? YES [%d] - NO [%d]", level.vote.dmflags, level.vote.vote_yes, level.vote.vote_no);
				}
				else if (level.vote.vote_what & VOTE_TP)
					Com_sprintf (text, sizeof(text), "Change tp to %d? YES [%d] - NO [%d]", level.vote.tp, level.vote.vote_yes, level.vote.vote_no);
				else if (level.vote.vote_what & VOTE_POWERUPS)
					Com_sprintf (text, sizeof(text), "Change powerups to %d? YES [%d] - NO [%d]", level.vote.powerups, level.vote.vote_yes, level.vote.vote_no);
				else if (level.vote.vote_what & VOTE_BFG)
					Com_sprintf (text, sizeof(text), "Change bfg to %d? YES [%d] - NO [%d]", level.vote.bfg, level.vote.vote_yes, level.vote.vote_no);
				else if (level.vote.vote_what & VOTE_HUD)
					Com_sprintf (text, sizeof(text), "Change hud to %d? YES [%d] - NO [%d]", level.vote.hud, level.vote.vote_yes, level.vote.vote_no);
				else if (level.vote.vote_what & VOTE_MAP)
					Com_sprintf (text, sizeof(text), "Change map to %s? YES [%d] - NO [%d]", level.vote.map, level.vote.vote_yes, level.vote.vote_no);
				else if (level.vote.vote_what & VOTE_FASTWEAPONS)
					Com_sprintf (text, sizeof(text), "Change fastweapons to %d? YES [%d] - NO [%d]", level.vote.fastweapons, level.vote.vote_yes, level.vote.vote_no);
				else if (level.vote.vote_what & VOTE_HAND3)
					Com_sprintf (text, sizeof(text), "Change hand3 to %d? YES [%d] - NO [%d]",       level.vote.hand3, level.vote.vote_yes, level.vote.vote_no);
			}

			gi.configstring (CS_AIRACCEL-1, text);
			level.vote.update_vote = false;
		}

		for (i=0 ; i<game.maxclients ; i++)
		{
			if (!g_edicts[1+i].inuse || g_edicts[1+i].client->pers.mvdspec == true )
				continue;

			if (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH)
			{
				if (g_edicts[1+i].client->pers.save_data.team != TEAM_NONE)
					num_clients++;
			}
			else
				num_clients++;
		}

		if ( num_clients == level.vote.vote_no+level.vote.vote_yes && level.vote.vote_no == level.vote.vote_yes ) //tie
		{
			VoteFailed();
			return;
		}

		num_clients /=2;
		num_clients += 1;

		if (num_clients <= level.vote.vote_yes)
			VotePassed();
		else if (num_clients <= level.vote.vote_no)
			VoteFailed();
	}
}

/*
** 2.C - CheckMatchState
*/

void CheckMatchState(void)
{
	int		i;
	int ready=0;
	int	notready=0;
	int	total=0;
	int	mins, secs;
	edict_t	*cl;
	char	text[32];
	edict_t *ent;
	int	team_a=0;
	int	team_b=0;
	struct admin_s *adminCheck;

	if (level.match_state == END)
		return;

	for (i=0 ; i<game.maxclients ; i++)
	{
		cl = &g_edicts[1+i];
		if (!cl->inuse)
			continue;

		if (!cl->client->pers.connected)
			return;

		if (cl->client->pers.save_data.team != TEAM_NONE)
		{
			total++;
			if (cl->client->pers.save_data.ready_or_not == true)
				ready++;
			else
				notready++;

			if (cl->client->pers.save_data.team == TEAM_A)
				team_a++;
			else if (cl->client->pers.save_data.team == TEAM_B)
				team_b++;
		}
	}
	level.notready = notready;
	if (level.framenum > 100 && !team_a && game.teamA_locked == true && level.match_state != MATCH && level.match_state != OVERTIME && level.match_state != SUDDENDEATH)
	{
		gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "No player in team \"%s\", unlocking.\n", game.teamA_name);
		game.teamA_locked = false;
	}
	if (level.framenum > 100 && !team_b && game.teamB_locked == true && level.match_state != MATCH && level.match_state != OVERTIME && level.match_state != SUDDENDEATH)
	{
		gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "No player in team \"%s\", unlocking.\n", game.teamB_name);
		game.teamB_locked = false;
	}
	if (!team_a || !team_b)
	{
		if (level.match_state != WARMUP)
		{
			int oldMachState = level.match_state;

			level.match_state = WARMUP;
			level.teamA_score = 0;
			level.teamB_score = 0;
			level.teamA_numpauses = 0;
			level.teamB_numpauses = 0;
			level.paused = false;
			level.pauser = NULL;
			ready=0;

			ClearDominationRunes();
			FreeOldScores();
			FreeSavedPlayers();

			for (i=0 ; i<game.maxclients ; i++)
			{
				cl = &g_edicts[1+i];
				if (!cl->inuse)
					continue;
			
				notready++;
				cl->client->pers.save_data.ready_or_not = false;
				cl->client->resp.score = 0;
				cl->client->resp.net = 0;

//				Reset_Accuracy(cl);

				for (adminCheck = game.admin_first; adminCheck; adminCheck = adminCheck->next)
				{
					if (strlen(adminCheck->password))
						adminCheck->used = false;
				}

				if (oldMachState == MATCH || oldMachState == OVERTIME || oldMachState == SUDDENDEATH)
				{
					char stuffCmd[128];
					Com_sprintf(stuffCmd, sizeof(stuffCmd), "accuracy \"%s\"%s\n", cl->client->pers.netname, cl->client->pers.save_data.autorecord ? "; stop" : "");
					gi.WriteByte (svc_stufftext);
					gi.WriteString (stuffCmd);
					gi.unicast(cl, true);
				}

				if (cl->client->pers.save_data.team != TEAM_NONE )
				{
					char		userinfo[MAX_INFO_STRING];

					if (cl->health > 0)
					{
						memcpy (userinfo, cl->client->pers.userinfo, sizeof(userinfo));
						InitClientPersistant(cl->client);
						cl->client->newweapon = FindItem ("Railgun");
						ChangeWeapon (cl);
						ClientUserinfoChanged (cl, userinfo);
					}
				}

			}
			level.update_score = true;
			if (game.teamA_locked || game.teamB_locked)
			{
				if (!team_a && !team_b)
				{
					gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Teams unlocked!\n");
					game.teamB_locked = false;
					game.teamA_locked = false;
				}
				else if (!team_a)
				{
					gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Team %s unlocked!\n", game.teamA_name);
					game.teamA_locked = false;
				}
				else if (!team_b)
				{
					gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Team %s unlocked!\n", game.teamB_name);
					game.teamB_locked = false;
				}
			}
			for (i=game.maxclients, ent=g_edicts+i; i < globals.num_edicts; i++,ent++)
			{
				if (!ent->item)
					continue;

				if (ent->spawnflags & DROPPED_ITEM || ent->spawnflags & DROPPED_PLAYER_ITEM)
					G_FreeEdict(ent);

				SetRespawn(ent,1);
			}
		}
	}

	if (level.match_state == WARMUP)
	{
		if (total > 1 && ready >= notready)
		{
			gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_MAROON, "PREGAME has started, type \"ready\" in console to start COUNTDOWN.\n");
			level.match_state = PREGAME;
			sprintf(text, "  PREGAME");
			if (strcmp(level.status, text))
			{
				strcpy(level.status, text);
				for (i=0; i<strlen(text); i++)
					text[i] |= 128;
				gi.configstring(CS_AIRACCEL-2, text);
			}
			for (i=0 ; i<game.maxclients ; i++)
			{
				cl = &g_edicts[1+i];
				if (!cl->inuse)
					continue;

				if (!cl->client->pers.connected)
					continue;

				gi.WriteByte (svc_stufftext);
				gi.WriteString ("play misc/talk1.wav\n");
				gi.unicast(cl, true);

				if (cl->client->pers.save_data.team != TEAM_NONE)
				{
					char		userinfo[MAX_INFO_STRING];

					if (cl->health > 0)
					{
						cl->client->newweapon = FindItem ("blaster");
						ChangeWeapon (cl);
						memcpy (userinfo, cl->client->pers.userinfo, sizeof(userinfo));
						InitClientPersistant(cl->client);
						ClientUserinfoChanged (cl, userinfo);
					}
				}
			}
		}
		else
		{
			sprintf(text, " WARMUP");

			if (strcmp(level.status, text))
			{
				strcpy(level.status, text);
				for (i=0; i<strlen(text); i++)
					text[i] |= 128;
				gi.configstring(CS_AIRACCEL-2, text);
			}
		}
	}
	else if (level.match_state == PREGAME)
	{
		if (total > 1 && ready == total)
		{
			char demoName[128];
			time_t currTime;
			struct tm *timeStruct;
			cvar_t *hostname = gi.cvar("hostname", "", 0);

			gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_MAROON, "COUNTDOWN has started!\n");
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "Teams locked!\n");
			game.teamB_locked = true;
			game.teamA_locked = true;

			for (i=0; i<level.numSpawnPoints; i++)
				level.spawnPoints[i].free = true;

			level.match_state = COUNTDOWN;
			level.countdown_framenum = level.framenum + 200;
			sprintf(text, "COUNTDOWN: %02d", (int)(level.countdown_framenum - level.framenum)/10);
			if (strcmp(level.status, text))
			{
				strcpy(level.status, text);
				for (i=0; i<strlen(text); i++)
					text[i] |= 128;
				gi.configstring(CS_AIRACCEL-2, text);
			}

			currTime = game.servertime;
			timeStruct = localtime(&currTime);
			Com_sprintf(demoName, sizeof(demoName), "play misc/tele_up.wav; record %s_%d-%02d-%02d_%02d.%02d.%02d\n", level.mapname, 
				1900+timeStruct->tm_year, timeStruct->tm_mon+1, timeStruct->tm_mday,
				timeStruct->tm_hour, timeStruct->tm_min, timeStruct->tm_sec);
			for (i=0 ; i<game.maxclients ; i++)
			{
				cl = &g_edicts[1+i];
				if (!cl->inuse)
					continue;

				if (!cl->client->pers.connected)
					continue;

				cl->client->showsets = false;
				Cmd_Sets_f(cl);

				if (cl->client->pers.save_data.team != TEAM_NONE && cl->health <=0 )
					respawn(cl, false, true);

				gi.WriteByte (svc_stufftext);
				if (cl->client->pers.save_data.autorecord)
					gi.WriteString (demoName);
				else
					gi.WriteString ("play misc/tele_up.wav\n");
				gi.unicast(cl, true);
			}
			for (i=game.maxclients, ent=g_edicts+i; i < globals.num_edicts; i++,ent++)
			{

				if (ent->s.effects & EF_GIB)
					G_FreeEdict(ent);
				if (ent->classname)
				{
					if (!strcmp(ent->classname, "bodyque"))
					{
						ent->svflags |= SVF_NOCLIENT;
						ent->takedamage = DAMAGE_NO;
						ent->solid = SOLID_NOT;
						ent->movetype = MOVETYPE_NONE;
					}
				}

				if (!ent->item)
					continue;

				if (ent->spawnflags & DROPPED_ITEM || ent->spawnflags & DROPPED_PLAYER_ITEM)
					G_FreeEdict(ent);

				SetRespawn(ent,9);
			}
		}
	}
	else if (level.match_state == COUNTDOWN)
	{
		if (total > 1 && ready < total)
		{
			gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_MAROON, "COUNTDOWN aborted!\n");

			level.match_state = PREGAME;
			level.countdown_framenum = 0;
			sprintf(text, "  PREGAME");
			if (strcmp(level.status, text))
			{
				strcpy(level.status, text);
				for (i=0; i<strlen(text); i++)
					text[i] |= 128;
				gi.configstring(CS_AIRACCEL-2, text);
			}

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
			}
		}
		else if (level.countdown_framenum == level.framenum+10)
		{
			gi.bprintf(ATTR_PRINT_CHAT|ATTR_BOLD|ATTR_C_MAROON, "MATCH has started!\n");
			level.match_state = MATCH;
			level.countdown_framenum = level.framenum + (int)timelimit->value*600;
			level.start_framenum = level.framenum;
			level.update_score = true;

			FreeOldScores();

			memset(&level.vote, 0, sizeof(vote_struct));

			for (i=0 ; i<game.maxclients ; i++)
			{
				cl = &g_edicts[1+i];
				if (!cl->inuse)
					continue;

				cl->client->pers.save_data.vote_yon = VOTE_NONE;
				cl->client->pers.save_data.vote_change_count = 0;

				if (!cl->client->pers.connected)
					continue;

				if (cl->client->pers.save_data.team != TEAM_NONE)
				{
					cl->client->resp.old_avgping = cl->client->resp.avgping = 0;
					respawn(cl, false, false);
				}
				else if (!cl->client->chase_target && cl->client->pers.save_data.team == TEAM_NONE && !cl->client->pers.mvdspec)
					GetChaseTarget(cl);

				if (cl->client->pers.save_data.is_admin)
				{
					for (adminCheck = game.admin_first; adminCheck; adminCheck = adminCheck->next)
					{
						if (strlen(adminCheck->password))
						{
							if (!strcmp(cl->client->pers.save_data.admin_password, adminCheck->password))
							{
								adminCheck->used = true;
								break;
							}
						}
					}
				}
				Reset_Accuracy(cl);
			}
			for (i=game.maxclients, ent=g_edicts+i; i < globals.num_edicts; i++,ent++)
			{
				if (ent->spawnflags & DROPPED_ITEM || ent->spawnflags & DROPPED_PLAYER_ITEM)
					G_FreeEdict(ent);
				if (ent->s.effects & EF_GIB)
					G_FreeEdict(ent);
				if (ent->classname)
				{
					if (!strcmp(ent->classname, "bodyque"))
					{
						ent->svflags |= SVF_NOCLIENT;
						ent->takedamage = DAMAGE_NO;
						ent->solid = SOLID_NOT;
						ent->movetype = MOVETYPE_NONE;
					}
				}
			}

		}
		else
		{
			int		time;

			time = (int)(level.countdown_framenum - level.framenum);
			sprintf(text, "COUNTDOWN: %02d", (int)(level.countdown_framenum - level.framenum)/10);
			if (strcmp(level.status, text))
			{
				strcpy(level.status, text);
				for (i=0; i<strlen(text); i++)
					text[i] |= 128;
				gi.configstring(CS_AIRACCEL-2, text);
			}
			if (time == 110)
			{
				for (i=0 ; i<game.maxclients ; i++)
				{
					cl = &g_edicts[1+i];
					if (!cl->inuse)
						continue;
			
					if (!cl->client->pers.connected)
						continue;

					cl->client->showsets = true;
					Cmd_Sets_f(cl);
					cl->client->showscores = true;
					DeathmatchScoreboardMessage (cl, cl->enemy);
					gi.unicast (cl, false);

					gi.WriteByte (svc_stufftext);
					gi.WriteString ("play world/10_0.wav\n");
					gi.unicast(cl, true);
				}
			}
		}
	}

	if (level.match_state == MATCH || level.match_state == OVERTIME)
	{
		mins = (level.countdown_framenum - level.framenum)/600;
		secs = ((level.countdown_framenum - level.framenum)-(mins*600))/10;
		if (level.countdown_framenum - level.framenum == 110)
		{
			for (i=0 ; i<game.maxclients ; i++)
			{
				cl = &g_edicts[1+i];
				if (!cl->inuse)
					continue;

				if (!cl->client->pers.connected)	
					continue;

				gi.WriteByte (svc_stufftext);
				gi.WriteString ("play world/10_0.wav\n");
				gi.unicast(cl, true);
			}
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "10 seconds left!\n");
		}
		else if (level.countdown_framenum - level.framenum == 310)
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "30 seconds left.\n");
		else if (level.countdown_framenum - level.framenum == 610)
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "1 minute left.\n");
		else if (level.countdown_framenum - level.framenum == 3010)
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "5 minutes left.\n");
		else if (level.countdown_framenum - level.framenum == 6010)
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "10 minutes left.\n");
		else if (level.countdown_framenum - level.framenum == 9010)
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD|ATTR_C_MAROON, "15 minutes left.\n");

		sprintf(text, "  %2d:%02d", mins, secs);
		if (strcmp(level.status, text))
		{
			strcpy(level.status, text);
			for (i=0; i<strlen(text); i++)
				text[i] |= 128;
			gi.configstring(CS_AIRACCEL-2, text);
		}
	}
}

/*
** 2.D - CheckForUpdate
*/

#if defined __linux__ && defined UDSYSTEM

void RemoveTempFile( char *filename )
{
	if ( unlink( filename ) < 0 )
		gi.dprintf( "Unable to remove temporary file. (%s:%d)\n", strerror( errno ), errno );
}

typedef struct update_info
{
	char file_url[1024];
	char file_sum[41];
} update_into_t;

size_t WriteFileCallback( void *ptr, size_t size, size_t nmemb, void *stream )
{
	FILE *f = (FILE*)stream;
	if ( !ptr || !f || !nmemb )
		return 0;

	fwrite( ptr, size, nmemb, f );
	return size*nmemb;
}

size_t UpdateCheckResponse( void *ptr, size_t size, size_t nmemb, void *stream )
{
	update_into_t *ud_info = (update_into_t *)stream;
	char *start_pos, *end_pos;

	if ( !nmemb || !ptr || size*nmemb < 2 || !ud_info )
		return 0;

	ud_info->file_url[0] = 0;

	start_pos = memchr( ptr, '|', size*nmemb );
	if ( start_pos == NULL )
		return size*nmemb;

	start_pos++;
	
	end_pos = memchr( start_pos, '|', size*nmemb-(start_pos-(char*)ptr) );
	if ( end_pos == NULL || end_pos+1 >= (char*)ptr+(nmemb*size) || end_pos-start_pos >= 1024 )
		return size*nmemb;

	strncpy( ud_info->file_url, start_pos, end_pos-start_pos );
	if ( !strcmp( ud_info->file_url, "no update" ) )
		return size*nmemb;

	start_pos = end_pos+1;
	end_pos = memchr( start_pos, '|', size*nmemb-(start_pos-(char*)ptr) );
	if ( end_pos == NULL || end_pos-start_pos > 40 )
	{
		ud_info->file_url[0] = 0;
		return size*nmemb;
	}

	strncpy( ud_info->file_sum, start_pos, end_pos-start_pos );

	return size*nmemb;
}

size_t UpdateTestHeaders( void *ptr, size_t size, size_t nmemb, void *stream )
{
	int *test_ok = (int*)stream;
	int len = size*nmemb;
	char *tmp_ptr;
	if ( !nmemb || !test_ok || !ptr )
		return 0;

	if ( !memcmp( ptr, "Content-Type:", ( len > 13 ? 13 : len ) ) )
	{
		tmp_ptr = memchr( ptr, ' ', len );
		if ( tmp_ptr && len-(tmp_ptr-(char*)ptr) > 1 )
			tmp_ptr++;
		if ( !memcmp( tmp_ptr, "application/octet-stream", ( len-(tmp_ptr-(char*)ptr) > 24 ? 24 : len-(tmp_ptr-(char*)ptr) ) ) )
			*test_ok = 1;
	}

	return size*nmemb;
}

char *FileSHA1Sum( char *filename )
{
	int i, len = 0, j, len_sum = 0;
	char buff[1024], sum[SHS_DIGESTSIZE];
	SHA_CTX ctx;
	static const char *hex = "0123456789abcdef";
	static char out[(SHS_DIGESTSIZE*2)+1];
	FILE *f = NULL;

	if ( !filename )
		return NULL;

	f = fopen( filename, "rb" );

	if ( !f )
		return NULL;

	memset( buff, 0, sizeof( buff ) );

	SHAInit( &ctx );

	while( ( len = fread( buff, 1, sizeof( buff ), f ) ) > 0 )
	{
		len_sum += len;
		SHAUpdate( &ctx, buff, len );
	}

	if ( ferror( f ) )
		return NULL;

	fclose( f );

	SHAFinal( sum, &ctx );

	for( i=0,j=0; j<SHS_DIGESTSIZE;)
	{
		out[i++] = hex[(sum[j]>>4)&0xf];
		out[i++] = hex[sum[j++]&0xf];
	}
	out[i] = 0;

	return out;
}

void DownloadCurrentVersion( char *outfile )
{
	long code = 0;
	int error_code;
	update_into_t ud_info;
	CURL *cu;
	char url[1024];
	char *shasum = FileSHA1Sum( outfile );

	if ( !shasum )
	{
		gi.dprintf( "Update failed. Reason: error while calculating %s checksum\n", outfile );
		return;
	}

	memset( &ud_info, 0, sizeof( update_into_t ) );

	if ( ( cu = curl_easy_init() ) == NULL )
	{
		gi.dprintf( "Update failed. Reason: curl initialization failed.\n" );
		return;
	}

	Com_sprintf( url, sizeof( url ), "%s?sum=%s", ud_address->string, shasum );

	curl_easy_setopt( cu, CURLOPT_URL, url ); 
	curl_easy_setopt( cu, CURLOPT_USERAGENT, TDMVERSION );
	curl_easy_setopt( cu, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1 );
	curl_easy_setopt( cu, CURLOPT_WRITEFUNCTION, UpdateCheckResponse );
	curl_easy_setopt( cu, CURLOPT_WRITEDATA, &ud_info );

	gi.dprintf( "Sending request to %s...\n", url );
	if ( ( error_code = curl_easy_perform( cu ) ) != 0 )
	{
		gi.dprintf( "Update failed. Reason: %s.\n", curl_easy_strerror( error_code ) );
		curl_easy_cleanup( cu );
		return;
	}

	if ( ( error_code = curl_easy_getinfo( cu, CURLINFO_RESPONSE_CODE, &code ) ) != CURLE_OK )
	{
		gi.dprintf( "Update failed. Reason: %s.\n", curl_easy_strerror( error_code ) );
		curl_easy_cleanup( cu );
		return;
	}

	if ( code == 200 && strlen( ud_info.file_url ) && strlen( ud_info.file_sum ) )
	{
		char *template = NULL;
		FILE *f = NULL;

		gi.dprintf( "Recived download location %s and checksum %s\n", ud_info.file_url, ud_info.file_sum );

		do
		{
			int len = strlen( outfile ) + strlen( ".XXXXXX" );
			int d, test_ok = 0;
			shasum = NULL;

			if ( ( template = (char*)malloc( len+1 ) ) == NULL )
			{
				gi.dprintf( "Update failed. Reason: memory allocation error at %s:%d (%s).\n", __FILE__, __LINE__, strerror( errno ) );
				break;
			}
			Com_sprintf( template, len+1, "%s.XXXXXX", outfile );
			template[len] = 0;
			if ( ( d = mkstemp( template ) ) == -1 )
			{
				gi.dprintf( "Update failed. Reason: failed to create temporary file: %s\n", strerror( errno ) );
				break;
			}

			if ( ( f = fdopen( d, "wb" ) ) == NULL )
			{
				gi.dprintf( "Update failed. Reason: failed to open temporary file descriptor: %s\n", strerror( errno ) );
				break;
			}

			gi.dprintf( "Downloading file to %s...\n", template );

			curl_easy_reset( cu );
			curl_easy_setopt( cu, CURLOPT_USERAGENT, TDMVERSION );
			curl_easy_setopt( cu, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1 );
			curl_easy_setopt( cu, CURLOPT_URL, ud_info.file_url );
			curl_easy_setopt( cu, CURLOPT_WRITEFUNCTION, WriteFileCallback );
			curl_easy_setopt( cu, CURLOPT_WRITEDATA, f );
			curl_easy_setopt( cu, CURLOPT_HEADERFUNCTION, UpdateTestHeaders );
			curl_easy_setopt( cu, CURLOPT_HEADERDATA, &test_ok );

			if ( ( error_code = curl_easy_perform( cu ) ) != 0 )
			{
				gi.dprintf( "Update failed. Reason: %s.\n", curl_easy_strerror( error_code ) );
				break;
			}

			if ( ( error_code = curl_easy_getinfo( cu, CURLINFO_RESPONSE_CODE, &code ) ) != CURLE_OK )
			{
				gi.dprintf( "Update failed. Reason: %s.\n", curl_easy_strerror( error_code ) );
				break;
			}

			if ( code != 200 )
			{
				gi.dprintf( "Update failed. Reason: server responded with code %d.\n", code );
				break;
			}

			gi.dprintf( "Checking file checksum...\n" );

			fclose( f );
			f = NULL;
			shasum = FileSHA1Sum( template );
			if ( test_ok != 1 || !shasum || strcmp( shasum, ud_info.file_sum ) )
			{
				if ( test_ok )
					gi.dprintf( "Update failed. Reason: file checksum not match.\n" );
				else
					gi.dprintf( "Update failed. Reason: bad response Content-Type.\n" );
				break;
			}

			gi.dprintf( "Renaming downloaded file to %s\n", outfile );
			
			if ( chmod( template, S_IRWXU|S_IRGRP|S_IROTH ) < 0 )
				gi.dprintf( "Unable to change permission for the file.(%s)\nUpdate may be incomplete.\n", strerror( errno ) );
			if ( rename( template, outfile ) < 0 )
				gi.dprintf( "Unable to rename the file.(%s)\nUpdate may be incomplete.\n", strerror( errno ) );

			free( template );
			template = NULL;

			gi.dprintf( "Done.\n" );

			if ( ud_restart->value )
			{
				char data[256];
				gi.dprintf( "Please wait while restarting...\n" );
				Com_sprintf( data, sizeof( data ), "map %s", level.mapname );
				gi.AddCommandString( data );
			}
		} while( 0 );
		if ( f )
			fclose( f );
		if ( template )
		{
			RemoveTempFile( template );
			free( template );
		}
	}
	else
	{
		if ( code != 200 )
			gi.dprintf( "Update failed. Reason: server responded with code %d.\n", code );
		if ( !strcmp( ud_info.file_url, "no update" ) )
			gi.dprintf( "Update failed. Reason: unexpected server response (probably wrong ud_address provided).\n", code );
		else
			gi.dprintf( "No update available\n" );
	}

	curl_easy_cleanup( cu );
}

void CheckForUpdate( void )
{
	cvar_t	*basedir;
	cvar_t	*gamedir;
	char	filename[256];
	time_t rawtime;
	struct tm *timeinfo;

	if ( !strlen( ud_filename->string ) || !strlen( ud_address->string ) )
		return;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	if ( timeinfo->tm_hour == ud_time->value && timeinfo->tm_min == 0 && timeinfo->tm_sec == 0 ) //time to check for update
	{	
		gi.dprintf( "Checking for update...\n" );
		level.update_break = level.time + GAMESECONDS(20); //wait 2 sec before entering the function again

		basedir = gi.cvar( "basedir", "", 0 );
		gamedir = gi.cvar( "gamedir", "", 0 );
		
		strcpy( filename, basedir->string );
		if( strlen( gamedir->string ) )
		{
			strcat( filename, "/");
			strcat( filename, gamedir->string );
			strcat( filename, "/" );
			strcat( filename, ud_filename->string );
		}
		else
		{
			strcat( filename, "/baseq2/" );
			strcat( filename, ud_filename->string );
		}
		DownloadCurrentVersion( filename );
	}
}

#endif

/*
** 2.E - UpdateConfigStrings
*/

void UpdateConfigStrings(void)
{
	cvar_t *cvar_sets[8];/* = { timelimit, dmflags, allow_bfg, allow_powerups, allow_hud, allow_hand3, fastweapons, (cvar_t*)&game.tp };*/
	static char *sets_names[8] = { "TIMELIMIT", "DMFLAGS", "BFG", "POWERUPS", "HUD", "HAND 3", "FASTWEAPONS", "TP" };
	char refresh = ( maxclients->value > 19 ? (char)maxclients->value+1 : 20 );
	edict_t		*cl_ent;
	char	text[32];
	int		roznica, count=0;

	if (domination->value)
		return;

	cvar_sets[0] = timelimit;
	cvar_sets[1] = dmflags;
	cvar_sets[2] = allow_bfg;
	cvar_sets[3] = allow_powerups;
	cvar_sets[4] = allow_hud;
	cvar_sets[5] = allow_hand3;
	cvar_sets[6] = fastweapons;
	cvar_sets[7] = (cvar_t*)&game.tp;

	if ( level.match_state < MATCH && !( level.framenum % refresh ) )
	{
		level.hud_setting_num++;
		if ( level.hud_setting_num >= sizeof( sets_names )/sizeof( char*) )
			level.hud_setting_num = 0;

		level.update_score = true;
	}

	if (level.update_score == true)
	{
		level.update_score_client_num = 0;
		level.update_score = false;
	}

	if (!level.update_score)
	{
		if (level.update_score_client_num >= game.maxclients)
			return;

		while (level.update_score_client_num < game.maxclients)
		{
			cl_ent = g_edicts + 1 + level.update_score_client_num;
			count++;

			if (!cl_ent->inuse || !cl_ent->client->pers.connected)
			{
				level.update_score_client_num++;
				continue;
			}
			else if ( level.match_state < MATCH )
			{
				if ( (int*)cvar_sets[(int)level.hud_setting_num] == &game.tp )
					sprintf(text, "%s: %d", sets_names[(int)level.hud_setting_num], *((int*)cvar_sets[(int)level.hud_setting_num]) );
				else
					sprintf(text, "%s: %d", sets_names[(int)level.hud_setting_num], (int)cvar_sets[(int)level.hud_setting_num]->value );
				gi.WriteByte( svc_configstring );
				gi.WriteShort( CS_GENERAL + (cl_ent - g_edicts) );
				gi.WriteString( text );
				gi.unicast( cl_ent, true );
			}
			else
			{
				if (cl_ent->client->pers.save_data.team == TEAM_B)
				{
					roznica = level.teamB_score - level.teamA_score;
					sprintf(text, "%s%d (%d)%d:%d", roznica > 0 ? "+" : "", roznica, cl_ent->client->resp.score, level.teamB_score, level.teamA_score);
					gi.WriteByte( svc_configstring );
					gi.WriteShort( CS_GENERAL + (cl_ent - g_edicts) );
					gi.WriteString( text );
					gi.unicast( cl_ent, true );
				}
				else if (cl_ent->client->pers.save_data.team == TEAM_A)
				{
					roznica = level.teamA_score - level.teamB_score;
					sprintf(text, "%s%d (%d)%d:%d", roznica > 0 ? "+" : "", roznica, cl_ent->client->resp.score, level.teamA_score, level.teamB_score);
					gi.WriteByte( svc_configstring );
					gi.WriteShort( CS_GENERAL + (cl_ent - g_edicts) );
					gi.WriteString( text );
					gi.unicast( cl_ent, true );
				}
				else
				{
					SetChaseHudScore( cl_ent );
				}
			}
			level.update_score_client_num++;
			if (count >= 2)
				break;
		}
	}
}

/*
** 2.F - CheckCaptain
*/

void PrintToOtherClients(edict_t *except, int type, char *text, ...)
{
	int				i;
	va_list			argptr;
	static char		string[1024];

	va_start (argptr, text);
	vsprintf (string, text, argptr);
	va_end (argptr);

	for (i=1; i<=game.maxclients; i++)
	{
		if (!g_edicts[i].inuse)
			continue;

		if (&g_edicts[i] != except)
			gi.cprintf(&g_edicts[i], type, string);
	}
}

void CheckCaptain(edict_t *ent)
{
	int	i;
	qboolean	found=false;
	int			time_playing;

	if (!ent->client)
		return;

	if (ent->client->pers.save_data.team == TEAM_NONE || !ent->inuse)
	{
		if (game.teamA_captain == ent)
		{
			game.teamA_captain = NULL;
			time_playing = level.framenum;
			for (i=1; i<=game.maxclients; i++)
			{
				if (!g_edicts[i].inuse)
					continue;

				if (&g_edicts[i] == ent)
					continue;

				if (g_edicts[i].client->pers.save_data.team == TEAM_A)
				{
					if (g_edicts[i].client->resp.enterframe < time_playing)
					{
						time_playing = g_edicts[i].client->resp.enterframe;
						game.teamA_captain = &g_edicts[i];
						found = true;
					}
				}
			}
			if (found)
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has become captain of team %s.\n", game.teamA_captain->client->pers.netname, game.teamA_name);
		}
		else if (game.teamB_captain == ent)
		{
			game.teamB_captain = NULL;
			time_playing = level.framenum;
			for (i=1; i<=game.maxclients; i++)
			{
				if (!g_edicts[i].inuse)
					continue;

				if (&g_edicts[i] == ent)
					continue;

				if (g_edicts[i].client->pers.save_data.team == TEAM_B)
				{
					if (g_edicts[i].client->resp.enterframe < time_playing)
					{
						time_playing = g_edicts[i].client->resp.enterframe;
						game.teamB_captain = &g_edicts[i];
						found = true;
					}
				}
			}
			if (found)
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has become captain of team %s.\n", game.teamB_captain->client->pers.netname, game.teamB_name);
		}
	}
	else if (ent->client->pers.save_data.team == TEAM_A)
	{
		if (game.teamB_captain == ent)
		{
			time_playing = level.framenum;
			for (i=1; i<=game.maxclients; i++)
			{
				if (!g_edicts[i].inuse)
					continue;

				if (&g_edicts[i] == ent)
					continue;

				if (g_edicts[i].client->pers.save_data.team == TEAM_B)
				{
					if (g_edicts[i].client->resp.enterframe < time_playing)
					{
						time_playing = g_edicts[i].client->resp.enterframe;
						game.teamB_captain = &g_edicts[i];
						found = true;
					}
				}
			}
			if (!found)
				game.teamB_captain = NULL;
			else
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has become captain of team %s.\n", game.teamB_captain->client->pers.netname, game.teamB_name);
		}

		if (game.teamA_captain == NULL)
		{
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has become captain of team %s.\n", ent->client->pers.netname, game.teamA_name);
			game.teamA_captain = ent;
		}
	}
	else if (ent->client->pers.save_data.team == TEAM_B)
	{
		if (game.teamA_captain == ent)
		{
			time_playing = level.framenum;
			for (i=1; i<=game.maxclients; i++)
			{
				if (!g_edicts[i].inuse)
					continue;

				if (&g_edicts[i] == ent)
					continue;

				if (g_edicts[i].client->pers.save_data.team == TEAM_A)
				{
					if (g_edicts[i].client->resp.enterframe < time_playing)
					{
						time_playing = g_edicts[i].client->resp.enterframe;
						game.teamA_captain = &g_edicts[i];
						found = true;
					}
				}
			}
			if (!found)
				game.teamA_captain = NULL;
			else
				gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has become captain of team %s.\n", game.teamA_captain->client->pers.netname, game.teamA_name);
		}
		if (game.teamB_captain == NULL)			
		{
			gi.bprintf(ATTR_PRINT_HIGH|ATTR_BOLD, "%s has become captain of team %s.\n", ent->client->pers.netname, game.teamB_name);
			game.teamB_captain = ent;
		}
	}
}

/*
** 2.G - Spawn Ranges
*/

void FreeSpawnRanges (void)
{
	num_spawn_ranges = 0;
	if (spawn_ranges)
	{
		free(spawn_ranges);
		spawn_ranges = NULL;
	}
}

void InitSpawnRanges(void)
{
	FILE *f;
	char	filename[256];
	cvar_t	*basedir, *gamedir;
	char	*line;
	vec3_t	mins, maxs;

	FreeSpawnRanges();

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);

	strcpy(filename,basedir->string);
	if(strlen(gamedir->string))
	{
		strcat(filename,"/");
		strcat(filename,gamedir->string);
		strcat(filename,"/spawns/");
	}
	else
		strcat(filename,"/baseq2/spawns/");

	strcat(filename,level.mapname);
	strcat(filename,".ran");
	
	gi.dprintf("Loading random spawn ranges from %s...", filename);

	f = fopen(filename, "r");
	if (!f)
	{
		gi.dprintf("File not found!\n");
		return;
	}

	do
	{
		line = read_line(f);
		if (!line)
				break;
		mins[0] = (float)atoi(read_word(line, 0));
		mins[1] = (float)atoi(read_word(line, 1));
		mins[2] = (float)atoi(read_word(line, 2));
		maxs[0] = (float)atoi(read_word(line, 3));
		maxs[1] = (float)atoi(read_word(line, 4));
		maxs[2] = (float)atoi(read_word(line, 5));
		if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0 && maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0)
			break;

		num_spawn_ranges++;
		if (spawn_ranges == NULL)
			spawn_ranges = (spawn_range_t *)realloc(NULL, sizeof(spawn_range_t));
		else
			spawn_ranges = (spawn_range_t *)realloc(spawn_ranges, sizeof(spawn_range_t)*num_spawn_ranges);
		VectorCopy(mins, spawn_ranges[num_spawn_ranges-1].mins);
		VectorCopy(maxs, spawn_ranges[num_spawn_ranges-1].maxs);
	} while (!feof(f));

	fclose(f);

	gi.dprintf("Done.\n");
}

/*
** 3.A - Write_ConfigString
*/

void Write_ConfigString(void)
{
	edict_t *cl_ent;
	char	text[32];
	int		i, roznica;

	if (domination->value)
		return;

	for (i=0 ; i<game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;

		if (!cl_ent->client->pers.connected)
			continue;

		if (cl_ent->client->pers.save_data.team == TEAM_B)
		{
			roznica = level.teamB_score - level.teamA_score;
			sprintf(text, "%s%d (%d)%d:%d", roznica > 0 ? "+" : "", roznica, cl_ent->client->resp.score, level.teamB_score, level.teamA_score);
			gi.configstring(CS_GENERAL + (cl_ent - g_edicts), text);
		}
		else if (cl_ent->client->pers.save_data.team == TEAM_A)
		{
			roznica = level.teamA_score - level.teamB_score;
			sprintf(text, "%s%d (%d)%d:%d", roznica > 0 ? "+" : "", roznica, cl_ent->client->resp.score, level.teamA_score, level.teamB_score);
			gi.configstring(CS_GENERAL + (cl_ent - g_edicts), text);
		}
		else
		{
			sprintf(text, "%d:%d", level.teamA_score, level.teamB_score);
			gi.configstring(CS_GENERAL + (cl_ent - g_edicts), text);
		}
	}
}

void Write_ConfigString2(edict_t *ent)
{
	char	text[32];
	int		roznica;

	if ( domination->value || !ent->client->pers.connected || level.match_state < MATCH )
		return;

	if (ent->client->pers.save_data.team == TEAM_B)
	{
		roznica = level.teamB_score - level.teamA_score;
		sprintf(text, "%s%d (%d)%d:%d", roznica > 0 ? "+" : "", roznica, ent->client->resp.score, level.teamB_score, level.teamA_score);
	}
	else if (ent->client->pers.save_data.team == TEAM_A)
	{
		roznica = level.teamA_score - level.teamB_score;
		sprintf(text, "%s%d (%d)%d:%d", roznica > 0 ? "+" : "", roznica, ent->client->resp.score, level.teamA_score, level.teamB_score);
	}
	else
		sprintf(text, "%d:%d", level.teamA_score, level.teamB_score);
	gi.WriteByte( svc_configstring );
	gi.WriteShort( CS_GENERAL + ( ent - g_edicts ) );
	gi.WriteString( text );
	gi.unicast( ent, true );
}

/*
** 3.B - TDM_SelectSpawnPoint
*/

//iEno++
//dm1: 1152
//dm2: 943
//dm3: 400
//dm6: 255
//dm8: 400

qboolean findspawnpoint (edict_t *ent, vec3_t mins, vec3_t maxs, vec3_t origin, vec3_t angles)
{
	vec3_t loc = {0,0,0}, floor, start, end;
	int i, j=0, num_tries;
	trace_t tr;
	qboolean	found=false;
	float		fraction = 1, yaw;
	int			range=-1;
	vec3_t		mmins = {-16, -16, 0};
	vec3_t		mmaxs = {16, 16, 0};

	if ((int)sv_spawnrandom_numtries->value > 0)
		num_tries = (int)sv_spawnrandom_numtries->value;
	else
		num_tries = 1;

	for (j=0; j < num_tries; j++)
	{
		if ((int)sv_spawnrandom_numtries->value == 0)
			j = -1;

		if (num_spawn_ranges > 0)
		{
			if (range == -1)
				range = ranfr(0, num_spawn_ranges-1);

			loc[0] = ranfr(spawn_ranges[range].mins[0], spawn_ranges[range].maxs[0]);
			loc[1] = ranfr(spawn_ranges[range].mins[1], spawn_ranges[range].maxs[1]);
			loc[2] = ranfr(spawn_ranges[range].mins[2], spawn_ranges[range].maxs[2]);
		}
		else
		{
			for (i = 0; i < 3; i++)
			{
				if (i == 2)
				{
					if (!strcmp(level.mapname, "q2dm1"))
						loc[i] = ranfr(-4096, 1152);
					else if (!strcmp(level.mapname, "q2dm2"))
						loc[i] = ranfr(-4096, 943);
					else if (!strcmp(level.mapname, "q2dm3"))
						loc[i] = ranfr(-4096, 400);
					else if (!strcmp(level.mapname, "q2dm8"))
						loc[i] = ranfr(-4096, 400);
					else if (!strcmp(level.mapname, "q2dm6"))
						loc[i] = ranfr(-4096, 255);
					else
						loc[i] = ranfr(-4096, 4096);
				}
				else
					loc[i] = ranfr(-4096, 4096);
			}
		}

		if (gi.pointcontents(loc) != 0)
			continue;

		VectorCopy(loc, floor);
		floor[2] = -4096;
//		tr = gi.trace (loc, vec3_origin, vec3_origin, floor, ent, MASK_SHOT);
		tr = gi.trace (loc, mmins, mmaxs, floor, ent, MASK_SHOT);

		if (tr.startsolid || tr.allsolid || gi.pointcontents(tr.endpos) != 0)
			continue;
		else
		{
			VectorCopy (tr.endpos, loc);
			loc[2] += 1;
			if (ent->client->pers.save_data.team != TEAM_NONE)
			{
				start[0] = loc[0] + maxs[0]+4;
				start[1] = loc[1] + maxs[1]+4;
				start[2] = loc[2] + maxs[2]+1;
				VectorCopy (start, end);
				end[2] = loc[2]-1;
				if (gi.pointcontents(start) == 0 && gi.pointcontents(end) == 0)
				{
					tr = gi.trace (start, vec3_origin, vec3_origin, end, ent, MASK_SHOT);
					if (tr.fraction != 1)
						continue;
					if (tr.allsolid)
						continue;
					if (tr.startsolid)
						continue;
				}
				else
					continue;
				start[2] = loc[2] + maxs[2]+1;
				start[0] = loc[0] - maxs[0]-4;
				start[1] = loc[1] + maxs[1]+4;
				VectorCopy (start, end);
				end[2] = loc[2]-1;
				if (gi.pointcontents(start) == 0 && gi.pointcontents(end) == 0)
				{
					tr = gi.trace (start, vec3_origin, vec3_origin, end, ent, MASK_SHOT);
					if (tr.fraction != 1)
						continue;
					if (tr.allsolid)
						continue;
					if (tr.startsolid)
						continue;
				}
				else
					continue;
				start[2] = loc[2] + maxs[2]+1;
				start[0] = loc[0] + maxs[0]+4;
				start[1] = loc[1] - maxs[1]-4;
				VectorCopy (start, end);
				end[2] = loc[2]-1;
				if (gi.pointcontents(start) == 0 && gi.pointcontents(end) == 0)
				{
					tr = gi.trace (start, vec3_origin, vec3_origin, end, ent, MASK_SHOT);
					if (tr.fraction != 1)
						continue;
					if (tr.allsolid)
						continue;
					if (tr.startsolid)
						continue;
				}
				else
					continue;
				start[2] = loc[2] + maxs[2]+1;
				start[0] = loc[0] - maxs[0]-4;
				start[1] = loc[1] - maxs[1]-4;
				VectorCopy (start, end);
				end[2] = loc[2]-1;
				if (gi.pointcontents(start) == 0 && gi.pointcontents(end) == 0)
				{
					tr = gi.trace (start, vec3_origin, vec3_origin, end, ent, MASK_SHOT);
					if (tr.fraction != 1)
						continue;
					if (tr.allsolid)
						continue;
					if (tr.startsolid)
						continue;
				}
				else
					continue;
			}

			found = true;
			break;
		}
	}

	loc[2] += 24;
	VectorCopy(loc, origin);
	angles[PITCH] = 0;

	VectorCopy(origin, start);
	VectorCopy(origin, end);
	end[0] += 128;
	end[1] += 128;
	tr = gi.trace (start, vec3_origin, vec3_origin, end, ent, MASK_SHOT);
	if (tr.fraction != 1)
	{
		fraction = tr.fraction;
		yaw = (int) (atan2(tr.plane.normal[1], tr.plane.normal[0]) * 180 / M_PI);
		angles[YAW] = yaw;
	}

	VectorCopy(origin, start);
	VectorCopy(origin, end);
	end[0] -= 128;
	end[1] += 128;
	tr = gi.trace (start, vec3_origin, vec3_origin, end, ent, MASK_SHOT);
	if (tr.fraction != 1 && tr.fraction < fraction)
	{
		fraction = tr.fraction;
		yaw = (int) (atan2(tr.plane.normal[1], tr.plane.normal[0]) * 180 / M_PI);
		angles[YAW] = yaw;
	}

	VectorCopy(origin, start);
	VectorCopy(origin, end);
	end[0] -= 128;
	end[1] -= 128;
	tr = gi.trace (start, vec3_origin, vec3_origin, end, ent, MASK_SHOT);
	if (tr.fraction != 1 && tr.fraction < fraction)
	{
		fraction = tr.fraction;
		yaw = (int) (atan2(tr.plane.normal[1], tr.plane.normal[0]) * 180 / M_PI);
		angles[YAW] = yaw;
	}

	VectorCopy(origin, start);
	VectorCopy(origin, end);
	end[0] += 128;
	end[1] -= 128;
	tr = gi.trace (start, vec3_origin, vec3_origin, end, ent, MASK_SHOT);
	if (tr.fraction != 1 && tr.fraction < fraction)
	{
		fraction = tr.fraction;
		yaw = (int) (atan2(tr.plane.normal[1], tr.plane.normal[0]) * 180 / M_PI);
		angles[YAW] = yaw;
	}

	if (fraction == 1)
		angles[YAW] = ranfr(1, 360); // face a random direction
	angles[ROLL] = 0;
	return found;
}
//iEno--

void TDM_SelectSpawnPoint (edict_t *ent, vec3_t origin, vec3_t angles)
{
	int chosen, direction, i, firstChosen;
	qboolean found=false;

	if (level.spawnPoints)
	{
randagain:
		firstChosen = chosen = ranfr(1, level.numSpawnPoints-1);
		if (level.spawnPoints[chosen].free)
		{
			level.spawnPoints[chosen].free = false;
			VectorCopy(level.spawnPoints[chosen].ent->s.origin, origin);
			VectorCopy(level.spawnPoints[chosen].ent->s.angles, angles);
			origin[2] += 9;
		}
		else
		{
			direction = ranfr(0, 1);
			for (i=1; i<level.numSpawnPoints; i++)
			{
				if (direction)
					chosen++;
				else
					chosen--;

				if (chosen >= level.numSpawnPoints)
					chosen = 1;
				else if (chosen < 1)
					chosen = level.numSpawnPoints-1;

				if (level.spawnPoints[chosen].free)
				{
					level.spawnPoints[chosen].free = false;
					VectorCopy(level.spawnPoints[chosen].ent->s.origin, origin);
					VectorCopy(level.spawnPoints[chosen].ent->s.angles, angles);
					origin[2] += 9;
					found = true;
					break;
				}
				else
					continue;
			}
			//there is no free spot
			if (!found)
			{
				int k;
				vec3_t forward, right, end, start;
				trace_t tr;

				AngleVectors(level.spawnPoints[firstChosen].ent->s.angles, forward, right, NULL);

				for (k = 0; k < 4; k++)
				{
					if (k == 0)
						VectorMA(level.spawnPoints[firstChosen].ent->s.origin, 50, forward, end);
					else if (k == 1)
						VectorMA(level.spawnPoints[firstChosen].ent->s.origin, 50, right, end);
					else if (k == 2)
						VectorMA(level.spawnPoints[firstChosen].ent->s.origin, -50, right, end);
					else
						VectorMA(level.spawnPoints[firstChosen].ent->s.origin, -50, forward, end);
					VectorCopy(end, start);
					end[2] -= 15; //remember: 8 is the height of the spawn deck
					start[2] += 41;

					if ( gi.pointcontents(start) == 0 && gi.pointcontents(end)== 0 )
					{
						tr = gi.trace(end, NULL, NULL, start, level.spawnPoints[firstChosen].ent, CONTENTS_MONSTER);
						if (tr.fraction >= 1)
						{
							end[2] += 24;
							tr = gi.trace(level.spawnPoints[firstChosen].ent->s.origin, NULL, NULL, end, level.spawnPoints[firstChosen].ent, CONTENTS_MONSTER);
							if (tr.fraction >= 1 && gi.pointcontents(end) == 0)
							{
								VectorCopy(end, origin);
								VectorCopy(level.spawnPoints[firstChosen].ent->s.angles, angles);
								found = true;
								break;
							}
						}
					}
				} 
				if (!found)
					goto randagain;
//				gi.error ("Couldn't find spawn point %s\n", game.spawnpoint);
			}
		}
	}
	else
		gi.error ("Couldn't find spawn point %s\n", game.spawnpoint);
}

/*
** 3.C - DeathmatchScoreboardMessage
*/

int DeathmatchScoreboardMessageSortCompare( const void *a, const void *b )
{
	int score_diff = game.clients[*(int*)b].resp.score - game.clients[*(int*)a].resp.score;
	if ( score_diff != 0 )
		return score_diff;
	else
	{
		int net_diff = (game.clients[*(int*)b].resp.score-game.clients[*(int*)b].resp.net)-(game.clients[*(int*)a].resp.score-game.clients[*(int*)a].resp.net);
		if ( net_diff != 0 )
			return net_diff;
		else
			return ( *(int*)a - *(int*)b );
	}

	return 0;
}

void num_to_pics( char *buffer, int len, int xoffset, int num )
{
	int negative = num < 0 ? 1 : 0;
	if ( num < 0 )
	{
		if ( num >= -9 )
			xoffset += 8;
		Com_sprintf( buffer+strlen( buffer ), len-strlen( buffer ), "xv %d picn num_minus ", xoffset );
		xoffset += 16;
	}

	num = abs( num );

	if ( num/100 )
	{
		Com_sprintf( buffer+strlen( buffer ), len-strlen( buffer ), "xv %d picn num_%d ", xoffset, num/100 );
		xoffset += 16;
	}
	else if ( !negative )
		xoffset += 8;

	if ( ( num/100 && !((num%100)/10) ) || (num%100)/10 )
	{
		Com_sprintf( buffer+strlen( buffer ), len-strlen( buffer ), "xv %d picn num_%d ", xoffset, (num%100)/10 );
		xoffset += 16;
	}
	else if ( !negative )
		xoffset += 8;

	Com_sprintf( buffer+strlen( buffer ), len-strlen( buffer ),"xv %d picn num_%d ", xoffset, num%10 );
}

void DeathmatchScoreboardMessage (edict_t *ent, edict_t *killer)
{
	char entry[1024];
	char string[1400];
	int stringlength;
	int i, j, k;
	int px=-80, py=-40;
	int sorted_clients[3][MAX_CLIENTS];
	int *ptr[3];
	int totals[3] = { 0, 0, 0 };
	gclient_t *cl;
	edict_t *cl_ent;
	int teams_ping[2] = { 0, 0 };
	char team_skin_a[128];
	char team_skin_b[128];
	int offset=0;

	ptr[0] = sorted_clients[0];
	ptr[1] = sorted_clients[1];
	ptr[2] = sorted_clients[2];

	// sort the clients by score
	for ( i=0; i<game.maxclients; i++ )
	{
		cl_ent = g_edicts + 1 + i;
		if ( !cl_ent->inuse || cl_ent->client->pers.mvdspec == true )
			continue;

		totals[game.clients[i].pers.save_data.team]++;
		*ptr[game.clients[i].pers.save_data.team] = i;
		ptr[game.clients[i].pers.save_data.team]++;

		if ( game.clients[i].pers.save_data.team == TEAM_A || game.clients[i].pers.save_data.team == TEAM_B )
			teams_ping[game.clients[i].pers.save_data.team] += game.clients[i].resp.avgping;
	}

	for( i=0; i<2; i++ )
	{
		if ( totals[i] > 1 )
			qsort( sorted_clients[i], totals[i], sizeof( sorted_clients[0][0] ), DeathmatchScoreboardMessageSortCompare );

		teams_ping[i] = totals[i] > 0 ? teams_ping[i]/totals[i] : 0;
	}

	stringlength = string[0] = entry[0] = 0;

	/* TEAM_A VS TEAM_B */
	Com_sprintf( entry, sizeof( entry ), "xv %d yv %d string2 \"%13s \x0d6\x0d3 %s\"", px+120, py+0, game.teamA_name, game.teamB_name );
	j = strlen( entry );
	if ( stringlength + j > 1024 )
		return;

	strcpy (string + stringlength, entry);
	stringlength += j;

	/* TEAM_A face tag */
	if ( totals[TEAM_A] )
		Com_sprintf( entry, sizeof( entry ), "client %i %i %i 0 0 0 xv %i picn %s ", px+67, py+12, sorted_clients[TEAM_A][0], px+99, domination->value ? "ctfsb1" : "tag1" );
	else
		Com_sprintf( entry, sizeof( entry ), "xv %d yv %d picn \"/players/male/grunt_i.pcx\" xv %i picn %s ", px+67, py+12, px+99, domination->value ? "ctfsb1" : "tag1" );

	j = strlen(entry);
	if (stringlength + j > 1024)
		return;
	strcpy (string + stringlength, entry);
	stringlength += j;

	/* TEAM_B face tag */
	if ( totals[TEAM_B] )
		Com_sprintf( entry, sizeof( entry ),"client %i %i %i 0 0 0 xv %i picn %s yv %d ", px+253, py+12, sorted_clients[TEAM_B][0], px+285, domination->value ? "ctfsb2" : "tag1", py+16);
	else
		Com_sprintf (entry, sizeof(entry),"xv %d picn \"/players/female/athena_i.pcx\" xv %i picn %s yv %d ", px+253, px+285, domination->value ? "ctfsb2" : "tag1", py+16);

	j = strlen(entry);
	if (stringlength + j > 1024)
		return;
	strcpy (string + stringlength, entry);
	stringlength += j;

	/* TEAM_A score */
	entry[0] = 0;
	num_to_pics( entry, sizeof( entry ), px+139, level.teamA_score );
	Com_sprintf( entry+strlen( entry ), sizeof( entry )-strlen( entry ), "xv %d picn num_minus ", px+232 );
	j = strlen(entry);
	if (stringlength + j > 1024)
		return;
	strcpy (string + stringlength, entry);
	stringlength += j;

	/* TEAM_B score */
	entry[0] = 0;
	num_to_pics( entry, sizeof( entry ), px+325, level.teamB_score );
	j = strlen(entry);
	if (stringlength + j > 1024)
		return;
	strcpy (string + stringlength, entry);
	stringlength += j;

	team_skin_a[0] = '\0';
	team_skin_b[0] = '\0';
	sprintf(team_skin_a, "%s (%s)", game.teamA_name, game.teamA_skin);
	sprintf(team_skin_b, "%s (%s)", game.teamB_name, game.teamB_skin);

	offset=6+1;

	/* TEAM_A table header */
	if (totals[TEAM_A] != 0)
	{
		Com_sprintf (entry, sizeof(entry), "yv %d xv %d string \"%-32s %3d\"yv %d string2 \"\x01dID\x01e\x01eNAME\x01e\x01e\x01e\x01e\x01e\x01e\x01e\x01e\x01e\x01e\036FRG\036DTH\036NET\x01ePING\x01f\"", py+offset*8, px+92, team_skin_a, (int)teams_ping[TEAM_A], py+(offset+1)*8 );
		j = strlen(entry);
		if (stringlength + j > 1024)
			return;
		strcpy (string + stringlength, entry);
		stringlength += j;
		offset+=2;
	}

	/* TEAM_A players */
	for (i=0; i<totals[TEAM_A]; i++)
	{
		char highlight = 0;
		cl = &game.clients[sorted_clients[TEAM_A][i]];
		cl_ent = g_edicts + 1 + sorted_clients[TEAM_A][i];

		if ((cl_ent == game.teamA_captain && ent->client->pers.save_data.highlight == 0) || (cl_ent == ent && ent->client->pers.save_data.highlight == 1))
			highlight = 1;


		if ( level.match_state < MATCH )
			Com_sprintf (entry, sizeof(entry), "yv %d %s \"%3d %-15s  is %s %3d\"", py+offset*8, highlight ? "string2" : "string", cl_ent->s.number-1, cl->pers.netname, cl->pers.save_data.ready_or_not ? "READY   " : "NOTREADY", cl->ping);
		else
			Com_sprintf (entry, sizeof(entry), "yv %d %s \"%3d %-15s %3d %3d %3d %4d\"", py+offset*8, highlight ? "string" : "string2", cl_ent->s.number-1, cl->pers.netname, cl->resp.score, cl->resp.net, cl->resp.score-cl->resp.net, cl->ping);
		j = strlen(entry);
		if (stringlength + j > 1024)
		{
			gi.WriteByte (svc_layout);
			gi.WriteString (string);
			return;
		}
		strcpy (string + stringlength, entry);
		stringlength += j;
		offset += 1;
	}
	if (totals[TEAM_A])
		offset+=2;

	/* TEAM_B table header */
	if (totals[TEAM_B] != 0)
	{
		Com_sprintf (entry, sizeof(entry), "yv %d xv %d string \"%-32s %3d\"yv %d string2 \"\x01dID\x01e\x01eNAME\x01e\x01e\x01e\x01e\x01e\x01e\x01e\x01e\x01e\x01e\036FRG\036DTH\036NET\x01ePING\x01f\"", py+offset*8, px+92, team_skin_b, (int)teams_ping[TEAM_B], py+(offset+1)*8 );
		j = strlen(entry);
		if (stringlength + j > 1024)
		{
			gi.WriteByte (svc_layout);
			gi.WriteString (string);
			return;
		}
		strcpy (string + stringlength, entry);
		stringlength += j;
		offset+=2;
	}

	/* TEAM_B players */
	for (i=0; i<totals[TEAM_B]; i++)
	{
		char highlight = 0;

		cl = &game.clients[sorted_clients[TEAM_B][i]];
		cl_ent = g_edicts + 1 + sorted_clients[TEAM_B][i];

		if ( ( cl_ent == game.teamB_captain && ent->client->pers.save_data.highlight == 0) || ( cl_ent == ent && ent->client->pers.save_data.highlight == 1 ))
			highlight = 1;

		if ( level.match_state < MATCH )
			Com_sprintf( entry, sizeof( entry ), "yv %d %s \"%3d %-15s  is %s %3d\"", py+offset*8, highlight ? "string2" : "string", cl_ent->s.number-1, cl->pers.netname, cl->pers.save_data.ready_or_not ? "READY   " : "NOTREADY", cl->ping);
		else
			Com_sprintf (entry, sizeof(entry), "yv %d %s \"%3d %-15s %3d %3d %3d %4d\"", py+offset*8, highlight ? "string" : "string2", cl_ent->s.number-1, cl->pers.netname, cl->resp.score, cl->resp.net, cl->resp.score-cl->resp.net, cl->ping);
		j = strlen(entry);
		if (stringlength + j > 1024)
		{
			gi.WriteByte (svc_layout);
			gi.WriteString (string);
			return;
		}
		strcpy (string + stringlength, entry);
		stringlength += j;
		offset += 1;
	}
	if ( totals[TEAM_B] )
		offset+=1;

	/* SPECTATORS */
	for (i=0; i<totals[TEAM_NONE]; i++)
	{
		cl_ent = g_edicts + 1 + sorted_clients[TEAM_NONE][i];

		offset++;
		if ( cl_ent->client->chase_target && cl_ent->client->chase_target->inuse == true )
				Com_sprintf (entry, sizeof(entry), "yv %d xv %d string \"%d: %s:%d->%s\"", py+8*offset, px+240-((strlen(cl_ent->client->pers.netname)+7+strlen(cl_ent->client->chase_target->client->pers.netname))*8)/2, cl_ent->s.number-1, cl_ent->client->pers.netname, cl_ent->client->ping, cl_ent->client->chase_target->client->pers.netname);
		else
			Com_sprintf (entry, sizeof(entry), "yv %d xv %d string \"%d: %s:%d\"", py+8*offset, px+240-((strlen(cl_ent->client->pers.netname)+2+3)*8)/2, cl_ent->s.number-1, cl_ent->client->pers.netname, cl_ent->client->ping);
		j = strlen(entry);
		if (stringlength + j > 1024)
		{
			gi.WriteByte (svc_layout);
			gi.WriteString (string);
			return;
		}
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
//	gi.dprintf("%d\n", stringlength);
}

/*
** 3.D - Old Scores
*/

void SortOldScores(void)
{
	int		i,j,k, score, net;
	int		sortedscores_a[MAX_CLIENTS], sortednets_a[MAX_CLIENTS];
	int		sortedscores_b[MAX_CLIENTS], sortednets_b[MAX_CLIENTS];
	qboolean	more;
	int		total_a = 0;
	int		total_b = 0;

	old_scores.team_a_avgping = 0;

	for (i=0 ; i<old_scores.num_playersA ; i++)
	{
		score = old_scores.playersA[i].score;
		net = old_scores.playersA[i].score - old_scores.playersA[i].net;

		old_scores.team_a_avgping += old_scores.playersA[i].avg_ping;

		more=false;

		for (j=0; j<total_a; j++)
		{
			if (score < sortedscores_a[j])
				continue;

			if (score == sortedscores_a[j] && net < sortednets_a[j])
				continue;

			more = true;
			break;
		}

		if (more)
		{
			for(k=total_a; k > j; k--)
			{
				sortedscores_a[k] = sortedscores_a[k-1];
				sortednets_a[k] = sortednets_a[k-1];
				old_scores.sorted_A[k] = old_scores.sorted_A[k-1];
			}
		}

		sortedscores_a[j] = score;
		sortednets_a[j] = net;
		old_scores.sorted_A[j] = i;
		total_a++;
	}

	old_scores.team_b_avgping = 0;

	for (i=0 ; i<old_scores.num_playersB ; i++)
	{
		score = old_scores.playersB[i].score;
		net = old_scores.playersB[i].score - old_scores.playersB[i].net;

		old_scores.team_b_avgping += old_scores.playersB[i].avg_ping;

		more=false;

		for (j=0; j<total_b; j++)
		{
			if (score < sortedscores_b[j])
				continue;

			if (score == sortedscores_b[j] && net < sortednets_b[j])
				continue;

			more = true;
			break;
		}

		if (more)
		{
			for(k=total_b; k > j; k--)
			{
				sortedscores_b[k] = sortedscores_b[k-1];
				sortednets_b[k] = sortednets_b[k-1];
				old_scores.sorted_B[k] = old_scores.sorted_B[k-1];
			}
		}

		sortedscores_b[j] = score;
		sortednets_b[j] = net;
		old_scores.sorted_B[j] = i;
		total_b++;
	}
	if (total_a)
		old_scores.team_a_avgping = (int)old_scores.team_a_avgping/total_a;

	if (total_b)
		old_scores.team_b_avgping = (int)old_scores.team_b_avgping/total_b;
}

void AddPlayerToOldScore(edict_t *ent)
{
	int			found = 0;
	int			i;

	if ((level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == SUDDENDEATH || level.match_state == END) && ent->client->pers.save_data.team != TEAM_NONE)
	{
		if (ent->client->pers.save_data.team == TEAM_A)
		{
			for (i=0; i<old_scores.num_playersA; i++)
			{
				if (!strcmp(ent->client->pers.netname, old_scores.playersA[i].name))
				{
					found = 1;
					break;
				}
			}
		}
		else
		{
			for (i=0; i<old_scores.num_playersB; i++)
			{
				if (!strcmp(ent->client->pers.netname, old_scores.playersB[i].name))
				{
					found = 2;
					break;
				}
			}
		}
	
		if (!found)
		{
			if (ent->client->pers.save_data.team == TEAM_A && old_scores.num_playersA == 0)
				old_scores.playersA = malloc(sizeof(team_players));
			else if (ent->client->pers.save_data.team == TEAM_A)
				old_scores.playersA = realloc(old_scores.playersA, sizeof(team_players)*(old_scores.num_playersA+1));
			else if (ent->client->pers.save_data.team == TEAM_B && old_scores.num_playersB == 0)
				old_scores.playersB = malloc(sizeof(team_players));
			else if (ent->client->pers.save_data.team == TEAM_B)
				old_scores.playersB = realloc(old_scores.playersB, sizeof(team_players)*(old_scores.num_playersB+1));

			if (ent->client->pers.save_data.team == TEAM_A)
			{
				old_scores.playersA[old_scores.num_playersA].avg_ping = (int)ent->client->resp.avgping;
				old_scores.playersA[old_scores.num_playersA].ping = ent->client->ping;
				strcpy(old_scores.playersA[old_scores.num_playersA].name, ent->client->pers.netname);
				old_scores.playersA[old_scores.num_playersA].net = ent->client->resp.net;
				old_scores.playersA[old_scores.num_playersA].score = ent->client->resp.score;
				old_scores.num_playersA++;
			}
			else
			{
				old_scores.playersB[old_scores.num_playersB].avg_ping = (int)ent->client->resp.avgping;
				old_scores.playersB[old_scores.num_playersB].ping = ent->client->ping;
				strcpy(old_scores.playersB[old_scores.num_playersB].name, ent->client->pers.netname);
				old_scores.playersB[old_scores.num_playersB].net = ent->client->resp.net;
				old_scores.playersB[old_scores.num_playersB].score = ent->client->resp.score;
				old_scores.num_playersB++;
			}
		}
		else if (found == 1)
		{
			old_scores.playersA[i].avg_ping = (int)ent->client->resp.avgping;
			old_scores.playersA[i].ping = ent->client->ping;
			old_scores.playersA[i].net = ent->client->resp.net;
			old_scores.playersA[i].score = ent->client->resp.score;
		}
		else if (found == 2)
		{
			old_scores.playersB[i].avg_ping = (int)ent->client->resp.avgping;
			old_scores.playersB[i].ping = ent->client->ping;
			old_scores.playersB[i].net = ent->client->resp.net;
			old_scores.playersB[i].score = ent->client->resp.score;
		}
	}
}

void CreateOldScores (void)
{
	int		i;
	edict_t *ent;

	if (level.num_A_players == 0 || level.num_B_players == 0)
	{
		FreeOldScores();
		return;
	}

	for (i=0 ; i<game.maxclients ; i++)
	{
		ent = &g_edicts[i+1];

		if (!ent->inuse || ent->client->pers.mvdspec)
			continue;

		if (ent->client->pers.save_data.team == TEAM_NONE)
		{
			if (old_scores.num_spectators == 0)
				old_scores.spectators = malloc(sizeof(spectator_player));
			else if (old_scores.num_spectators > 0)
				old_scores.spectators = realloc(old_scores.spectators, sizeof(spectator_player)*(old_scores.num_spectators+1));

			strcpy(old_scores.spectators[old_scores.num_spectators].name, ent->client->pers.netname);

			if (ent->client->chase_target->inuse && ent->client->chase_target->client)
			{
				old_scores.spectators[old_scores.num_spectators].chasecam = true;
				strcpy(old_scores.spectators[old_scores.num_spectators].chasing, ent->client->chase_target->client->pers.netname);
			}
			else
				old_scores.spectators[old_scores.num_spectators].chasecam = false;

			old_scores.spectators[old_scores.num_spectators].ping = ent->client->ping;
			old_scores.num_spectators++;

			continue;
		}

		AddPlayerToOldScore(ent);
	}

	strcpy(old_scores.teamA_name,game.teamA_name);
	strcpy(old_scores.teamB_name,game.teamB_name);
	old_scores.team_a_score = level.teamA_score;
	old_scores.team_b_score = level.teamB_score;
	strcpy(old_scores.teamA_skin, game.teamA_skin);
	strcpy(old_scores.teamB_skin, game.teamB_skin);
	strcpy(old_scores.level_name, level.level_name);
	strcpy(old_scores.mapname, level.mapname);

	SortOldScores();

	game.is_old_score = true;
}

void FreeOldScores(void)
{
	old_scores.num_playersA = 0;
	old_scores.num_playersB = 0;
	old_scores.num_spectators = 0;

	if (old_scores.playersA)
		free(old_scores.playersA);
	if (old_scores.playersB)
		free(old_scores.playersB);
	if (old_scores.spectators)
		free(old_scores.spectators);

	memset (&old_scores, 0, sizeof(old_scores));

	game.is_old_score = false;
}

void MatchEndInfo(void)
{
	char	match_end_info[1024];
	char	string[1024], entry[1024];
	int		j, stringlength=0;
	time_t	rawtime;
	struct	tm * timeinfo;
	cvar_t	*hostname;
	char * const wday_name[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	char * const mon_name[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	Com_sprintf(entry, sizeof(entry), "%s defeats %s by %d frag%s", level.teamA_score > level.teamB_score ? game.teamA_name : game.teamB_name,
																									level.teamA_score > level.teamB_score ? game.teamB_name : game.teamA_name,
																									abs(level.teamA_score-level.teamB_score), abs(level.teamA_score-level.teamB_score) > 1 ? "s" : "");

	Com_sprintf(match_end_info, sizeof(match_end_info), "yv -40 xv %d string \"%s\" ", 160-((strlen(entry)*8)/2), entry);
	stringlength += strlen(match_end_info);

	if (old_scores.team_a_avgping != old_scores.team_b_avgping)
		Com_sprintf(entry, sizeof(entry), "with %dms ping %s", old_scores.team_a_avgping > old_scores.team_b_avgping ? old_scores.team_a_avgping-old_scores.team_b_avgping : old_scores.team_b_avgping-old_scores.team_a_avgping,
																							level.teamA_score > level.teamB_score ? (old_scores.team_a_avgping < old_scores.team_b_avgping ? "advantage" : "disavantage") : (old_scores.team_b_avgping < old_scores.team_a_avgping ? "advantage" : "disavantage"));
	else
		Com_sprintf(entry, sizeof(entry), "with no ping adventage");

	Com_sprintf(string, sizeof(string), "yv -32 xv %d string \"%s\" ", 160-((strlen(entry)*8)/2), entry);

	j = strlen(string);
	if (stringlength + j > 1024)
	{
		gi.configstring(CS_STATUSBAR, match_end_info);
		return;
	}
	strcat (match_end_info, string);
	stringlength += j;

	Com_sprintf(entry, sizeof(entry), "%s - %s ", level.mapname, level.level_name);

	hostname = gi.cvar("hostname", "", 0);

	Com_sprintf(string, sizeof(string), "yv -72 xv %d string2 \"%s\" yv -64 xv %d string2 \"%s\" ", 160-((strlen(hostname->string)*8)/2), hostname->string, 160-((strlen(entry)*8)/2), entry);

	j = strlen(string);
	if (stringlength + j > 1024)
	{
		gi.configstring(CS_STATUSBAR, match_end_info);
		return;
	}
	strcat (match_end_info, string);
	stringlength += j;

	Com_sprintf(entry, sizeof(entry), "%s %s %d %.2d:%.2d:%.2d %02d", wday_name[timeinfo->tm_wday], mon_name[timeinfo->tm_mon],
																		timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min,
																		timeinfo->tm_sec, abs(100 - timeinfo->tm_year));

	Com_sprintf(string, sizeof(string), "yv -56 xv %d string2 \"(%s)\" ", 160-((strlen(entry)*8)/2), entry);

	j = strlen(string);
	if (stringlength + j > 1024)
	{
		gi.configstring(CS_STATUSBAR, match_end_info);
		return;
	}
	strcat (match_end_info, string);
	stringlength += j;

	Com_sprintf(entry, sizeof(entry), "%s (%s) by Harven, onimusha & As_Best.", TDMVERSION, __DATE__);

	Com_sprintf(string, sizeof(string), "yb -10 xl 2 string2 \"%s\" ", entry);

	j = strlen(string);
	if (stringlength + j > 1024)
	{
		gi.configstring(CS_STATUSBAR, match_end_info);
		return;
	}
	strcat (match_end_info, string);
	stringlength += j;

	gi.configstring(CS_STATUSBAR, match_end_info);
}

void DisplayOldScore(edict_t *ent)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j;
	int		px=-80, py=-40;
	char	lp=29;
	char	ls=30;
	char	l_k=31;
	char	team_a_score[5];
	char	team_b_score[5];
	char	team_skin_a[128];
	char	team_skin_b[128];
	char	vs[3];
	int		offset=0;
	int		eff=0;
	int		player_a = -1, player_b = -1;

	
	if (level.match_state == END)
		py = -16;

	string[0] = 0;

	stringlength = strlen(string);
	sprintf(vs, "VS");

	for (i=0; i<strlen(vs); i++)
		vs[i] |= 128;

	Com_sprintf (entry, sizeof(entry),"xv %d yv %d string2 \"%13s %s %s\"", px+120, py+0, game.teamA_name, vs, game.teamB_name);
	j = strlen(entry);
	if (stringlength + j > 1024)
		return;
	strcpy (string + stringlength, entry);
	stringlength += j;

	for (i=0; i<game.maxclients; i++)
	{
		if (!g_edicts[i+1].inuse)
			continue;

		if (g_edicts[i+1].client->pers.save_data.team == TEAM_A)
			player_a = i;

		if (g_edicts[i+1].client->pers.save_data.team == TEAM_B)
			player_b = i;
	}

	if (player_a >= 0)
	{
		if (domination->value)
			Com_sprintf (entry, sizeof(entry),"client %i %i %i 0 0 0 xv %i picn ctfsb1 ", px+67, py+12, player_a, px+99);
		else
			Com_sprintf (entry, sizeof(entry),"client %i %i %i 0 0 0 xv %i picn tag1 ", px+67, py+12, player_a, px+99);
		j = strlen(entry);
		if (stringlength + j > 1024)
			return;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}
	else
	{
		if (domination->value)
			Com_sprintf (entry, sizeof(entry),"xv %d yv %d picn \"/players/male/grunt_i.pcx\" xv %i picn ctfsb1 ", px+67, py+12, px+99);
		else
			Com_sprintf (entry, sizeof(entry),"xv %d yv %d picn \"/players/male/grunt_i.pcx\" xv %i picn tag1 ", px+67, py+12, px+99);
		j = strlen(entry);
		if (stringlength + j > 1024)
			return;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	if (player_b >= 0)
	{
		if (domination->value)
			Com_sprintf (entry, sizeof(entry),"client %i %i %i 0 0 0 xv %i picn ctfsb2 yv %d ", px+253, py+12, player_b, px+285, py+16);
		else
			Com_sprintf (entry, sizeof(entry),"client %i %i %i 0 0 0 xv %i picn tag1 yv %d ", px+253, py+12, player_b, px+285, py+16);
		j = strlen(entry);
		if (stringlength + j > 1024)
			return;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}
	else
	{
		if (domination->value)
			Com_sprintf (entry, sizeof(entry),"xv %d picn \"/players/female/athena_i.pcx\" xv %i picn ctfsb2 yv %d ", px+253, px+285, py+16);
		else
			Com_sprintf (entry, sizeof(entry),"xv %d picn \"/players/female/athena_i.pcx\" xv %i picn tag1 yv %d ", px+253, px+285, py+16);
		j = strlen(entry);
		if (stringlength + j > 1024)
			return;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	team_a_score[0] = '\0';
	Com_sprintf(team_a_score, 5, "%d", old_scores.team_a_score);
	if (old_scores.team_a_score > 99)
		Com_sprintf (entry, sizeof(entry),"xv %d picn num_%c xv %d picn num_%c xv %d picn num_%c xv %d picn num_minus ", px+139, team_a_score[0], px+155, team_a_score[1], px+171, team_a_score[2], px+232);
	else if (old_scores.team_a_score > 9)
		Com_sprintf (entry, sizeof(entry),"xv %d picn num_%c xv %d picn num_%c xv %d picn num_minus ", px+147, team_a_score[0], px+163, team_a_score[1], px+232);
	else if (old_scores.team_a_score >= 0)
		Com_sprintf (entry, sizeof(entry),"xv %d picn num_%c xv %d picn num_minus ", px+155, team_a_score[0], px+232);
	else if (old_scores.team_a_score < 0 && old_scores.team_a_score >= -9)
		Com_sprintf (entry, sizeof(entry),"xv %d picn num_minus xv %d picn num_%c xv %d picn num_minus ", px+147, px+163, team_a_score[1], px+232);
	else if (old_scores.team_a_score < -9)
		Com_sprintf (entry, sizeof(entry),"xv %d picn num_minus xv %d picn num_%c xv %d picn num_%c xv %d picn num_minus ", px+139, px+155, team_a_score[1], px+171,  team_a_score[2], px+232);
	j = strlen(entry);
	if (stringlength + j > 1024)
		return;
	strcpy (string + stringlength, entry);
	stringlength += j;


	team_b_score[0] = '\0';
	Com_sprintf(team_b_score, 5, "%d", old_scores.team_b_score);
	if (old_scores.team_b_score > 99)
		Com_sprintf (entry, sizeof(entry),"xv %d picn num_%c xv %d picn num_%c xv %d picn num_%c ", px+325, team_b_score[0], px+341, team_b_score[1], px+357, team_b_score[2]);
	else if (old_scores.team_b_score > 9)
		Com_sprintf (entry, sizeof(entry),"xv %d picn num_%c xv %d picn num_%c ", px+333, team_b_score[0], px+349, team_b_score[1]);
	else if (old_scores.team_b_score >= 0)
		Com_sprintf (entry, sizeof(entry),"xv %d picn num_%c ", px+341, team_b_score[0]);
	else if (old_scores.team_b_score < 0 && old_scores.team_b_score >= -9)
		Com_sprintf (entry, sizeof(entry),"xv %d picn num_minus xv %d picn num_%c ", px+333, px+349, team_b_score[1]);
	else if (old_scores.team_b_score < -9)
		Com_sprintf (entry, sizeof(entry),"xv %d picn num_minus xv %d picn num_%c xv %d picn num_%c ", px+325, px+341, team_b_score[1], px+357, team_b_score[2]);
	j = strlen(entry);
	if (stringlength + j > 1024)
		return;
	strcpy (string + stringlength, entry);
	stringlength += j;

	team_skin_a[0] = '\0';
	team_skin_b[0] = '\0';
	sprintf(team_skin_a, "%s (%s)", old_scores.teamA_name, old_scores.teamA_skin);
	sprintf(team_skin_b, "%s (%s)", old_scores.teamB_name, old_scores.teamB_skin);

	offset=6;

	if (old_scores.num_playersA != 0)
	{
		Com_sprintf (entry, sizeof(entry), "yv %d xv %d string \"%-31s AVGP: %3d\"yv %d string2 \"%cNAME%c%c%c%c%c%c%c%c%c%c%cFRG%cNET%cDTH%cEFF%c%cAVGP%c\"", py+offset*8, px+74, team_skin_a, old_scores.team_a_avgping, py+(offset+1)*8, lp, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, l_k);
		j = strlen(entry);
		if (stringlength + j > 1024)
			return;
		strcpy (string + stringlength, entry);
		stringlength += j;
		offset+=2;
	}

	for (i=0 ; i<old_scores.num_playersA ; i++)
	{
		if (old_scores.playersA[old_scores.sorted_A[i]].score > 0)
			eff = (int)(old_scores.playersA[old_scores.sorted_A[i]].score*100)/(old_scores.playersA[old_scores.sorted_A[i]].score+old_scores.playersA[old_scores.sorted_A[i]].net);
		else
			eff = 0;

		Com_sprintf (entry, sizeof(entry), "yv %d string \" %-15s%3d %3d %3d %3d%% %4d\"", py+offset*8,
																						old_scores.playersA[old_scores.sorted_A[i]].name,
																						old_scores.playersA[old_scores.sorted_A[i]].score,
																						old_scores.playersA[old_scores.sorted_A[i]].score-old_scores.playersA[old_scores.sorted_A[i]].net,
																						old_scores.playersA[old_scores.sorted_A[i]].net,
																						eff,
																						old_scores.playersA[old_scores.sorted_A[i]].avg_ping);

		j = strlen(entry);
		if (stringlength + j > 1024)
		{
			gi.WriteByte (svc_layout);
			gi.WriteString (string);
			gi.unicast (ent, true);
			return;
		}
		strcpy (string + stringlength, entry);
		stringlength += j;
		offset += 1;
	}

	if (old_scores.num_playersA)
		offset+=2;

	if (old_scores.num_playersB != 0)
	{
		Com_sprintf (entry, sizeof(entry), "yv %d xv %d string \"%-31s AVGP: %3d\"yv %d string2 \"%cNAME%c%c%c%c%c%c%c%c%c%c%cFRG%cNET%cDTH%cEFF%c%cAVGP%c\"", py+offset*8, px+74, team_skin_b, old_scores.team_b_avgping, py+(offset+1)*8, lp, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, ls, l_k);
		j = strlen(entry);
		if (stringlength + j > 1024)
		{
			gi.WriteByte (svc_layout);
			gi.WriteString (string);
			gi.unicast (ent, true);
			return;
		}
		strcpy (string + stringlength, entry);
		stringlength += j;
		offset+=2;
	}


	for (i=0 ; i<old_scores.num_playersB; i++)
	{
		if (old_scores.playersB[old_scores.sorted_B[i]].score > 0)
			eff = (int)(old_scores.playersB[old_scores.sorted_B[i]].score*100)/(old_scores.playersB[old_scores.sorted_B[i]].score+old_scores.playersB[old_scores.sorted_B[i]].net);
		else
			eff = 0;

		Com_sprintf (entry, sizeof(entry), "yv %d string \" %-15s%3d %3d %3d %3d%% %4d\"", py+offset*8,
																						old_scores.playersB[old_scores.sorted_B[i]].name,
																						old_scores.playersB[old_scores.sorted_B[i]].score,
																						old_scores.playersB[old_scores.sorted_B[i]].score-old_scores.playersB[old_scores.sorted_B[i]].net,
																						old_scores.playersB[old_scores.sorted_B[i]].net,
																						eff,
																						old_scores.playersB[old_scores.sorted_B[i]].avg_ping);

		j = strlen(entry);
		if (stringlength + j > 1024)
		{
			gi.WriteByte (svc_layout);
			gi.WriteString (string);
			gi.unicast (ent, true);
			return;
		}
		strcpy (string + stringlength, entry);
		stringlength += j;
		offset += 1;
	}

	for (i=0; i<old_scores.num_spectators; i++)
	{
		offset++;
		if (old_scores.spectators[i].chasing)
			Com_sprintf (entry, sizeof(entry), "yv %d xv %d string \"%s:%d->%s\"", py+8*offset, px+240-((strlen(old_scores.spectators[i].name)+7+strlen(old_scores.spectators[i].chasing))*8)/2, old_scores.spectators[i].name, old_scores.spectators[i].ping, old_scores.spectators[i].chasing);
		else
			Com_sprintf (entry, sizeof(entry), "yv %d xv %d string \"%s:%d\"", py+8*offset, px+240-((strlen(old_scores.spectators[i].name)+2+3)*8)/2, old_scores.spectators[i].name, old_scores.spectators[i].ping);

		j = strlen(entry);
		if (stringlength + j > 1024)
		{
			gi.WriteByte (svc_layout);
			gi.WriteString (string);
			gi.unicast (ent, true);
			return;
		}
		strcpy (string + stringlength, entry);
		stringlength += j;
	}
			
	gi.WriteByte (svc_layout);
	gi.WriteString (string);
	gi.unicast (ent, true);
}

void Cmd_OldScore_f (edict_t *ent)
{
	ent->client->showinventory = false;
	ent->client->showhelp = false;
	ent->client->showmenu = false;
	ent->client->showsets = false;
	ent->client->showid = false;
	ent->client->showscores = false;

	if (game.is_old_score && !ent->client->showoldscore)
	{
		ent->client->showoldscore = true;
		DisplayOldScore(ent);
	}
	else
	{
		ent->client->showscores = false;
		if (ent->client->chase_target)
			ent->client->update_chase = true;
	}
}

/*
** 3.E - Calc_AvgPing
*/

void Calc_AvgPing (edict_t *ent)
{
	ent->client->resp.old_ping = ent->client->ping;
	ent->client->resp.old_avgping = ent->client->resp.avgping;

	if (ent->client->ping != 0)
	{
		int		pingnum;
		float		pings;

		if (level.match_state == MATCH || level.match_state == END || level.match_state == OVERTIME || level.match_state == SUDDENDEATH)
			pingnum = level.framenum-level.start_framenum;
		else
			pingnum = level.framenum-ent->client->resp.enterframe-1;

		if (ent->client->resp.old_avgping == 0.0)
			ent->client->resp.old_avgping = (float)ent->client->ping;

		pings = ent->client->resp.old_avgping*((float)pingnum);

		ent->client->resp.avgping = (pings+ent->client->ping)/((float)pingnum+1);
	}
}


/*
** 3.F - Player ID
*/

void Cmd_Id_f (edict_t *ent)
{
	char *pos_x;
	char *pos_y;

	if (gi.argc() >= 3)
	{
		pos_x = gi.argv(1);
		pos_y = gi.argv(2);

		ent->client->pers.save_data.id_x = atoi(pos_x);
		ent->client->pers.save_data.id_y = atoi(pos_y);
		if (ent->client->pers.save_data.hudid == false)
		{
			gi.cprintf (ent, PRINT_HIGH, "Player ID tagging enabled.\n");
			ent->client->pers.save_data.hudid = true;
		}
	}
	else if (ent->client->pers.save_data.hudid == false)
	{
			ent->client->pers.save_data.id_x = 0;
			ent->client->pers.save_data.id_y = 0;

			gi.cprintf (ent, PRINT_HIGH, "Player ID tagging enabled.\n");
			ent->client->pers.save_data.hudid = true;
	}
	else
	{
		gi.cprintf (ent, PRINT_HIGH, "Player ID tagging disabled.\n");
		ent->client->pers.save_data.hudid = false;
	}
}

qboolean IsPlayerVisible( edict_t *ent1, edict_t *ent2, vec3_t *start )
{
	trace_t tr;
	vec3_t end, right, forward, dir;

	if ( !ent1 || !ent1->client || !ent2 || !ent2->client )
		return false;
	
	VectorCopy( ent2->s.origin, end );
	VectorSubtract( ent1->s.origin, ent2->s.origin, dir );
	VectorNormalize( dir );
	AngleVectors( dir, forward, right, NULL );

	tr = gi.trace( *start, NULL, NULL, end, ent1, MASK_SHOT );
	if ( tr.fraction < 1.0 && tr.ent && tr.ent == ent2 )
		return true;

	VectorMA( end, 5, forward, end ); //move endpoint few units farther
	VectorMA( end, 10, right, end ); //move little bit to the right
	tr = gi.trace( *start, NULL, NULL, end, ent1, MASK_SHOT );
	if ( tr.fraction < 1.0 && tr.ent && tr.ent == ent2 )
		return true;

	VectorMA( end, -20, right, end ); //move little bit to the left
	tr = gi.trace( *start, NULL, NULL, end, ent1, MASK_SHOT );
	if ( tr.fraction < 1.0 && tr.ent && tr.ent == ent2 )
		return true;

	VectorMA( end, 10, right, end ); //and back to the begining
	VectorMA( end, -5, forward, end );
	end[2] += 30;
	tr = gi.trace( *start, NULL, NULL, end, ent1, MASK_SHOT );
	if ( tr.fraction < 1.0 && tr.ent && tr.ent == ent2 )
		return true;

	end[2] -= 52;
	tr = gi.trace( *start, NULL, NULL, end, ent1, MASK_SHOT );
	if ( tr.fraction < 1.0 && tr.ent && tr.ent == ent2 )
		return true;

	return false;
}

void ShowPlayerId(edict_t *ent)
{
	vec3_t start, forward, end, range, radius;
	unsigned int closest = 0xffffffff;
	int pos,pos2,i;
	char text[128];
	char stats[64];
	float distance_waight = 0;
#ifdef TEXTURES_NAMES
	trace_t	tr;
#endif
	gitem_t *it, *ps;
	qboolean shield = false;
	qboolean shieldon = false;
	qboolean isVisible = false;
	edict_t *other = NULL;

	if (ent->client->chase_target && ent->client->pers.save_data.team == TEAM_NONE)
		return;

	if (ent->flags & FL_POWER_ARMOR)
		shieldon = true;

	if (!ent->client->pers.save_data.hudid)
		return;

	if (ent->client->showoldscore || ent->client->showscores || ent->client->showmenu || ent->client->showinventory || ent->client->showhelp)
		return;

	VectorCopy( ent->s.origin, start );
	start[2] += ent->viewheight;
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
#ifdef TEXTURES_NAMES
	VectorMA(start, 8192, forward, end);
	if (ent->client->resp.showtexture_name)
		tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT|MASK_WATER);
	else 
#endif
	for (i=1; i<=game.maxclients; i++)
	{
		edict_t *cl = &g_edicts[i];
		float tmp_dist = 0;

		if ( cl == ent || !cl->inuse || cl->client->pers.save_data.team == TEAM_NONE )
			continue;

		VectorSubtract( ent->s.origin, cl->s.origin, range );
		VectorMA( ent->s.origin, VectorLength( range ), forward, end );
		VectorSubtract( cl->s.origin, end, radius );

		//gi.cprintf( ent, PRINT_HIGH, "D=%f, H=%f H/D^2=%f\n", VectorLength( dir2 ), VectorLength( dir ), VectorLength( dir )/(VectorLength( dir2 )*VectorLength( dir2 )) );

		tmp_dist = VectorLength( range )/(VectorLength( radius )*VectorLength( radius ));

		if ( tmp_dist < 0.02f )
			continue;

		if ( !IsPlayerVisible( ent, cl, &start ) )
			continue;

		if ( tmp_dist > distance_waight )
		{
			other = &g_edicts[i];
			distance_waight = tmp_dist;
		}
	}

	if (other && other->client && other->client->pers.save_data.team == ent->client->pers.save_data.team)
	{
		isVisible = true;
		if (!(level.framenum & 15) || ent->client->showid == false)
		{
			ps = FindItem("Power Shield");
			if (other->client->pers.inventory[ITEM_INDEX(ps)] > 0)
				shield = true;
			if (shield)
				ps = FindItem("Cells");

			it = FindItem("Jacket Armor");
			if (other->client->pers.inventory[ITEM_INDEX(it)] > 0)
			{
				if (shield && shieldon)
					sprintf(stats, "%d/%dPS/%dJA", other->health, other->client->pers.inventory[ITEM_INDEX(ps)], other->client->pers.inventory[ITEM_INDEX(it)]);
				else if (shield && !shieldon)
					sprintf(stats, "%d/PS OFF/%dJA", other->health, other->client->pers.inventory[ITEM_INDEX(it)]);
				else
					sprintf(stats, "%d/%dJA", other->health, other->client->pers.inventory[ITEM_INDEX(it)]);
			}
			else
			{
				it = FindItem("Combat Armor");
				if (other->client->pers.inventory[ITEM_INDEX(it)] > 0)
				{
					if (shield && shieldon)
						sprintf(stats, "%d/%dPS/%dYA", other->health, other->client->pers.inventory[ITEM_INDEX(ps)], other->client->pers.inventory[ITEM_INDEX(it)]);
					else if (shield && !shieldon)
						sprintf(stats, "%d/PS OFF/%dYA", other->health, other->client->pers.inventory[ITEM_INDEX(it)]);
					else
						sprintf(stats, "%d/%dYA", other->health, other->client->pers.inventory[ITEM_INDEX(it)]);
				}
				else
				{
					it = FindItem("Body Armor");
					if (other->client->pers.inventory[ITEM_INDEX(it)] > 0)
					{
						if (shield && shieldon)
							sprintf(stats, "%d/%dPS/%dRA", other->health, other->client->pers.inventory[ITEM_INDEX(ps)], other->client->pers.inventory[ITEM_INDEX(it)]);
						else if (shield && !shieldon)
							sprintf(stats, "%d/PS OFF/%dRA", other->health, other->client->pers.inventory[ITEM_INDEX(it)]);
						else
							sprintf(stats, "%d/%dRA", other->health, other->client->pers.inventory[ITEM_INDEX(it)]);
					}
					else if (shield && shieldon)
						sprintf(stats, "%d/%dPS", other->health, other->client->pers.inventory[ITEM_INDEX(ps)]);
					else if (shield && !shieldon)
						sprintf(stats, "%d/PS OFF", other->health);
					else
						sprintf(stats, "%d/0A", other->health);
				}
			}
			ent->client->showid = true;
			if (ent->client->pers.save_data.id_x == 0 && ent->client->pers.save_data.id_y == 0)
			{
				pos = 0;
				pos2 = 0 + strlen("Viewing ")/2 - strlen(stats)/2;
				sprintf(text, "xv %d yb -58 string2 \"Viewing %s\" xv %d yb -50 string2 %s", pos*8, other->client->pers.netname, pos2*8, stats);
			}
			else
			{
				pos = ((40 + ent->client->pers.save_data.id_x) - strlen(other->client->pers.netname))/2;
				pos2 = ((40 + ent->client->pers.save_data.id_x) - strlen(stats))/2;
				sprintf(text, "xv %d yv %d string2 %s xv %d yv %d string2 %s", pos*8, 116 + ent->client->pers.save_data.id_y, other->client->pers.netname, pos2*8, 116+ent->client->pers.save_data.id_y+8, stats);
			}
			gi.WriteByte (svc_layout);
			gi.WriteString (text);
			gi.unicast (ent, true);
		}
	}
	else if (other && other->client)
	{
		if (gi.pointcontents(ent->s.origin) == gi.pointcontents(other->s.origin) )
		{
			isVisible = true;
			if (!(level.framenum & 15) || ent->client->showid == false)
			{
				ent->client->showid = true;
				if (ent->client->pers.save_data.id_x == 0 && ent->client->pers.save_data.id_y == 0)
					sprintf(text, "xv 0 yb -58 string2 \"Viewing %s\"", other->client->pers.netname);
				else
				{
					pos = ((40 + ent->client->pers.save_data.id_x) - strlen(other->client->pers.netname))/2;
					sprintf(text, "xv %d yv %d string2 %s", pos*8, 116 + ent->client->pers.save_data.id_y, other->client->pers.netname);
				}
				gi.WriteByte (svc_layout);
				gi.WriteString (text);
				gi.unicast (ent, true);
			}
		}
	}
#ifdef TEXTURES_NAMES
	else if (ent->client->resp.showtexture_name)
	{
		isVisible = true;
		ent->client->showid = true;

		pos = (40 - strlen(tr.surface->name))/2;

		sprintf(text, "xv %d yv 135 string2 %s", pos*8, tr.surface->name);
		gi.WriteByte (svc_layout);
		gi.WriteString (text);
		gi.unicast (ent, true);
	}
#endif

	if ( !isVisible )
		ent->client->showid = false;
}

/*
** 3.G - Save Player
*/

void InitSavedPlayers(void)
{
	num_saved_players = 0;
	saved_players = NULL;
}

void FreeSavedPlayers(void)
{
	num_saved_players = 0;
	if (saved_players)
		free(saved_players);
	saved_players = NULL;
}

void SavePlayerData(edict_t *ent)
{
	int	i=0,free=-1;
	qboolean	found=false;

	if ((level.match_state == WARMUP || level.match_state == PREGAME || level.match_state == COUNTDOWN || level.match_state == END) || ent->client->pers.save_data.team == TEAM_NONE)
		return;

	if (saved_players)
	{
		for (i=0; i<num_saved_players; i++)
		{
			if (!strcmp(ent->client->pers.netname, saved_players[i].pers.netname))
			{
				found = true;
				break;
			}
			else if (!strlen(saved_players[i].pers.netname))
				free = i;
		}
	}

	if (!found && free == -1)
	{
		num_saved_players++;

		if (num_saved_players == 1)
			saved_players = (saved_player_t *)malloc(sizeof(saved_player_t));
		else
			saved_players = (saved_player_t *)realloc(saved_players, sizeof(saved_player_t)*num_saved_players);

		i = num_saved_players-1;
	}

	if (free != -1 && !found)
		i = free;

	memcpy(&saved_players[i].resp, &ent->client->resp, sizeof(client_respawn_t));
	memcpy(&saved_players[i].pers, &ent->client->pers, sizeof(client_persistant_t));
	saved_players[i].ammo_index = ent->client->ammo_index;
	saved_players[i].health = ent->health;
	saved_players[i].pers.save_data.admin_flags = 0;
	saved_players[i].pers.save_data.admin_password[0] = '\0';
	saved_players[i].pers.save_data.is_admin = false;
	saved_players[i].pers.save_data.judge = false;
}

void CheckSavedPlayer(edict_t *ent)
{
	int	i;
	qboolean	found=false;

	if (saved_players == NULL || num_saved_players == 0)
		return;

	if (level.match_state == WARMUP || level.match_state == PREGAME || level.match_state == COUNTDOWN || level.match_state == END)
		return;

	for (i=0; i<num_saved_players; i++)
	{
		if (!strcmp(ent->client->pers.netname, saved_players[i].pers.netname))
		{
			found = true;
			break;
		}
	}

	if (!found)
		return;

	memcpy(&ent->client->resp, &saved_players[i].resp, sizeof(client_respawn_t));
	memcpy(&ent->client->pers, &saved_players[i].pers, sizeof(client_persistant_t));
	ent->health = saved_players[i].health;
	memset(&saved_players[i].resp, 0, sizeof(client_respawn_t));
	memset(&saved_players[i].pers, 0, sizeof(client_persistant_t));
	if (ent->client->pers.health > 100)
		ent->client->pers.health = 100;
	ent->client->ammo_index = saved_players[i].ammo_index;
	saved_players[i].health = 0;
	saved_players[i].ammo_index = 0;

}

/*
** 4.A - Admin
*/

void SVCmd_AdminList_f(void)
{
	int	i=0;
	char	flags[6];
	struct admin_s *admin;

	if (!game.admin_first)
	{
		gi.cprintf(NULL, PRINT_HIGH, "The adminlist is empty.\n");
		return;
	}
	
	gi.cprintf(NULL, PRINT_HIGH, "\nNr | Name             | Password         | Flags | Time\n");
	gi.cprintf(NULL, PRINT_HIGH, "---+------------------+------------------+-------+------\n");

	for (admin = game.admin_first; admin; admin = admin->next)
	{
		if (strlen(admin->password))
		{
			flags[0]='\0';
			if (admin->flags & AD_ALL)
				strcat(flags, "A");
			else
			{
				if (admin->flags & AD_MATCH)
					strcat(flags, "M");
				if (admin->flags & AD_TEAMS)
					strcat(flags, "T");
				if (admin->flags & AD_SERVER)
					strcat(flags, "S");
				if (admin->flags & AD_BAN)
					strcat(flags, "B");
			}
			flags[strlen(flags)] = '\0';
			if (!admin->judge)
				gi.cprintf(NULL, PRINT_HIGH, "%-2d | %-16s | %-16s | %-5s | %-4d\n", i, admin->nick, admin->password, flags, admin->time);
			else
				gi.cprintf(NULL, PRINT_HIGH, "%-2d | REFEREE          | %-16s | %-5s | %-4d - REFEREE PASSWORD\n", i, admin->password, flags, admin->time);
		}
		i++;
	}
	gi.cprintf(NULL, PRINT_HIGH, "\nFlags: (A)ll, (M)atch, (S)erver, (T)eam, (B)an.\nTime: 0 = admin deleted, -1 = time not set.\n");
}

void SVCmd_AddAdmin_f(void)
{
	char	*name;
	char	*password;
	char	*flags;
	char	*time;
	int		i;
	int		free_slot=-1;
	qboolean badflags=false;
	struct admin_s *newAdmin;

	if (gi.argc() < 6)
	{
		gi.cprintf(NULL, PRINT_HIGH, "USAGE: addadmin <player name> <password> <num. matches> <flags>.\n");
		return;
	}

	name = gi.argv(2);

	if (strlen(name) < 1 && strlen(name) > 16)
	{
		gi.cprintf(NULL, PRINT_HIGH, "Name must be at last 1 and not more than 16 chars long.\n");
		return;
	}

	password = gi.argv(3);

	if (strlen(password) > 16 || strlen(password) < 6)
	{
		gi.cprintf(NULL, PRINT_HIGH, "Password must be at last 6 and not more than 16 chars long.\n");
		return;
	}

	for(newAdmin = game.admin_first; newAdmin; newAdmin = newAdmin->next)
	{
		if (!strcmp(newAdmin->password, password) && newAdmin->time != 0)
		{
			gi.cprintf(NULL, PRINT_HIGH, "The password is already in use, try another.\n");
			return;
		}
	}

	time = gi.argv(4);

	if (atoi(time) == 0)
	{
		gi.cprintf(NULL, PRINT_HIGH, "Time must be greater than 0 or equal -1.\n");
		return;
	}

	flags = gi.argv(5);

	if (strlen(flags) > 5)
	{
		gi.cprintf(NULL, PRINT_HIGH, "Too many server flags.\n");
		return;
	}

	for (i=0; i<strlen(flags); i++)
	{
		flags[i] = LOWER(flags[i]);
		if (flags[i] != 'a' && flags[i] != 'b' && flags[i] != 's' && flags[i] != 'm' && flags[i] != 't')
		{
			gi.cprintf(NULL, PRINT_HIGH, "Bad allowed commands flag: %c.\n", flags[i]);
			gi.cprintf(NULL, PRINT_HIGH, "You can use fallowing flags: a (all), b (ban), s (server), m (match), t (teams).\n");
			badflags = true;
			break;
		}
	}

	if (badflags)
		return;

	newAdmin = (struct admin_s*)malloc(sizeof(struct admin_s));
	memset(newAdmin, 0, sizeof(struct admin_s));

	for (i=0; i<strlen(flags); i++)
	{
		if (flags[i] == 'a')
		{
			newAdmin->flags |= AD_ALL;
			break;
		}
		if (flags[i] == 'b')
			newAdmin->flags |= AD_BAN;
		if (flags[i] == 's')
			newAdmin->flags |= AD_SERVER;
		if (flags[i] == 'm')
			newAdmin->flags |= AD_MATCH;
		if (flags[i] == 't')
			newAdmin->flags |= AD_TEAMS;
	}

	strcpy(newAdmin->nick, name);
	strcpy(newAdmin->password, password);
	newAdmin->time = atoi(time);
	LINK(newAdmin, game.admin_first, game.admin_last, next, prev);

	gi.cprintf(NULL, PRINT_HIGH, "New admin password added.\n");

	WriteAdminList();
}

void SVCmd_DelAdmin_f(void)
{
	char	*p;
	char	string[1024];
	int		admin_num, i=0;
	struct admin_s *toDelete;
	qboolean found = false;
	
	if (gi.argc() < 3)
	{
		SVCmd_AdminList_f();
		gi.cprintf(NULL, PRINT_HIGH, "USAGE: deladmin <admin_num>.\n");
		return;
	}
	

	p = gi.argv(2);
	if (strlen(p) > 3)
	{
		SVCmd_AdminList_f();
		gi.cprintf(NULL, PRINT_HIGH, "USAGE: deladmin <admin_num>.\n");
		return;
	}

	string[0] = '\0';
	admin_num = atoi(p);
	sprintf(string, "%d", admin_num);

	if (strcmp(string, p))
	{
		SVCmd_AdminList_f();
		gi.cprintf(NULL, PRINT_HIGH, "USAGE: deladmin <admin_num>.\n");
		return;
	}

	for (toDelete = game.admin_first; toDelete; toDelete = toDelete->next)
	{
		if (i == admin_num)
		{
			found = true;
			break;
		}
		i++;
	}

	if (!found)
	{
		gi.cprintf(NULL, PRINT_HIGH, "Admin not found.\n");
		return;
	}

	if (toDelete->time == 0)
	{
		gi.cprintf(NULL, PRINT_HIGH, "This admin is already deleted.\n");
		return;
	}

	DeleteAdmin(toDelete);
	gi.cprintf(NULL, PRINT_HIGH, "Admin password deleted.\n");
	WriteAdminList();
}

/*
** 4.B - Update
*/

#if defined __linux__ && defined UDSYSTEM
void SVCmd_Update_f(void)
{
	cvar_t	*basedir;
	cvar_t	*gamedir;
	char	filename[256];

	if ( !strlen( ud_filename->string ) || !strlen( ud_address->string ) )
	{
		gi.cprintf( NULL, PRINT_HIGH, "You must first set ud_filename and ud_address.\n" );
		return;
	}

	basedir = gi.cvar( "basedir", "", 0 );
	gamedir = gi.cvar( "gamedir", "", 0 );
	strcpy( filename, basedir->string );
	if( strlen( gamedir->string ) )
	{
		strcat( filename, "/" );
		strcat( filename, gamedir->string );
		strcat( filename, "/" );
		strcat( filename, ud_filename->string );
	}
	else
	{
		strcat( filename, "/baseq2/" );
		strcat( filename, ud_filename->string );
	}
	DownloadCurrentVersion( filename );
}
#endif

/*
** 5.A - Timer/Score
*/

void TimerThink (edict_t *ent)
{
	int mins, secs, i;
	char	text[32];
	char	num[2];
	int		number;

	if (level.match_state == MATCH || level.match_state == OVERTIME || level.match_state == COUNTDOWN)
	{
		mins = (level.countdown_framenum - level.framenum)/600;
		secs = ((level.countdown_framenum - level.framenum)-(mins*600))/10;
		if (level.match_state != COUNTDOWN)
			Com_sprintf(text, sizeof(text), "%2d%02d", mins, secs);
		else
			Com_sprintf(text, sizeof(text), "%02d%02d", secs, secs);
	}
	else if (level.match_state == WARMUP || level.match_state == PREGAME)
		Com_sprintf(text, sizeof(text), "%2d00", (int)timelimit->value);
	else
		Com_sprintf(text, sizeof(text), "----");

	text[4] = '\0';
	for (i=0; i<4; i++)
	{
		if (text[i] == ' ')
			number = 11;
		else if (text[i] == '-')
			number = 10;
		else
		{
			Com_sprintf (num, sizeof(num), "%c", text[i]);
			number = atoi(num);
		}
		if (i==0)
			ent->s.skinnum = number;
		else if (i == 1)
			ent->slave1->s.skinnum = number;
		else if (i == 2)
			ent->slave2->s.skinnum = number;
		else if (i == 3)
			ent->slave3->s.skinnum = number;
	}
	ent->nextthink = level.time + GAMESECONDS(FRAMETIME);
	ent->think = TimerThink;
}

void SP_TimerNum (edict_t *self, int i)
{
	edict_t *ent;

	ent = G_Spawn();
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.frame = 0;
	ent->s.skinnum = 10;
	ent->s.modelindex = gi.modelindex ("models/objects/scoreboard/num.md2");
	ent->owner = self;
	if (i == 0)
	{
		VectorCopy(self->s.origin, ent->s.origin);
		ent->nextthink = level.time + GAMESECONDS(FRAMETIME);
		ent->think = TimerThink;
	}
	else
	{
		VectorCopy(self->owner->s.origin, ent->s.origin);
		if (i == 1)
			self->slave1 = ent;
		if (i == 2)
			self->slave2 = ent;
		if (i == 3)
			self->slave3 = ent;
	}
	ent->s.origin[0] -= 10.0;
	switch (i)
	{
	case 0:
		ent->s.origin[1] += 27.5;
	break;
	case 1:
		ent->s.origin[1] += 12.5;
	break;
	case 2:
		ent->s.origin[1] -= 12.5;
	break;
	case 3:
		ent->s.origin[1] -= 27.5;
	break;
	}
	ent->s.angles[1] = self->s.angles[1];

	gi.linkentity (ent);

	if (i == 0)
	{
		for(i=1; i<4; i++)
			SP_TimerNum (ent, i);
	}
}

void FragerThink (edict_t *ent)
{
	int i, number;
	char	text[32];
	char	num[2];

	if (level.match_state == MATCH || level.match_state == COUNTDOWN || level.match_state == OVERTIME || level.match_state == SUDDENDEATH || level.match_state == END)
	{
			if (level.teamB_score < 0)
				Com_sprintf(text, sizeof(text), "%3d%d ", level.teamA_score, level.teamB_score);
			else if (level.teamB_score < 10)
				Com_sprintf(text, sizeof(text), "%3d%d  ", level.teamA_score, level.teamB_score);
			else if (level.teamB_score >= 10 && level.teamB_score < 100)
				Com_sprintf(text, sizeof(text), "%3d%d ", level.teamA_score, level.teamB_score);
			else if (level.teamB_score >= 100)
				Com_sprintf(text, sizeof(text), "%3d%d", level.teamA_score, level.teamB_score);
	}
	else if (level.match_state == WARMUP || level.match_state == PREGAME)
	{
		if (game.is_old_score)
		{
			if (old_scores.team_a_score < 0)
				Com_sprintf(text, sizeof(text), "%3d%d ", old_scores.team_a_score, old_scores.team_b_score);
			else if (old_scores.team_b_score < 10)
				Com_sprintf(text, sizeof(text), "%3d%d  ", old_scores.team_a_score, old_scores.team_b_score);
			else if (old_scores.team_b_score >= 10 && old_scores.team_b_score < 100)
				Com_sprintf(text, sizeof(text), "%3d%d ", old_scores.team_a_score, old_scores.team_b_score);
			else if (old_scores.team_b_score >= 100)
				Com_sprintf(text, sizeof(text), "%3d%d", old_scores.team_a_score, old_scores.team_b_score);
		}
		else
		{
			ent->style++;
			if (ent->style >= 20)
				ent->style = 0;
			if (ent->style >= 0 && ent->style < 5)
				Com_sprintf(text, sizeof(text), "-    -");
			else if (ent->style >= 5 && ent->style < 10)
				Com_sprintf(text, sizeof(text), " -  - ");
			else if (ent->style >= 10 && ent->style < 15)
				Com_sprintf(text, sizeof(text), "  --  ");
			else if (ent->style >= 15)
				Com_sprintf(text, sizeof(text), " -  - ");
		}
	}
	else
		Com_sprintf(text, sizeof(text), "------");

	text[6] = '\0';
	for (i=0; i<6; i++)
	{
		if (text[i] == ' ')
			number = 11;
		else if (text[i] == '-')
			number = 10;
		else
		{
			Com_sprintf (num, sizeof(num), "%c", text[i]);
			number = atoi(num);
		}
		if (i==0)
			ent->s.skinnum = number;
		else if (i == 1)
			ent->slave1->s.skinnum = number;
		else if (i == 2)
			ent->slave2->s.skinnum = number;
		else if (i == 3)
			ent->slave3->s.skinnum = number;
		else if (i == 4)
			ent->slave4->s.skinnum = number;
		else if (i == 5)
			ent->slave5->s.skinnum = number;
	}

	ent->nextthink = level.time + GAMESECONDS(FRAMETIME);
	ent->think = FragerThink;
}

void SP_FragerNum (edict_t *self, int i)
{
	edict_t *ent;

	ent = G_Spawn();
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.frame = 0;
	ent->s.skinnum = 10;
	ent->s.modelindex = gi.modelindex ("models/objects/scoreboard/num.md2");
	ent->owner = self;
	if (i == 0)
	{
		VectorCopy(self->s.origin, ent->s.origin);
		ent->nextthink = level.time + GAMESECONDS(FRAMETIME);
		ent->think = FragerThink;
	}
	else
	{
		VectorCopy(self->owner->s.origin, ent->s.origin);
		if (i == 1)
			self->slave1 = ent;
		if (i == 2)
			self->slave2 = ent;
		if (i == 3)
			self->slave3 = ent;
		if (i == 4)
			self->slave4 = ent;
		if (i == 5)
			self->slave5 = ent;
	}
	ent->s.origin[2] -= 30.0;
	ent->s.origin[0] -= 10.0;
	switch (i)
	{
	case 0:
		ent->s.origin[1] += 42.5;
	break;
	case 1:
		ent->s.origin[1] += 27.5;
	break;
	case 2:
		ent->s.origin[1] += 12.5;
	break;
	case 3:
		ent->s.origin[1] -= 12.5;
	break;
	case 4:
		ent->s.origin[1] -= 27.5;
	break;
	case 5:
		ent->s.origin[1] -= 42.5;
	break;
	}
	ent->s.angles[1] = self->s.angles[1];
	gi.linkentity (ent);

	if (i == 0)
	{
		for(i=1; i<6; i++)
			SP_FragerNum (ent, i);
	}
}

void StatStringThink(edict_t *ent)
{
	if (level.match_state == WARMUP || level.match_state == PREGAME)
	{
		ent->count++;
		if (ent->count >= 140)
			ent->count = 0;

		if (ent->count >= 0 && ent->count < 20)
		{
			if (level.match_state == WARMUP)
				ent->s.skinnum = 0;
			else
				ent->s.skinnum = 1;
			if (ent->slave1->s.origin[0] != ent->s.origin[0] + 1)
				ent->slave1->s.origin[0] += 2;
		}
		else if (ent->count >= 20 && ent->count < 40)
		{
			ent->s.skinnum = 5;
			if (ent->slave1->s.origin[0] != ent->s.origin[0] + 1)
				ent->slave1->s.origin[0] += 2;
		}
		else if (ent->count >= 40 && ent->count < 60)
		{
			ent->s.skinnum = 10;
			if (ent->slave1->s.origin[0] == ent->s.origin[0] + 1)
				ent->slave1->s.origin[0] -= 2;
			ent->slave1->s.skinnum = level.notready;
		}
		else if (ent->count >= 60 && ent->count < 80)
		{
			ent->s.skinnum = 6;
			if (ent->slave1->s.origin[0] == ent->s.origin[0] + 1)
				ent->slave1->s.origin[0] -= 2;
			ent->slave1->s.skinnum = (int)allow_bfg->value;
		}
		else if (ent->count >= 80 && ent->count < 100)
		{
			ent->s.skinnum = 7;
			if (ent->slave1->s.origin[0] == ent->s.origin[0] + 1)
				ent->slave1->s.origin[0] -= 2;
			ent->slave1->s.skinnum = (int)allow_powerups->value;
		}
		else if (ent->count >= 100 && ent->count < 120)
		{
			ent->s.skinnum = 8;
			if (ent->slave1->s.origin[0] == ent->s.origin[0] + 1)
				ent->slave1->s.origin[0] -= 2;
			ent->slave1->s.skinnum = (int)allow_gibs->value;
		}
		else if (ent->count >= 120 && ent->count < 140)
		{
			ent->s.skinnum = 9;
			if (ent->slave1->s.origin[0] == ent->s.origin[0] + 1)
				ent->slave1->s.origin[0] -= 2;
			ent->slave1->s.skinnum = (int)fastweapons->value;
		}
	}
	else if (level.match_state == COUNTDOWN)
	{
		if (ent->slave1->s.origin[0] != ent->s.origin[0] + 1)
			ent->slave1->s.origin[0] += 2;
		ent->s.skinnum = 11;
	}
	else if (level.match_state == MATCH)
	{
		if (ent->slave1->s.origin[0] != ent->s.origin[0] + 1)
			ent->slave1->s.origin[0] += 2;
		ent->s.skinnum = 2;
	}
	else if (level.match_state == OVERTIME)
	{
		if (ent->slave1->s.origin[0] != ent->s.origin[0] + 1)
			ent->slave1->s.origin[0] += 2;
		ent->s.skinnum = 3;
	}
	else if (level.match_state == SUDDENDEATH)
	{
		if (ent->slave1->s.origin[0] != ent->s.origin[0] + 1)
			ent->slave1->s.origin[0] += 2;
		ent->s.skinnum = 4;
	}
	else
	{
		if (ent->slave1->s.origin[0] != ent->s.origin[0] + 1)
			ent->slave1->s.origin[0] += 2;
		ent->s.skinnum = 0;
	}
	ent->nextthink = level.time + GAMESECONDS(FRAMETIME);
	ent->think = StatStringThink;
}

void SP_StringNum(edict_t *self)
{
	edict_t *ent;

	ent = G_Spawn();
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.frame = 0;
	ent->s.skinnum = 11;
	ent->s.modelindex = gi.modelindex ("models/objects/scoreboard/num.md2");
	ent->owner = self;
	self->slave1 = ent;
	VectorCopy(self->s.origin, ent->s.origin);
	ent->s.origin[1] -= 52.0;
//	ent->s.origin[2] += 30.0;
	ent->s.origin[0] -= 1.0;
	ent->s.angles[1] = self->s.angles[1];

	gi.linkentity (ent);
}

void SP_StatString (edict_t *self)
{
	edict_t *ent;

	ent = G_Spawn();
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.frame = 0;
	ent->s.skinnum = 0;
	ent->s.modelindex = gi.modelindex ("models/objects/scoreboard/string.md2");
	ent->owner = self;
	VectorCopy(self->s.origin, ent->s.origin);
	ent->nextthink = level.time + GAMESECONDS(FRAMETIME);
	ent->think = StatStringThink;
//	ent->s.origin[1] += 42.5;
	ent->s.origin[2] += 30.0;
	ent->s.origin[0] -= 10.0;
	ent->s.angles[1] = self->s.angles[1];
	gi.linkentity (ent);

	SP_StringNum(ent);
}

void SP_scoreboard (int x, int y, int z, int dir)
{
	edict_t *ent;

	ent = G_Spawn();
	ent->s.origin[0] = x;
	ent->s.origin[1] = y;
	ent->s.origin[2] = z;
	ent->s.angles[1] = dir+90;
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex ("models/objects/scoreboard/board.md2");
	ent->s.frame = 0;
	ent->classname = "scoreboard";

	gi.linkentity (ent);

	SP_TimerNum(ent, 0);

	SP_FragerNum(ent, 0);

	SP_StatString(ent);
}

void SpawnScoreBoards(void)
{
	cvar_t	*basedir, *gamedir;
	char	filename[256];
	FILE	*f=NULL;
	int		num=0;
	int		x,y,z,dir;

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);

	strcpy(filename,basedir->string);
	if(strlen(gamedir->string))
	{
		strcat(filename,"/");
		strcat(filename,gamedir->string);
		strcat(filename,"/scoreboard");
		strcat(filename,port->string);
		strcat(filename,".lst");
		gi.dprintf("Spawning scoreboards from %s...", filename);
		f = fopen(filename,"r");
		if (!f)
		    gi.dprintf("file not found!\nTrying to load from ");
	}

	if (!f || !strlen(gamedir->string))
	{
		strcpy(filename,basedir->string);
		strcat(filename,"/baseq2/scoreboard");
		strcat(filename,port->string);
		strcat(filename,".lst");
		gi.dprintf("%s...", filename);
		f = fopen(filename,"r");
		if (!f)
		{
		    gi.dprintf("file not found!\n");
		    return;
		}
	}

	
	if (f)
	{
		char	*line;
		do
		{
			line = read_line(f);
			if (!line)
				break;
			if (!strcmp(level.mapname, read_word(line, 0)))
			{
				num++;
				x = atoi(read_word(line, 1));
				y = atoi(read_word(line, 2));
				z = atoi(read_word(line, 3));
				dir = atoi(read_word(line, 4));
				SP_scoreboard(x, y, z, dir);
			}
			else
				continue;
		} while (!feof(f));
		fclose(f);
	}
	gi.dprintf("Done (%d scoreboard%s spawned)\n", num, num > 1 ? "s" : "");
}

/*
** 6.A DominationRuneThink
*/

void DominationRuneThink(edict_t *self)
{
	if (level.match_state == WARMUP || level.match_state == PREGAME || level.match_state == COUNTDOWN || level.match_state == END)
	{
		self->nextthink = 0;
		self->think = NULL;
		return;
	}

	if ((self->style == 0 && level.rune1_owner == TEAM_A) || (self->style == 1 && level.rune2_owner == TEAM_A) || (self->style == 2 && level.rune3_owner == TEAM_A))
	{
		level.teamA_score++;
		self->nextthink = level.time + GAMESECONDS(5);
		self->think = DominationRuneThink;
		if (self->owner)
		{
			if (self->owner->client && self->owner->inuse && self->owner->client->pers.save_data.team == TEAM_A)
				self->owner->client->resp.score++;
		}
	}
	else if ((self->style == 0 && level.rune1_owner == TEAM_B) || (self->style == 1 && level.rune2_owner == TEAM_B) || (self->style == 2 && level.rune3_owner == TEAM_B))
	{
		level.teamB_score++;
		self->nextthink = level.time + GAMESECONDS(5);
		self->think = DominationRuneThink;
		if (self->owner)
		{
			if (self->owner->client && self->owner->inuse && self->owner->client->pers.save_data.team == TEAM_B)
				self->owner->client->resp.score++;
		}
	}
}

/*
** 6.B DominationRuneTouch
*/

void DominationRuneTouch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	qboolean	touched=false;

	if (level.match_state == WARMUP || level.match_state == PREGAME || level.match_state == COUNTDOWN)
		return;
	if (!other->client)
		return;
	if (other->client->pers.save_data.team == TEAM_NONE)
		return;

	if (self->style == 0)
	{
		if (level.rune1_owner == TEAM_NONE || level.rune1_owner != other->client->pers.save_data.team)
		{
			touched = true;
			level.rune1_owner = other->client->pers.save_data.team;
			self->owner = other;
			if (level.rune1_owner != other->client->pers.save_data.team)
				self->nextthink = level.time + GAMESECONDS(8);
			else
				self->nextthink = level.time + GAMESECONDS(5);

			self->think = DominationRuneThink;
		}
	}
	else if (self->style == 1)
	{
		if (level.rune2_owner == TEAM_NONE || level.rune2_owner != other->client->pers.save_data.team)
		{
			touched = true;
			level.rune2_owner = other->client->pers.save_data.team;
			self->owner = other;
			if (level.rune2_owner != other->client->pers.save_data.team)
				self->nextthink = level.time + GAMESECONDS(8);
			else
				self->nextthink = level.time + GAMESECONDS(5);

			self->think = DominationRuneThink;
		}
	}
	else if (self->style == 2)
	{
		if (level.rune3_owner == TEAM_NONE || level.rune3_owner != other->client->pers.save_data.team)
		{
			touched = true;
			level.rune3_owner = other->client->pers.save_data.team;
			self->owner = other;
			if (level.rune3_owner != other->client->pers.save_data.team)
				self->nextthink = level.time + GAMESECONDS(8);
			else
				self->nextthink = level.time + GAMESECONDS(5);

			self->think = DominationRuneThink;
		}
	}
	if (touched)
	{
		if (self->owner->client->pers.save_data.team == TEAM_A)
		{
			gi.setmodel (self, "models/objects/domination/red.md2");
			self->s.effects |= EF_FLAG1;
			if (self->s.effects & EF_FLAG2)
				self->s.effects &= ~EF_FLAG2;
		}
		else if (self->owner->client->pers.save_data.team == TEAM_B)
		{
			gi.setmodel (self, "models/objects/domination/blue.md2");
			if (self->s.effects & EF_FLAG1)
				self->s.effects &= ~EF_FLAG1;
			self->s.effects |= EF_FLAG2;
		}
	}
}

void ClearDominationRunes(void)
{
	if (!domination->value)
		return;

	if (level.rune1)
	{
		if (level.rune1->s.effects & EF_FLAG1)
			level.rune1->s.effects &= ~EF_FLAG1;
		if (level.rune1->s.effects & EF_FLAG2)
			level.rune1->s.effects &= ~EF_FLAG2;
		gi.setmodel (level.rune1, "models/objects/domination/neutral.md2");
		level.rune1_owner = TEAM_NONE;
		level.rune1->nextthink = 0;
		level.rune1->think = NULL;
		level.rune1->owner = NULL;
	}
	if (level.rune2)
	{
		if (level.rune2->s.effects & EF_FLAG1)
			level.rune2->s.effects &= ~EF_FLAG1;
		if (level.rune2->s.effects & EF_FLAG2)
			level.rune2->s.effects &= ~EF_FLAG2;
		gi.setmodel (level.rune2, "models/objects/domination/neutral.md2");
		level.rune2_owner = TEAM_NONE;
		level.rune2->nextthink = 0;
		level.rune2->think = NULL;
		level.rune2->owner = NULL;
	}
	if (level.rune3)
	{
		if (level.rune3->s.effects & EF_FLAG1)
			level.rune3->s.effects &= ~EF_FLAG1;
		if (level.rune3->s.effects & EF_FLAG2)
			level.rune3->s.effects &= ~EF_FLAG2;
		gi.setmodel (level.rune3, "models/objects/domination/neutral.md2");
		level.rune3_owner = TEAM_NONE;
		level.rune3->nextthink = 0;
		level.rune3->think = NULL;
		level.rune3->owner = NULL;
	}
}

/*
** 6.C SP_domination_rune
*/
void SP_domination_rune(edict_t *ent)
{
	if (!domination->value)
		return;

	if (ent->style == 0)
		level.rune1 = ent;
	else if (ent->style == 1)
		level.rune2 = ent;
	else if (ent->style == 2)
		level.rune3 = ent;
	gi.setmodel (ent, "models/objects/domination/neutral.md2");
	ent->s.skinnum = 0;
	ent->touch = DominationRuneTouch;
	ent->owner = NULL;
	ent->s.effects |= EF_ROTATE;
	ent->s.renderfx |= RF_FULLBRIGHT;
	ent->solid = SOLID_TRIGGER;
	VectorSet (ent->mins, -25, -25, -32);
	VectorSet (ent->maxs, 25, 25, 32);
	gi.linkentity (ent);
}

/*
** 6.D SetDominationStatusbar
*/

void SetDominationStatusbar(void)
{
	char	statusbar[1024];
	char	entry[1024];

	Com_sprintf(statusbar, sizeof(statusbar), "yb -24 xv 0 hnum xv 50 picn i_health "); //health
	strcat(statusbar, "if 3 xv 100 anum xv 150 pic 0 endif "); //ammo
	strcat(statusbar, "if 5 xv 200 rnum xv 250 pic 2 endif "); //armor
	strcat(statusbar, "if 4 xv  296 pic 4 endif "); //selected item
	strcat(statusbar, "yb -50 if 9 xv 148 pic 9 endif "); //help / weapon icon
	strcat(statusbar, "xv 200 yb -56 stat_string 31 "); //match state
	strcat(statusbar, "if 14 xr -50 yt 2 num 3 14 endif "); //frags
	strcat(statusbar, "if 30 xl 8 yv 144 stat_string 30 endif "); //vote
	strcat(statusbar, "yb -102 xr -26 picn i_redteam xr -78 num 3 17 "); // red team
	strcat(statusbar, "if 16 yb -104 xr -28 pic 16 endif "); //joined overlay
	strcat(statusbar, "yb -75 xr -26 picn i_blueteam xr -78 num 3 19 "); // blue team
	strcat(statusbar, "if 18 yb -77 xr -28 pic 18 endif "); //joined overlay
	if (level.rune3_name)
	{
		Com_sprintf(entry, sizeof(entry), "yb -136 xr -%d string \"%s\" ", strlen(level.rune3_name)*8, level.rune3_name);
		strcat(statusbar, entry); // rune3 string
	}
	else
		strcat(statusbar, "yb -136 xr -40 string \"RUNE3\" "); // rune3 string
	strcat(statusbar, "yb -170 xr -34 pic 22 "); // rune3 icon
	if (level.rune2_name)
	{
		Com_sprintf(entry, sizeof(entry), "yb -186 xr -%d string \"%s\" ", strlen(level.rune2_name)*8, level.rune2_name);
		strcat(statusbar, entry); // rune3 string
	}
	else
		strcat(statusbar, "yb -186 xr -40 string \"RUNE2\" "); // rune2 string
	strcat(statusbar, "yb -220 xr -34 pic 21 "); // rune2 icon
	if (level.rune1_name)
	{
		Com_sprintf(entry, sizeof(entry), "yb -236 xr -%d string \"%s\" ", strlen(level.rune1_name)*8, level.rune1_name);
		strcat(statusbar, entry); // rune3 string
	}
	else
		strcat(statusbar, "yb -236 xr -40 string \"RUNE1\" "); // rune2 string
	strcat(statusbar, "yb -270 xr -34 pic 20 "); // rune2 icon
	strcat(statusbar, "if 7 yb -50 xv 246 num 2 8 xv 296 pic 7 endif ");// timer

	gi.configstring(CS_STATUSBAR, statusbar);
}
