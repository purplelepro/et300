/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
 * Copyright (C) 2012 Konrad Mosoń <mosonkonrad@gmail.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 *
 * @file sv_trackbase.h
 * @brief Sends game statistics to Trackbase
 */
#ifdef TRACKBASE_SUPPORT

#include "server.h"
#include "sv_trackbase.h"

long t;
int modulo;
int waittime = 20; // seconds
char expect[16];
int expectnum;

qboolean maprunning = qfalse;

int querycl = -1;

enum {
	TB_BOT_NONE,
	TB_BOT_CONNECT
} catchBot;
qboolean catchBotNum = 0;

netadr_t addr;
netadr_t addrC;

char infostring[MAX_INFO_STRING];

char *TB_getGUID(client_t *cl);

char uMsg1[48];
char uMsg2[56];

void TB_Send(char *format, ...){
	va_list argptr;
	char    msg[MAX_MSGLEN];

	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg), format, argptr);
	va_end(argptr);

	NET_OutOfBandPrint(NS_SERVER, addr, "%s", msg);
}
void TB_SendX(char *format, ...){
	va_list argptr;
	char    msg[MAX_MSGLEN];

	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg), format, argptr);
	va_end(argptr);

	NET_OutOfBandPrint(NS_SERVER, addrC, "%s", msg);
}

void TB_Init()
{
	int i,j;

	t = time(0);
	expectnum = 0;

	Com_Printf( "Resolving %s\n", TRACKBASE_ADDR);
	if(!NET_StringToAdr(TRACKBASE_ADDR, &addr)){
		Com_Printf( "Couldn't resolve address: %s\n", TRACKBASE_ADDR);
	}else{
		Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", TRACKBASE_ADDR,addr.ip[0], addr.ip[1],addr.ip[2],addr.ip[3],BigShort(addr.port));
	}
	Com_Printf( "Resolving %s\n", TRACKBASE_ADDRX);
	if(!NET_StringToAdr(TRACKBASE_ADDRX, &addrC)){
		Com_Printf( "Couldn't resolve address: %s\n", TRACKBASE_ADDRX);
	}else{
		Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", TRACKBASE_ADDRX,addrC.ip[0], addrC.ip[1],addrC.ip[2],addrC.ip[3],BigShort(addrC.port));
	}

	//cpm "^7You are running an ^1old ET version^7!"
	sprintf(uMsg1,"ero\"$`9[qw\"ctg\"twppkpi\"cp\"`3qnf\"GV\"xgtukqp`9#$");
	i = strlen(uMsg1);
	for(j=0;j<i;j++){
		uMsg1[j] = (char)(((int)uMsg1[j])-2);
	}

	
	//cpm "Please update at ^3http://et.trackbase.net/update"
	sprintf(uMsg2,"dqn!#Qmfbtf!vqebuf!bu!_4iuuq;00fu/usbdlcbtf/ofu0vqebuf#");
	i = strlen(uMsg2);
	for(j=0;j<i;j++){
		uMsg2[j] = (char)(((int)uMsg2[j])-1);
	}
}

void TB_ServerStart()
{
	TB_Send("start");
}

void TB_ServerStop()
{
	TB_Send("stop");
}

void TB_ClientConnect(client_t *cl)
{
	TB_Send("connect %i %s %s", (int)(cl - svs.clients), TB_getGUID(cl), cl->name);
}

void TB_ClientDisconnect(client_t *cl)
{
	TB_Send("disconnect %i", (int)(cl - svs.clients));
}

void TB_ClientName(client_t *cl)
{
	if (!*cl->name)
	{
		return;
	}

	TB_Send("name %i %s %s", (int)(cl - svs.clients), TB_getGUID(cl), Info_ValueForKey(cl->userinfo, "name"));
}

void TB_ClientTeam(client_t *cl)
{
	playerState_t *ps;
	ps = SV_GameClientNum(cl - svs.clients);

	TB_Send("team %i %i %i %s", (int)(cl - svs.clients), Info_ValueForKey(Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE), "P")[cl - svs.clients], ps->stats[STAT_PLAYER_CLASS], cl->name);
}

void TB_Map(char *mapname)
{
	TB_Send("map %s", mapname);
	maprunning = qtrue;
}

