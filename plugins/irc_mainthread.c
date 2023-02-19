#include "tdm_irc.h"

irc_thread_func ircMainThread(void *params)
{
	if (ircWaitForMutex(&tdmIrc.hMainThreadMutex, 0) == WAIT_OBJECT_0)
	{
		struct sConnectStruct *conStruct = (struct sConnectStruct *)params;
		struct hostent *server;
		struct timeval timeout;
		int ret;
		char *message = NULL;
		char msgReady = 0;

		tdmIrc.conState = CONSTATE_CONNECTING;

		server = gethostbyname(conStruct->adress);

		if (!server)
		{
			adminPrintf(PRINT_HIGH, "Unresovled hostname.\n");
			closesocket(tdmIrc.connSock);
			if (conStruct->adress)
				free(conStruct->adress);
			free(conStruct);
			tdmIrc.conState = CONSTATE_DISCONNECTED;
			ircReleaseMutex(&tdmIrc.hMainThreadMutex);
			ircExitThread(0);
		}

		memset(&tdmIrc.servAddr, 0, sizeof(struct sockaddr_in)); 

		tdmIrc.servAddr.sin_family = AF_INET;
		memcpy(&tdmIrc.servAddr.sin_addr.s_addr, server->h_addr, server->h_length);
		tdmIrc.servAddr.sin_port = htons(conStruct->port); 

		if (connect(tdmIrc.connSock, (struct sockaddr *)&tdmIrc.servAddr, (int)sizeof(tdmIrc.servAddr)) == -1)
		{
			adminPrintf(PRINT_HIGH, "Unable to connect to %s on port %d.\n", server->h_addr_list[0], conStruct->port);
			closesocket(tdmIrc.connSock);
			if (conStruct->adress)
				free(conStruct->adress);
			free(conStruct);
			tdmIrc.conState = CONSTATE_DISCONNECTED;
			ircReleaseMutex(&tdmIrc.hMainThreadMutex);
			ircExitThread(0);
		}

		if (conStruct->adress)
			free(conStruct->adress);
		free(conStruct);

		tdmIrc.conState = CONSTATE_CONNECTED;

//		send(tdmIrc.connSock, (void *)"PASS password\r\n", strlen("PASS password\r\n"), 0);
		send(tdmIrc.connSock, (void *)"NICK tdmtest\r\n", strlen("NICK tdmtest\r\n"), 0);
		send(tdmIrc.connSock, (void *)"USER tdmtest 0 unused :TDM IRC Client - by Harven\r\n", strlen("USER tdmtest 0 unused :TDM IRC Client - by Harven\r\n"), 0);

		while(tdmIrc.conState != CONSTATE_DISCONNECTED)
		{
			FD_ZERO(&tdmIrc.fdRead);
			FD_SET(tdmIrc.connSock, &tdmIrc.fdRead);
			
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			
			ret = select(tdmIrc.connSock+1, &tdmIrc.fdRead, NULL, NULL, &timeout);
			
			if (ret == -1)
			{
				adminPrintf(PRINT_HIGH, "Select error (%s:%d).\n", __FILE__, __LINE__);
				closesocket(tdmIrc.connSock);
				tdmIrc.conState = CONSTATE_DISCONNECTED;
				ircReleaseMutex(&tdmIrc.hMainThreadMutex);
				ircExitThread(0);
			}
			else if (ret)
			{
				int len;
				char recvBuff[1024];
				
				memset(recvBuff, 0, sizeof(recvBuff));

				len = recv(tdmIrc.connSock, (void*)recvBuff, sizeof(recvBuff), 0);
				if (!len)
				{
					adminPrintf(PRINT_HIGH, "Connection closed.\n");
					closesocket(tdmIrc.connSock);
					tdmIrc.conState = CONSTATE_DISCONNECTED;
					ircReleaseMutex(&tdmIrc.hMainThreadMutex);
					ircExitThread(0);
				}
				if (len == -1)
				{
					adminPrintf(PRINT_HIGH, "Read error.\n");
					closesocket(tdmIrc.connSock);
					tdmIrc.conState = CONSTATE_DISCONNECTED;
					ircReleaseMutex(&tdmIrc.hMainThreadMutex);
					ircExitThread(0);
				}
				else
				{
					char *buffPtr, *buffCopy, *tmp, *buffToParse;
					char *msgToAdmin = NULL;
					int i;

					recvBuff[len] = 0;
					buffPtr = recvBuff;

					do
					{
						if (msgReady)
							message = NULL;
						if (!buffPtr[0])
							msgReady = 0;
						for (i=0; i<strlen(buffPtr); i++)
						{
							if (buffPtr[i] == '\r' || i + 1 == strlen(buffPtr))
							{
								if (message && !msgReady)
								{
									message = (char*)realloc(message, strlen(message) + i + 2);
									message[strlen(message)] = 0;
								}
								else
								{
									message = (char*)malloc(i+2);
									message[0] = 0;
								}
								if (buffPtr[i] == '\r')
								{
									strncat(message, buffPtr, i);
									strcat(message, "\n");
									msgReady = 1;
								}
								else
								{
									strncat(message, buffPtr, i+1);
									msgReady = 0;
								}
								buffPtr = buffPtr + i + 2;
								break;
							}
						}

						if (!msgReady)
							break;

						len = strlen(message);

						if (message[0] == ':')
						{
							char *tmp = strtok(message, " ");
							tmp = strtok(NULL, " ");
							if (tmp)
							{
								if (isdigit(tmp[0]))
								{
									char cCode[4];
									int replyCode;

									strncpy(cCode, tmp, 3);
									replyCode = atoi(cCode);
									if (replyCode >= 400 && replyCode <= 599) //error code
										msgToAdmin = strdup(strtok(NULL, "\n"));
									else
									{
										switch(replyCode)
										{
											case 1:
												msgToAdmin = strdup(strtok(NULL, "\n"));
											case 2:
											case 3:
											case 4:
											case 5:
											break;
											case 372:
											break;
										}
									}
								}
							}
						}
						else if (len >= 6 && !strncmp(message, "PING :", 6))
						{
							char *tmp = strtok(message, "\n");
							if (tmp)
							{
								char *buffPtr = tmp + 6;
								if (buffPtr)
								{
									char *toSend = (char*)malloc(len+2);
									sprintf(toSend, "PONG %s\r\n", buffPtr);
									send(tdmIrc.connSock, (void*)toSend, strlen(toSend), 0);
								}
							}
						}

						free(message);
						if (msgToAdmin)
						{
							adminPrintf(PRINT_HIGH, "%s\n", msgToAdmin);
							free(msgToAdmin);
							msgToAdmin = NULL;
						}
					} while(message);
				}
			}
			//else
			//	tdmPlugFuncs.gi->dprintf("ircMainThread\n"); //timeout
			if (tdmIrc.shutdown)
			{
				tdmIrc.conState = CONSTATE_DISCONNECTED;
				closesocket(tdmIrc.connSock);
				ircReleaseMutex(&tdmIrc.hMainThreadMutex);
				ircExitThread(0);
			}
		}
		tdmIrc.conState = CONSTATE_DISCONNECTED;
		ircReleaseMutex(&tdmIrc.hMainThreadMutex);
	}
	ircExitThread(0);
}

