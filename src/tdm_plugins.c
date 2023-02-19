#include "g_local.h"
#include "tdm_plugins_internal.h"

#ifdef __linux__
#include <sys/types.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/select.h>
#else
#include <windows.h>
#include <winsock2.h>
#include <time.h>
#endif

#include <pthread.h>

/*
	Hash Tables
*/

pthread_mutex_t syncFunctionsMutex = PTHREAD_MUTEX_INITIALIZER;

struct tdmFuncsHashTableNode tdmFuncsHashTable[256];
struct tdmEventsHashTableNode tdmEventsHashTable[256];

unsigned char Rand8Unique[256]=
{
	237,	47,		222,	160,	91,		211,	94,		26,
	64,		106,	6,		23,		165,	104,	204,	202,
	193,	178,	76,		194,	86,		132,	121,	219,
	149,	55,		145,	183,	216,	39,		212,	242,
	93,		59,		239,	156,	252,	100,	199,	60,
	174,	37,		95,		207,	24,		159,	89,		84,
	153,	80,		137,	230,	135,	233,	35,		71,
	148,	236,	27,		225,	18,		117,	155,	166,
	190,	191,	138,	73,		226,	5,		52,		213,
	30,		144,	136,	44,		74,		58,		4,		110,
	244,	254,	220,	147,	130,	188,	87,		56,
	67,		203,	41,		140,	205,	54,		126,	61,
	19,		221,	107,	105,	82,		16,		101,	154,
	122,	53,		185,	8,		68,		196,	151,	14,
	170,	28,		163,	180,	186,	120,	123,	176,
	33,		179,	175,	209,	128,	17,		119,	98,
	129,	2,		218,	208,	112,	192,	113,	177,
	97,		36,		247,	255,	195,	46,		173,	42,
	184,	133,	92,		167,	206,	253,	243,	45,
	161,	11,		49,		172,	240,	108,	109,	78,
	228,	214,	181,	85,		32,		79,		168,	141,
	116,	158,	198,	249,	238,	114,	43,		232,
	72,		217,	150,	127,	103,	142,	51,		90,
	210,	7,		38,		102,	246,	21,		152,	10,
	34,		70,		62,		9,		171,	13,		169,	66,
	131,	134,	65,		164,	115,	234,	245,	227,
	143,	63,		0,		111,	31,		22,		229,	157,
	83,		215,	118,	182,	48,		15,		235,	124,
	224,	88,		25,		20,		162,	146,	250,	231,
	50,		125,	69,		189,	3,		139,	251,	81,
	201,	187,	75,		96,		99,		1,		77,		29,
	223,	248,	197,	57,		200,	241,	40,		12
};

unsigned char TdmHash(char *str)
{
    unsigned char h = 0;
    while (*str) h = Rand8Unique[h ^ *str++];
    return h;
}

void InitHashTables(void)
{
	int i;
	for (i=0; i<256; i++)
	{
		tdmFuncsHashTable[i].first = NULL;
		tdmFuncsHashTable[i].last = NULL;
		tdmEventsHashTable[i].first = NULL;
		tdmEventsHashTable[i].last = NULL;
	}
}

void FreeHashTables(void)
{
	int i;
	for (i=0; i<256; i++)
	{
		if (tdmFuncsHashTable[i].first)
		{
			struct tdmPluginsFunction *someFunction;
			someFunction = tdmFuncsHashTable[i].first;

			while(someFunction)
			{
				UNLINK(someFunction, tdmFuncsHashTable[i].first, tdmFuncsHashTable[i].last, next, prev);
				free(someFunction);
				someFunction = NULL;
				someFunction = tdmFuncsHashTable[i].first;
			}
		}

		if (tdmEventsHashTable[i].first)
		{
			struct tdmPluginsEvent *someEvent;
			someEvent = tdmEventsHashTable[i].first;

			while(someEvent)
			{
				if (someEvent->firstCallback)
				{
					struct tdmEventCallback *someCallback;
					someCallback = someEvent->firstCallback;

					while(someCallback)
					{
						UNLINK(someCallback, someEvent->firstCallback, someEvent->lastCallback, next, prev);
						free(someCallback);
						someCallback = NULL;
						someCallback = someEvent->firstCallback;
					}
				}
				UNLINK(someEvent, tdmEventsHashTable[i].first, tdmEventsHashTable[i].last, next, prev);
				free(someEvent);
				someEvent = NULL;
				someEvent = tdmEventsHashTable[i].first;
			}
		}
	}
}