void TB_MapRestart()
{
	TB_Send("maprestart");
	maprunning = qtrue;
}

void TB_MapEnd()
{
	TB_Send("mapend");
	TB_requestWeaponStats();
	maprunning = qfalse;
}

void TB_TeamSwitch(client_t *cl)
{
	TB_Send("team %i", (int)(cl - svs.clients));
}

char *TB_makeClientInfo(int clientNum)
{
	playerState_t *ps;
	ps = SV_GameClientNum(clientNum);

	return va("%i\\%i\\%c\\%i\\%s", svs.clients[clientNum].ping, ps->persistant[PERS_SCORE], Info_ValueForKey(Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE), "P")[clientNum], ps->stats[STAT_PLAYER_CLASS], svs.clients[clientNum].name);
}

void TB_requestWeaponStats()
{
	int i;
	qboolean onlybots = qtrue;
	char *P;

	if (!maprunning)
	{
		return;
	}

	strcpy(infostring, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE));
	P = Info_ValueForKey(infostring, "P");

	strcpy(expect, "ws");
	for (i = 0; i < sv_maxclients->value; i++)
	{
		if (svs.clients[i].state == CS_ACTIVE)
		{
			if (svs.clients[i].netchan.remoteAddress.type != NA_BOT)
			{
				onlybots = qfalse;
				querycl = i;
			}
			expectnum++;
		}
	}

	if (expectnum > 0)
	{
		TB_Send("wsc %i", expectnum);

		for (i = 0; i < sv_maxclients->value; i++)
		{
			if (svs.clients[i].state == CS_ACTIVE)
			{
				// send basic data is client is spectator
				if (P[i] == '3' || (svs.clients[i].netchan.remoteAddress.type == NA_BOT && onlybots))
				{
					TB_Send("ws %i 0 0 0\\%s", i, TB_makeClientInfo(i));
				}
			}
		}

		if (querycl >= 0)
		{
			SV_ExecuteClientCommand(&svs.clients[querycl], "statsall", qtrue, qfalse);
		}
	}
}

void TB_Frame(int msec)
{
	int i;
	if (catchBot == TB_BOT_CONNECT)
	{
		TB_ClientConnect(&svs.clients[catchBotNum]);
		catchBot = TB_BOT_NONE;
	}

	if (!(time(0) - waittime > t))
	{
		return;
	}

	TB_Send("p"); // send ping to tb to show that server is still alive

	expectnum = 0; // reset before next statsall
	TB_requestWeaponStats();

	t = time(0);

	modulo++;
	if(modulo == 9){ //Once in 180 sec
		
		for (i = 0; i < sv_maxclients->value; i++){
			if (svs.clients[i].state == CS_ACTIVE && svs.clients[i].protocol == 82){
				SV_SendServerCommand(&svs.clients[i],uMsg1);
				SV_SendServerCommand(&svs.clients[i],uMsg2);
			}
		}
		modulo = 0;
	}
}

qboolean TB_catchServerCommand(int clientNum, char *msg)
{
	int slot;

	if (clientNum != querycl)
	{
		return qfalse;
	}

	if (expectnum == 0)
	{
		return qfalse;
	}

	if (!(!strncmp(expect, msg, strlen(expect))))
	{
		return qfalse;
	}

	if (msg[strlen(msg) - 1] == '\n')
	{
		msg[strlen(msg) - 1] = '\0';
	}

	if (!Q_strncmp("ws", msg, 2))
	{
		expectnum--;
		if (expectnum == 0)
		{
			strcpy(expect, "");
			querycl = -1;
		}
		slot = 0;
		sscanf(msg, "ws %i", &slot);
		TB_Send("%s\\%s", msg, TB_makeClientInfo(slot));
		return qtrue;
	}

	return qfalse;
}

void TB_catchBotConnect(int clientNum)
{
	catchBot    = TB_BOT_CONNECT;
	catchBotNum = clientNum;
}