void ircConnect(edict_t *ent)
{
	struct sConnectStruct *conStruct;

	if (tdmIrc.conState != CONSTATE_DISCONNECTED)
	{
		tdmPlugFuncs.gi->cprintf(ent, PRINT_HIGH, "TDM IRC Client is already connected to an irc server.\n");
		return;
	}

	if (tdmPlugFuncs.gi->argc() < 4)
	{
		tdmPlugFuncs.gi->cprintf(ent, PRINT_HIGH, "USAGE: irc /connect <hostname/IP> <port>.\n");
		return;
	}

	tdmIrc.connSock = socket(AF_INET, SOCK_STREAM, 0);
	if (tdmIrc.connSock == -1)
	{
		tdmPlugFuncs.gi->cprintf(ent, PRINT_HIGH, "Error during socket initialization.\n");
		return;
	}

	conStruct = (struct sConnectStruct *)malloc(sizeof(struct sConnectStruct));
	if (!conStruct)
	{
		tdmPlugFuncs.gi->cprintf(ent, PRINT_HIGH, "Memory allocation error (%s:%d).\n", __FILE__, __LINE__);
		return;
	}

	conStruct->adress = strdup(tdmPlugFuncs.gi->argv(2));
	if (!conStruct->adress)
	{
		free(conStruct);
		tdmPlugFuncs.gi->cprintf(ent, PRINT_HIGH, "Memory allocation error (%s:%d).\n", __FILE__, __LINE__);
		return;
	}
	conStruct->port = atoi(tdmPlugFuncs.gi->argv(3));

	ircCreateThread(&tdmIrc.hMainThread, &ircMainThread, (void*)conStruct);
}