/*
	Plugins Function and Events
*/

qboolean AddFunction(char *name, pluginFunction toAddFunction)
{
	unsigned char tableKey;
	struct tdmPluginsFunction *newFunction;

	if (!name || !toAddFunction)
		return false;

	if (strlen(name) > MAX_FUNC_NAME_LEN)
		return false;

	tableKey = TdmHash(name);

	for(newFunction = tdmFuncsHashTable[tableKey].first; newFunction; newFunction = newFunction->next)
	{
		if (!strcmp(newFunction->funcName, name)) //already exists
			return false;
	}

	newFunction = (struct tdmPluginsFunction *)malloc(sizeof(struct tdmPluginsFunction));
	if (!newFunction)
		return false;

	memset(newFunction, 0, sizeof(struct tdmPluginsFunction));
	strncpy(newFunction->funcName, name, MAX_FUNC_NAME_LEN);
	newFunction->function = toAddFunction;
	LINK(newFunction, tdmFuncsHashTable[tableKey].first, tdmFuncsHashTable[tableKey].last, next, prev);
	return true;
}

qboolean DelFunction(char *name)
{
	unsigned char tableKey;
	struct tdmPluginsFunction *whatFunction;
	qboolean found = false;

	if (!name)
		return false;

	if (strlen(name) > MAX_FUNC_NAME_LEN)
		return false;

	tableKey = TdmHash(name);

	for(whatFunction = tdmFuncsHashTable[tableKey].first; whatFunction; whatFunction = whatFunction->next)
	{
		if (!strcmp(whatFunction->funcName, name)) //function found
		{
			found = true;
			break;
		}
	}
	if (!found)
		return false;

	UNLINK(whatFunction, tdmFuncsHashTable[tableKey].first, tdmFuncsHashTable[tableKey].last, next, prev);
	free(whatFunction);
	whatFunction = NULL;
	return true;
}

long CallFunction(char *name, unsigned int wParam, long lParam)
{
	unsigned char tableKey;
	struct tdmPluginsFunction *whatFunction;
	qboolean found = false;

	if (!name)
		return false;

	if (strlen(name) > MAX_FUNC_NAME_LEN)
		return false;

	tableKey = TdmHash(name);

	for(whatFunction = tdmFuncsHashTable[tableKey].first; whatFunction; whatFunction = whatFunction->next)
	{
		if (!strcmp(whatFunction->funcName, name)) //function found
		{
			found = true;
			break;
		}
	}

	if (!found)
		return -1;

	return whatFunction->function(wParam, lParam);
}

long CallFunctionSync(char *name, unsigned int wParam, long lParam)
{
	unsigned char tableKey;
	struct tdmPluginsFunction *whatFunction;
	qboolean found = false;
	long ret;

	if (!name)
		return false;

	if (strlen(name) > MAX_FUNC_NAME_LEN)
		return false;

	tableKey = TdmHash(name);

	for(whatFunction = tdmFuncsHashTable[tableKey].first; whatFunction; whatFunction = whatFunction->next)
	{
		if (!strcmp(whatFunction->funcName, name)) //function found
		{
			found = true;
			break;
		}
	}

	if (!found)
		return -1;

	pthread_mutex_lock( &syncFunctionsMutex );
	ret = whatFunction->function(wParam, lParam);
	pthread_mutex_unlock( &syncFunctionsMutex );

	return ret;
}

void AllowSyncFunctionsCalls( void )
{
	struct timeval timeout;
	pthread_mutex_unlock( &syncFunctionsMutex );

	timeout.tv_sec = 0;
	timeout.tv_usec = 1;

	select( 0, NULL, NULL, NULL, &timeout );
}

