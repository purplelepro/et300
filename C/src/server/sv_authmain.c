/*
===========================================================================
Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.
===========================================================================
*/

#include "sv_authmain.h"
#include "server.h"

void SV_ClientAuthorize( netadr_t from, char *arg1, char *arg2 );

void SV_AuthInit(){
	NET_StringToAdr(AUTH_ADDR, &svs.authorizeAddress);
}

void SV_ClientAuthorize( netadr_t from, char *arg1, char *arg2 ) {
	char *s ;
	char *msg ;
	int clientnum ;

	if ( !NET_CompareBaseAdr( from, svs.authorizeAddress ) ) {
		Com_Printf( "SV_ClientAuthorize: not from authorize server\n" );
		return;
	}


	s = arg1 ;
	clientnum = atoi( arg2 );

	if ( !Q_stricmp( s, "accept" ) ) {
		// allow client to connect
		Com_DPrintf( "[AUTH_SERVER] Clientnum: %d is clean\n", clientnum );
	}
	else if ( !Q_stricmp( s, "noallow" ) ) {
		// no allow client to connect
		Com_DPrintf( "[AUTH_SERVER] Clientnum: %d is globally banned\n", clientnum );
		NET_OutOfBandPrint( NS_SERVER, svs.clients[clientnum].netchan.remoteAddress, "print\r\n[err_dialog]You are globally banned banned from the ET servers!" );
		SV_DropClient( &svs.clients[clientnum], "is globally banned." );
	}
	else if ( !Q_stricmp( s, "sendmsg" ) ) {
		msg = Cmd_Argv( 4 );
		NET_OutOfBandPrint( NS_SERVER, svs.clients[clientnum].netchan.remoteAddress, "%s", msg );
		Com_DPrintf("[AUTH_SERVER] Sending message to Client %s\n", svs.clients[clientnum].name );
		// kick or what?
	}
}