char *TB_getGUID(client_t *cl)
{
	char *cl_guid = Info_ValueForKey(cl->userinfo, "cl_guid");
	if (strcmp("", cl_guid) != 0 && strcmp("unknown", cl_guid) != 0){
		return cl_guid;
	} else {
		if (*Info_ValueForKey(cl->userinfo, "n_guid")){
			return Info_ValueForKey(cl->userinfo, "n_guid");
		} else if (*Info_ValueForKey(cl->userinfo, "sil_guid")) {
			return Info_ValueForKey(cl->userinfo, "sil_guid");
		} else {
			return "unknown";
		}
	}
}

void TB_clientCommand(client_t *cl, char *msg){

	char *temp;
	char tempC[1024];
	int i = 1;
	int j = 0;

	memset(tempC,0,1024);
	//Handle TB Commands
	if(sv_tbCommands->integer == 0){
		SV_SendServerCommand(cl,"print\n\"%dTB Commands are not enabled on this server!\n\"");
	}else{
		Cmd_TokenizeString(msg);
		temp = Cmd_Argv(0);
		j = Cmd_Argc();

		while(i<j){
			sprintf(tempC,"%s\\%s",tempC,Cmd_Argv(i));
			i++;
		}
		
		TB_SendX("tbc\\%d\\%s\\%s%s",(int)(cl - svs.clients),temp,cl->name,tempC);

		/*if(!Q_stricmp(temp,"tb_help")){
			SV_SendServerCommand(cl,"print\n\"^dAvailable TB Commands:\ntb_myrate    tb_lastmap\n\"",temp);
		}else if(!Q_stricmp(temp,"tb_myrate")){
			SV_SendServerCommand(cl,"print\n\"^dPlease stand by while information is being gathered!\n\"");
			TB_SendX("tbc\\%d\\tb_myrate\\%s",(int)(cl - svs.clients),cl->name);
		}else if(!Q_stricmp(temp,"tb_lastmap")){
			SV_SendServerCommand(cl,"print\n\"^dPlease stand by while information is being gathered!\n\"");
			TB_SendX("tbc\\%d\\tb_lastmap\\%s",(int)(cl - svs.clients),cl->name);
		}else{
			SV_SendServerCommand(cl,"print\n\"^dUnknown TB Command %s!\n\"",temp);
		}*/
	}
}

void TB_clientCommandReply(netadr_t from, char *msg){

	int cNum,isGlobal,mLocation,i,j;
	char *pch;
	char bufferSend[1024];
	char buffer[1024];
	memset(buffer,0,1024);
	memset(bufferSend,0,1024);

	if ( !NET_CompareBaseAdr( from, addrC ) ) {
		return;
	}
	Com_Printf(msg);
	sscanf(msg,"tbc %d %d %d",&cNum,&isGlobal,&mLocation);
	pch = strpbrk(msg,"\\\\");

	if(pch == NULL){
		return;
	}

	i = pch-msg;
	strncpy(buffer,msg+i+2,strlen(msg)-i-1);
	i = strlen(buffer);

	for(j=0;j<i;j++){
		if(buffer[j] == '\t'){
			buffer[j] = '\n';
		}
	}

	//If we have a user message, be sure the user EXISTS 
	if(cNum != 0){
		if(cNum < 0 || cNum > sv_maxclients->integer || svs.clients[cNum].state != CS_ACTIVE){
			return;
		}
	}

	switch(mLocation){
		case 1:{
			sprintf(bufferSend,"chat \"%s\"\r\n",buffer);
		}break;
		case 2:{
			sprintf(bufferSend,"cpm \"%s\"\r\n",buffer);
		}break;
		case 3:{
			sprintf(bufferSend,"cp \"%s\"\r\n",buffer);
		}break;
		case 4:{
			sprintf(bufferSend,"bp \"%s\"\r\n",buffer);
		}break;
		case 0: default:sprintf(bufferSend,"print\r\n\"%s\r\n\"\r\n",buffer);break;
	}
	if(isGlobal == 0){
		SV_SendServerCommand(&svs.clients[cNum] , "%s", bufferSend);
	}else{
		SV_SendServerCommand( NULL, "%s", bufferSend);
	}
}

void TB_catchChat(client_t *cl, char *msg){
	//Handle chat relay
	TB_SendX("cc\\%d\\%s\\%s",(int)(cl - svs.clients),Info_ValueForKey(cl->userinfo, "name"),msg);
	return;
}

#endif // TRACKBASE_SUPPORT