void DisallowSyncFunctionsCalls( void )
{
	pthread_mutex_lock( &syncFunctionsMutex );
}

qboolean EventCreate(char *name)
{
	unsigned char tableKey;
	struct tdmPluginsEvent *newEvent;

	if (!name)
		return false;

	if (strlen(name) > MAX_FUNC_NAME_LEN)
		return false;

	tableKey = TdmHash(name);

	for(newEvent = tdmEventsHashTable[tableKey].first; newEvent; newEvent = newEvent->next)
	{
		if (!strcmp(newEvent->eventName, name)) //eventExists
			return false;
	}
	newEvent = (struct tdmPluginsEvent *)malloc(sizeof(struct tdmPluginsEvent));
	if (!newEvent)
		return false;

	memset(newEvent, 0, sizeof(struct tdmPluginsEvent));
	strncpy(newEvent->eventName, name, MAX_FUNC_NAME_LEN);
	LINK(newEvent, tdmEventsHashTable[tableKey].first, tdmEventsHashTable[tableKey].last, next, prev);
	return true;
}

qboolean EventAddCallback(char *name, pluginEventFunction toAddFunction)
{
	unsigned char tableKey;
	struct tdmPluginsEvent *whatEvent;
	struct tdmEventCallback *newCallback;
	qboolean found = false;

	if (!name || !toAddFunction)
		return false;

	if (strlen(name) > MAX_FUNC_NAME_LEN)
		return false;

	tableKey = TdmHash(name);

	for(whatEvent = tdmEventsHashTable[tableKey].first; whatEvent; whatEvent = whatEvent->next)
	{
		if (!strcmp(whatEvent->eventName, name)) //event exists
		{
			found = true;
			break;
		}
	}

	if (!found) //if not exist then add new one
	{
		whatEvent = (struct tdmPluginsEvent *)malloc(sizeof(struct tdmPluginsEvent));
		if (!whatEvent)
			return false;

		memset(whatEvent, 0, sizeof(struct tdmPluginsEvent));
		strncpy(whatEvent->eventName, name, MAX_FUNC_NAME_LEN);
		LINK(whatEvent, tdmEventsHashTable[tableKey].first, tdmEventsHashTable[tableKey].last, next, prev);
	}

	newCallback = (struct tdmEventCallback *)malloc(sizeof(struct tdmEventCallback));
	if (!newCallback)
		return false;

	memset(newCallback, 0, sizeof(struct tdmEventCallback));
	newCallback->eventCallbackFunction = toAddFunction;
	LINK(newCallback, whatEvent->firstCallback, whatEvent->lastCallback, next, prev);
	return true;
}

qboolean EventDelete(char *name)
{
	unsigned char tableKey;
	struct tdmPluginsEvent *whatEvent;
	struct tdmEventCallback *whatCallback;
	qboolean found = false;

	if (!name)
		return false;

	if (strlen(name) > MAX_FUNC_NAME_LEN)
		return false;

	tableKey = TdmHash(name);

	for(whatEvent = tdmEventsHashTable[tableKey].first; whatEvent; whatEvent = whatEvent->next)
	{
		if (!strcmp(whatEvent->eventName, name)) //event Exists
		{
			found = true;
			break;
		}
	}

	if (!found)
		return false;

	if (whatEvent->firstCallback)
	{
		whatCallback = whatEvent->firstCallback;
		while(whatCallback)
		{
			UNLINK(whatCallback, whatEvent->firstCallback, whatEvent->lastCallback, next, prev);
			free(whatCallback);
			whatCallback = NULL;
			whatCallback = whatEvent->firstCallback;
		}
	}

	UNLINK(whatEvent, tdmEventsHashTable[tableKey].first, tdmEventsHashTable[tableKey].last, next, prev);
	free(whatEvent);
	whatEvent = NULL;
	return true;
}

