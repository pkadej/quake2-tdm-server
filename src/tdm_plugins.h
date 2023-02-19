#define MAX_FUNC_NAME_LEN 128
#define TDM_PLUGIN_API_VERSION 1

typedef long (*pluginFunction) (unsigned int wParam, long lParam);
typedef qboolean (*pluginEventFunction) (unsigned int wParam, long lParam);

struct sPluginFunctions
{
	qboolean (*AddFunction)(char *name, pluginFunction toAddFunction);
	qboolean (*DelFunction)(char *name);
	long (*CallFunction)(char *name, unsigned int wParam, long lParam);
	long (*CallFunctionSync)(char *name, unsigned int wParam, long lParam);
	qboolean (*EventCreate)(char *name);
	qboolean (*EventAddCallback)(char *name, pluginEventFunction toAddFunction);
	qboolean (*EventDelete)(char *name);
	qboolean (*EventCall)(char *name, unsigned int wParam, long lParam);
	game_import_t *gi;
	struct edict_s *edicts;
	game_locals_t *game;
	level_locals_t *level;
};

struct sPluginInfo
{
	int pluginAPIVersion;
	char *pluginName;
};

struct sInfoStrings
{
	char *infoString;
	char *key;
	char *value;
};

#ifdef __linux__
	typedef void (*TDM_pluginLoadType) (struct sPluginFunctions *plugFunctions, struct sPluginInfo *pluginInfo);
	typedef void  (*TDM_pluginUnloadType) (void);
#else	
	typedef void (__cdecl *TDM_pluginLoadType) (struct sPluginFunctions *plugFunctions, struct sPluginInfo *pluginInfo);
	typedef void  (__cdecl *TDM_pluginUnloadType) (void);
#endif

#define EVENT_GRUNFRAME "G_RunFrame" //wParam = 0, lParam = 0
#define EVENT_CALLSPAWN "CallSpawn" //wParam = 0, lParam = edict_s *ent
#define	EVENT_SPAWNENTITIES "SpawnEntities" //wParam = char *mapname, lParam = char *entities
#define EVENT_CLIENTCOMMAND "ClientCommand" //wParam = char *cmd, lParam = edict_s *ent
#define EVENT_SERVERCOMMAND "ServerCommand" //wParam = char *cmd, lParam = 0
#define EVENT_CLIENTCONNECT "ClientConnect" //wParam = 0, lParam = *ent
#define EVENT_CLIENTBEGIN "ClientBegin" //wParam = 0, lParam = *ent
#define EVENT_CLIENTDISCONNECT "ClientDisconnect" //wParam = 0, lParam = *ent
#define EVENT_BPRINTF "bprintf" //wParam = int printlevel, lParam = char *message
#define EVENT_NAMECLIENTUSERINFOCHANGED "NameUserinfoChanged" //wParam = char *userinfo, lParam = *ent

#define FUNC_INFOSETVALUEFORKEY "Info_SetValueForKey" //wParam = 0, lParam = struct sInfoStrins *info
#define FUNC_INFOVALUEFORKEY "Info_ValueForKey" //wParam = 0, lParam = struct sInfoStrins *info

//#define EVENT_DPRINTF "dprintf" //wParam = 0, lParam = sPrintfStruct *params
