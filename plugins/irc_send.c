#include "tdm_irc.h"

irc_thread_func ircSendthread(void *params)
{
	if (!ircWaitForMutex(&tdmIrc.hSendMutex, INFINITE))
	{
		char *toSend = (char *)params;
		send(tdmIrc.connSock, (void *)toSend, strlen(toSend), 0);
		if (toSend)
			free(toSend);
		ircReleaseMutex(&tdmIrc.hSendMutex);
	}
}

void PrintToIRC(unsigned int wParam, long lParam)
{
	char *toPrint = (char *)lParam;
	char *whole = NULL, *command, *buffPtr, *toPrintCopy, format[7];
	int len, formatLen = 0, wholeLen = 0;
	irc_thread_handle hThread;
	qboolean isColor = false;

	if (tdmIrc.conState != CONSTATE_CONNECTED)
		return;

	len = strlen(toPrint);

	toPrintCopy = strdup(toPrint);
	if (!toPrintCopy)
		return;

	format[formatLen] = 0;

	if (wParam & ATTR_BOLD)
	{
		format[formatLen] = 2;
		formatLen++;
	}
	if (wParam & ATTR_ITALIC)
	{
		format[formatLen] = 22;
		formatLen++;
	}
	if (wParam & ATTR_UNDERLINE)
	{
		format[formatLen] = 31;
		formatLen++;
	}

	format[formatLen] = 3;
	format[formatLen+1] = 0;
	
	if (wParam & ATTR_C_WHITE)
	{
		isColor = true;
		strcat(format, "00");
	}
	else if (wParam & ATTR_C_BLACK)
	{
		isColor = true;
		strcat(format, "01");
	}
	else if (wParam & ATTR_C_DKBLUE)
	{
		isColor = true;
		strcat(format, "02");
	}
	else if (wParam & ATTR_C_GREEN)
	{
		isColor = true;
		strcat(format, "03");
	}
	else if (wParam & ATTR_C_RED)
	{
		isColor = true;
		strcat(format, "04");
	}
	else if (wParam & ATTR_C_MAROON)
	{
		isColor = true;
		strcat(format, "05");
	}
	else if (wParam & ATTR_C_PURPLE)
	{
		isColor = true;
		strcat(format, "06");
	}
	else if (wParam & ATTR_C_ORANGE)
	{
		isColor = true;
		strcat(format, "07");
	}
	else if (wParam & ATTR_C_YELLOW)
	{
		isColor = true;
		strcat(format, "08");
	}
	else if (wParam & ATTR_C_LTGREEN)
	{
		isColor = true;
		strcat(format, "09");
	}
	else if (wParam & ATTR_C_TEAL)
	{
		isColor = true;
		strcat(format, "10");
	}
	else if (wParam & ATTR_C_CYAN)
	{
		isColor = true;
		strcat(format, "11");
	}
	else if (wParam & ATTR_C_BLUE)
	{
		isColor = true;
		strcat(format, "12");
	}
	else if (wParam & ATTR_C_FUCHSIA)
	{
		isColor = true;
		strcat(format, "13");
	}
	else if (wParam & ATTR_C_DKGRAY)
	{
		isColor = true;
		strcat(format, "14");
	}
	else if (wParam & ATTR_C_LTGRAY)
	{
		isColor = true;
		strcat(format, "15");
	}

	if (!isColor)
		format[formatLen] = 0;

	buffPtr = strtok(toPrintCopy, "\n");
	while(buffPtr)
	{
		if (!buffPtr)
			break;

		len = strlen(buffPtr);

		command = (char *)malloc(len + strlen("PRIVMSG #q2tdm :\r\n") + strlen(format) + 1);

		if (!command)
		{
			free(toPrintCopy);
			if (whole)
			{
				free(whole);
				whole = NULL;
			}
			break;
		}

		sprintf(command, "PRIVMSG #q2tdm :%s%s\r\n", format[0] ? format : "", buffPtr);
		whole = (char*)realloc(whole, wholeLen + strlen(command)+1);
		if (!whole)
		{
			free(toPrintCopy);
			free(command);
			return;
		}
		whole[wholeLen] = 0;
		strcat(whole, command);
		wholeLen += strlen(whole);
		free(command);

		buffPtr = strtok(NULL, "\n");
	}
	if (whole)
		ircCreateThread(&hThread, &ircSendthread, (void*)whole);
	free(toPrintCopy);
}

void ircSendCommand(edict_t *ent)
{
	irc_thread_handle hThread;
	char *command, *params;
	int i, len;

	if (tdmIrc.conState != CONSTATE_CONNECTED)
	{
		tdmPlugFuncs.gi->cprintf(ent, PRINT_HIGH, "TDM IRC Client is not connected.\n");
		return;
	}

	params = tdmPlugFuncs.gi->args();

	for (i=0; i<strlen(params); i++)
	{
		if (params[i] == '/')
			break;
	}

	len = strlen(params+i+1);
	command = (char*)malloc(len+3);
	strcpy(command, params+i+1);

	command[len] = '\r';
	command[len+1] = '\n';
	command[len+2] = 0;

	if (!command)
	{
		tdmPlugFuncs.gi->cprintf(ent, PRINT_HIGH, "Memory allocation error (%s:%d).\n", __FILE__, __LINE__);
		return;
	}

	ircCreateThread(&hThread, &ircSendthread, (void*)command);
}