qboolean EventCall(char *name, unsigned int wParam, long lParam)
{
	unsigned char tableKey;
	struct tdmPluginsEvent *whatEvent;
	struct tdmEventCallback *whatCallback;
	qboolean found = false;

	if (!name)
		return true;

	if (strlen(name) > MAX_FUNC_NAME_LEN)
		return true;

	tableKey = TdmHash(name);

	for(whatEvent = tdmEventsHashTable[tableKey].first; whatEvent; whatEvent = whatEvent->next)
	{
		if (!strcmp(whatEvent->eventName, name)) //event Exists
		{
			found = true;
			break;
		}
	}

	if (!found)
		return true;

	for (whatCallback = whatEvent->firstCallback; whatCallback; whatCallback = whatCallback->next)
	{
		if ( whatCallback->eventCallbackFunction(wParam, lParam) == false )
			return false; /* stop event propagation */
	}

	return true;
}

/*
	Plugins
*/

struct tdmPlugins tdmPluginList;

#ifdef __linux__
void LoadPluginsLinux(char *path)
{
	DIR *searchDir;
	struct dirent *dirEnt;
	struct tdmPlugin *newPlugin;

	searchDir = opendir(path);

	if (!searchDir)
		return;

	do
	{
		dirEnt = readdir(searchDir);
		if (dirEnt)
		{
			char libPath[260];
			void *hLib;

			strncpy(libPath, path, 260);
			strncat(libPath, dirEnt->d_name, 260);
			dlerror();
			hLib = dlopen(libPath, RTLD_NOW|RTLD_LOCAL);
			if (!hLib)
			{
				gi.dprintf("%s - dlopen error: %s.\n", libPath, dlerror() );
				continue;
			}
			dlerror();
			if (dlsym(hLib, "TDM_pluginLoad"))
			{
				newPlugin = (struct tdmPlugin *)malloc(sizeof(struct tdmPlugin));
				if (!newPlugin)
				{
					dlclose(hLib);
					continue;
				}
				memset(newPlugin, 0, sizeof(struct tdmPlugin));
				newPlugin->hLib = hLib;
				newPlugin->pluginLoad = (TDM_pluginLoadType)dlsym(hLib, "TDM_pluginLoad");
				newPlugin->pluginUnload = (TDM_pluginUnloadType)dlsym(hLib, "TDM_pluginUnload");
				LINK(newPlugin, tdmPluginList.fist, tdmPluginList.last, next, prev);
				//gi.dprintf("%s - loaded.\n", libPath);
			}
			else
			{
				gi.dprintf("%s - dlsym error: %s\n", libPath, dlerror() );
				dlclose(hLib);
			}
		}
	} while(dirEnt);

	closedir(searchDir);
}
#else
void LoadPluginWin32(char *fileName)
{
	struct tdmPlugin *newPlugin;
	void *hLib;
	
	hLib = (void*)LoadLibrary(fileName);

	if (!hLib)
		return;

	if (GetProcAddress((HMODULE)hLib, "TDM_pluginLoad"))
	{
		newPlugin = (struct tdmPlugin *)malloc(sizeof(struct tdmPlugin));
		if (!newPlugin)
		{
			FreeLibrary((HMODULE)hLib);
			return;
		}
		memset(newPlugin, 0, sizeof(struct tdmPlugin));
		newPlugin->hLib = hLib;
		newPlugin->pluginLoad = (TDM_pluginLoadType)GetProcAddress((HMODULE)hLib, "TDM_pluginLoad");
		newPlugin->pluginUnload = (TDM_pluginUnloadType)GetProcAddress((HMODULE)hLib, "TDM_pluginUnload");
		LINK(newPlugin, tdmPluginList.fist, tdmPluginList.last, next, prev);
		//gi.dprintf("%s - loaded.\n", fileName);
	}
	else
		FreeLibrary((HMODULE)hLib);
}

void LoadPluginsWin32(char *path)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = NULL;
	char searchPath[260];
	char fileName[260];

	strncpy(searchPath, path, 260);
	strncat(searchPath, "*.dll", 260);

	hFind = FindFirstFile(searchPath, &FindFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		Com_sprintf(fileName, sizeof(fileName), "%s%s", path, FindFileData.cFileName);
		LoadPluginWin32(fileName);
		while (FindNextFile(hFind, &FindFileData))
		{		
			Com_sprintf(fileName, sizeof(fileName), "%s%s", path, FindFileData.cFileName);
			LoadPluginWin32(fileName);
		}
		FindClose(hFind);
	}
	else //empty pluings dir
	{
	}
}
#endif

