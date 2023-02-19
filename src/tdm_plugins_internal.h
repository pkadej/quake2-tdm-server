#include "tdm_plugins.h"

/*
	Plugins
*/

struct tdmPlugin
{
	struct tdmPlugin *next;
	struct tdmPlugin *prev;
	void *hLib;
	TDM_pluginLoadType pluginLoad;
	TDM_pluginUnloadType pluginUnload;
};

struct tdmPlugins
{
	struct tdmPlugin *fist;
	struct tdmPlugin *last;
};

/*
	Functions
*/

struct tdmPluginsFunction
{
	struct tdmPluginsFunction *next;
	struct tdmPluginsFunction *prev;
	char funcName[MAX_FUNC_NAME_LEN];
	pluginFunction function;
};

struct tdmFuncsHashTableNode
{
	struct tdmPluginsFunction *first;
	struct tdmPluginsFunction *last;
};

/*
	Events
*/

struct tdmEventCallback
{
	struct tdmEventCallback *next;
	struct tdmEventCallback *prev;
	pluginEventFunction eventCallbackFunction;
};

struct tdmPluginsEvent
{
	struct tdmPluginsEvent *next;
	struct tdmPluginsEvent *prev;
	char eventName[MAX_FUNC_NAME_LEN];
	struct tdmEventCallback *firstCallback;
	struct tdmEventCallback *lastCallback;
};

struct tdmEventsHashTableNode
{
	struct tdmPluginsEvent *first;
	struct tdmPluginsEvent *last;
};

unsigned char TdmHash(char *str);
void InitHashTables(void);
void FreeHashTables(void);
qboolean AddFunction(char *name, pluginFunction toAddFunction);
qboolean DelFunction(char *name);
long CallFunction(char *name, unsigned int wParam, long lParam);
qboolean EventCreate(char *name);
qboolean EventAddCallback(char *name, pluginEventFunction toAddFunction);
qboolean EventDelete(char *name);
qboolean EventCall(char *name, unsigned int wParam, long lParam);
void LoadPlugins(void);
void UnloadPlugins(void);
void AllowSyncFunctionsCalls( void );
void DisallowSyncFunctionsCalls( void );

long quake2InfoSetValueForKey( unsigned int wParam, long lParam );
long quake2InfoValueForKey( unsigned int wParam, long lParam );
