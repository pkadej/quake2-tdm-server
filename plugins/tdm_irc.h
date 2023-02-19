#include "../../src/g_local.h"
#include "../../src/tdm_plugins.h"

#ifdef WIN32

#include <windows.h>
#include <winsock.h>
#define irc_thread_func DWORD WINAPI
typedef HANDLE irc_thread_handle;
typedef HANDLE irc_mutex_handle;
#else

#include <sys/time.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <sys/errno.h>
#include <err.h>
#include <unistd.h>
#include <pthread.h>

#define irc_thread_func void*
typedef pthread_t irc_thread_handle;
typedef pthread_mutex_t irc_mutex_handle;
#endif

#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT 258
#endif

#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0 0
#endif

#ifndef WAIT_ABANDONED
#define WAIT_ABANDONED 128
#endif

#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif

#ifndef closesocket
#define closesocket close
#endif

#define CONSTATE_DISCONNECTED 0
#define CONSTATE_CONNECTING 1
#define CONSTATE_CONNECTED 2

struct tdmIrcStruct
{
	int connSock;
	struct sockaddr_in servAddr;
	fd_set fdRead;
	char nick[16];

	irc_thread_handle hMainThread;
	irc_mutex_handle hThreadsMutex;
	irc_mutex_handle hMainThreadMutex;
	irc_mutex_handle hSendMutex;

	unsigned long int mainThreadID;
	qboolean shutdown;
	int conState;
};

struct sConnectStruct
{
	char *adress;
	int port;
};

struct sPluginFunctions tdmPlugFuncs;
struct tdmIrcStruct tdmIrc;void ircConnect(edict_t *ent);void adminPrintf(int printlevel, char *fmt, ...);void PrintToIRC(unsigned int wParam, long lParam);