void LoadPlugins(void)
{
	struct sPluginFunctions plugFunctions;
	cvar_t *basedir, *gamedir;
	char pluginsDir[260];
	struct tdmPlugin *loadPlugin;

	AddFunction( FUNC_INFOSETVALUEFORKEY, quake2InfoSetValueForKey );
	AddFunction( FUNC_INFOVALUEFORKEY, quake2InfoValueForKey );

	pthread_mutex_lock( &syncFunctionsMutex );

	tdmPluginList.fist = NULL;
	tdmPluginList.last = NULL;

	memset(&plugFunctions, 0, sizeof(struct sPluginFunctions));
	plugFunctions.AddFunction = AddFunction;
	plugFunctions.DelFunction = DelFunction;
	plugFunctions.CallFunction = CallFunction;
	plugFunctions.CallFunctionSync = CallFunctionSync;
	plugFunctions.EventCreate = EventCreate;
	plugFunctions.EventAddCallback = EventAddCallback;
	plugFunctions.EventDelete = EventDelete;
	plugFunctions.EventCall = EventCall;
	plugFunctions.gi = &gi;
	plugFunctions.edicts = g_edicts;
	plugFunctions.level = &level;
	plugFunctions.game = &game;

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);

	strcpy(pluginsDir, basedir->string);

	if(strlen(gamedir->string))
	{
		strcat(pluginsDir, "/");
		strcat(pluginsDir, gamedir->string);
		strcat(pluginsDir, "/plugins/");
	}
	else
		strcat(pluginsDir, "/baseq2/plugins/");

	gi.dprintf("\nLoading plugins from %s...\n", pluginsDir);

#ifdef __linux__
	LoadPluginsLinux(pluginsDir);
#else
	LoadPluginsWin32(pluginsDir);
#endif
	for (loadPlugin = tdmPluginList.fist; loadPlugin; loadPlugin = loadPlugin->next)
	{
		struct sPluginInfo info;
		memset( &info, 0, sizeof( struct sPluginInfo ) );
		loadPlugin->pluginLoad(&plugFunctions, &info);
		if ( info.pluginAPIVersion != TDM_PLUGIN_API_VERSION )
		{
			gi.dprintf( "Plugin %s has incompatible API version (%d, should be %d)\n", info.pluginName, info.pluginAPIVersion, TDM_PLUGIN_API_VERSION );
			//UnloadPlugin( loadPlugin );
			continue;
		}
		gi.dprintf( "Plugin %s loaded.\n", info.pluginName );
	}
}

void UnloadPlugin( struct tdmPlugin *plugin )
{
	plugin->pluginUnload();
#ifdef __linux__
	dlclose( plugin->hLib );
#else
	FreeLibrary((HMODULE)plugin->hLib);
#endif
	UNLINK( plugin, tdmPluginList.fist, tdmPluginList.last, next, prev );
	free( plugin );
}

void UnloadPlugins(void)
{
	struct tdmPlugin *whichPlugin;

	for (whichPlugin = tdmPluginList.fist; whichPlugin; whichPlugin = whichPlugin->next)
	{
		whichPlugin->pluginUnload();
#ifdef __linux__
		dlclose(whichPlugin->hLib);
#else
		FreeLibrary((HMODULE)whichPlugin->hLib);
#endif
	}

	whichPlugin = tdmPluginList.fist;
	while(whichPlugin)
	{
		UNLINK(whichPlugin, tdmPluginList.fist, tdmPluginList.last, next, prev);
		free(whichPlugin);
		whichPlugin = NULL;
		whichPlugin = tdmPluginList.fist;
	}

	pthread_mutex_unlock( &syncFunctionsMutex );
	pthread_mutex_destroy( &syncFunctionsMutex );
}
