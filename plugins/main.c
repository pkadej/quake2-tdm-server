#include "tdm_irc.h"

#ifdef WIN32
int ircCreateThread(HANDLE *hHandle, DWORD *(*startAddres)(LPVOID), LPVOID lpParameter)
{
	DWORD threadID;
	*hHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)startAddres, lpParameter, 0, (LPDWORD)&threadID);
	if (*hHandle)
	{
		CloseHandle(*hHandle);
		return 0;
	}
	else
		return 1;
}

DWORD ircWaitForThread(HANDLE hHandle, unsigned long dwMilliseconds)
{
	return WaitForSingleObject(hHandle, (DWORD)dwMilliseconds);
}

DWORD ircWaitForMutex(HANDLE *hHandle, unsigned long dwMilliseconds)
{
	return WaitForSingleObject(*hHandle, (DWORD)dwMilliseconds);
}

void ircCreateMutex(HANDLE *hMutex)
{
	*hMutex = CreateMutex(NULL, FALSE, NULL);
}

void ircReleaseMutex(HANDLE *hMutex)
{
	ReleaseMutex(*hMutex);
}

void ircDestroyMutex(HANDLE *hMutex)
{
	CloseHandle(*hMutex);
}

void ircExitThread(int exitCode)
{
	ExitThread(exitCode);
}

#else

int ircCreateThread(pthread_t *hHandle, void *(*startAddres)(void *), void *lpParameter)
{
	return pthread_create(hHandle, NULL, startAddres, lpParameter);
}

unsigned long ircWaitForThread(pthread_t hHandle, unsigned long dwMilliseconds)
{
	return (!pthread_join(hHandle, NULL) ? WAIT_OBJECT_0 : WAIT_TIMEOUT);
}

unsigned long ircWaitForMutex(pthread_mutex_t *hHandle, unsigned long dwMilliseconds)
{
	if (dwMilliseconds == 0)
		return (pthread_mutex_trylock(hHandle) ? WAIT_ABANDONED : WAIT_OBJECT_0);
	else
		return (!pthread_mutex_lock(hHandle) ? WAIT_OBJECT_0 : WAIT_TIMEOUT);
}

void ircCreateMutex(pthread_mutex_t *hMutex)
{
	pthread_mutexattr_t mutexAttr;
	pthread_mutexattr_init(&mutexAttr);
	pthread_mutex_init(hMutex, &mutexAttr);
}

void ircReleaseMutex(pthread_mutex_t *hMutex)
{
	pthread_mutex_unlock(hMutex);
}

void ircDestroyMutex(pthread_mutex_t *hMutex)
{
	pthread_mutex_destroy(hMutex);
}

void ircExitThread(int exitCode)
{
	pthread_exit((void*)exitCode);
}
#endif

void adminPrintf(int printlevel, char *fmt, ...)
{
	int i;
	va_list	argptr;
	char string[2048];

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	for (i=1; i<=tdmPlugFuncs.game->maxclients; i++)
	{
		if (!tdmPlugFuncs.edicts[i].inuse)
			continue;
		if (!tdmPlugFuncs.edicts[i].client->pers.save_data.is_admin)
			continue;
		tdmPlugFuncs.gi->cprintf(&tdmPlugFuncs.edicts[i], printlevel, string);
	}
}

void ClientCommand(unsigned int wParam, long lParam)
{
	char *cmd = (char *)wParam;
	edict_t *ent = (edict_t *)lParam;

	if (cmd[0] == 0 || !ent)
		return;

	if (!strcmp(cmd, "irc"))
	{
		char *ircCommand;
		if (tdmPlugFuncs.gi->argc() < 2)
			return;

		if (!ent->client->pers.save_data.is_admin)
		{
			tdmPlugFuncs.gi->cprintf(ent, PRINT_HIGH, "You are not an admin.\n");
			*(char *)wParam = 0;
			return;
		}

		ircCommand = tdmPlugFuncs.gi->argv(1);

		if (ircCommand[0] == '/')
		{
			char *buffPtr;

			buffPtr = ircCommand+1;

			if (!strcmp(buffPtr, "connect"))
				ircConnect(ent);
			else
				ircSendCommand(ent);
		}
		else //chat
		{
		}

		*(char *)wParam = 0;
	}
}

//#ifdef WIN32
//DWORD WINAPI 
//#else
//void *
//#endif
//cleanerThread(void *params)
//{
//}

#ifdef WIN32
__declspec(dllexport) 
#endif
void TDM_pluginLoad(struct sPluginFunctions *plugFunctions)
{
	memset(&tdmIrc, 0, sizeof(struct tdmIrcStruct));

	ircCreateMutex(&tdmIrc.hThreadsMutex);
	ircCreateMutex(&tdmIrc.hMainThreadMutex);
	ircCreateMutex(&tdmIrc.hSendMutex);

	tdmIrc.shutdown = false;
	tdmIrc.conState = CONSTATE_DISCONNECTED;

	memcpy(&tdmPlugFuncs, plugFunctions, sizeof(struct sPluginFunctions));
	tdmPlugFuncs.EventAddCallback(EVENT_CLIENTCOMMAND, ClientCommand);
	tdmPlugFuncs.EventAddCallback(EVENT_BPRINTF, PrintToIRC);

//#ifdef WIN32
//	tdmIrc.hCleanerThread = CreateThread(NULL, 0, cleanerThread, NULL, 0, (LPDWORD)&tdmIrc.cleanerThreadID);
//#else
//	pthread_create(&tdmIrc.cleanerThreadID, NULL, &cleanerThread, NULL);
//#endif
}

#ifdef WIN32
__declspec(dllexport) 
#endif
void TDM_pluginUnload()
{
	tdmIrc.shutdown = true;
	if (tdmIrc.conState != CONSTATE_DISCONNECTED)
		ircWaitForThread(tdmIrc.hMainThread, INFINITE);
	ircDestroyMutex(&tdmIrc.hMainThreadMutex);
	ircDestroyMutex(&tdmIrc.hThreadsMutex);
}