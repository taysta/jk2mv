// cl_main.c  -- client main loop

#include "client.h"
#include "../qcommon/strip.h"
#include <limits.h>
#include "snd_local.h"
#include <mv_setup.h>

#if !defined(G2_H_INC)
	#include "../ghoul2/G2_local.h"
#endif

#ifdef G2_COLLISION_ENABLED
#if !defined (MINIHEAP_H_INC)
#include "../qcommon/MiniHeap.h"
#endif
#endif

#ifdef _DONETPROFILE_
#include "../qcommon/INetProfile.h"
#endif

cvar_t	*cl_nodelta;
cvar_t	*cl_debugMove;

cvar_t	*cl_noprint;
cvar_t	*cl_motd;

cvar_t	*rcon_client_password;
cvar_t	*rconAddress;

cvar_t	*cl_timeout;
cvar_t	*cl_maxpackets;
cvar_t	*cl_packetdup;
cvar_t	*cl_timeNudge;
cvar_t	*cl_showTimeDelta;
cvar_t	*cl_freezeDemo;

cvar_t	*cl_drawRecording;

cvar_t	*cl_shownet;
cvar_t	*cl_showSend;
cvar_t	*cl_timedemo;
cvar_t	*cl_aviFrameRate;
cvar_t	*cl_aviMotionJpeg;
cvar_t	*cl_aviMotionJpegQuality;
cvar_t	*cl_forceavidemo;

cvar_t	*cl_freelook;
cvar_t	*cl_sensitivity;

cvar_t	*cl_mouseAccel;
cvar_t	*cl_showMouseRate;

cvar_t	*m_pitch;
cvar_t	*m_yaw;
cvar_t	*m_forward;
cvar_t	*m_side;
cvar_t	*m_filter;

cvar_t	*cl_activeAction;

cvar_t	*cl_motdString;

cvar_t	*mv_allowDownload;
cvar_t	*cl_conXOffset;
cvar_t	*cl_inGameVideo;

cvar_t	*cl_serverStatusResendTime;
cvar_t	*cl_trn;
cvar_t	*cl_framerate;

//EternalJK2MV
cvar_t	*cl_logChat;
cvar_t	*cl_idrive; //JAPRO ENGINE

cvar_t *cl_colorString;
cvar_t *cl_colorStringCount;
cvar_t *cl_colorStringRandom;

cvar_t	*cl_autolodscale;

cvar_t	*mv_slowrefresh;
cvar_t	*mv_coloredTextShadows;
cvar_t	*mv_consoleShiftRequirement;
cvar_t	*mv_menuOverride;

cvar_t	*cl_downloadName;
cvar_t	*cl_downloadLocalName;
cvar_t	*cl_downloadSize;
cvar_t	*cl_downloadCount;
cvar_t	*cl_downloadTime;
cvar_t	*cl_downloadProtocol;

vec3_t cl_windVec;

clientActive_t		cl;
clientConnection_t	clc;
clientStatic_t		cls;
vm_t				*cgvm;

netadr_t rcon_address;

// Structure containing functions exported from refresh DLL
refexport_t	re;

ping_t	cl_pinglist[MAX_PINGREQUESTS];

typedef struct serverStatus_s
{
	char string[BIG_INFO_STRING];
	netadr_t address;
	int time, startTime;
	qboolean pending;
	qboolean print;
	qboolean retrieved;
} serverStatus_t;

serverStatus_t cl_serverStatusList[MAX_SERVERSTATUSREQUESTS];
int serverStatusCount;

#ifdef G2_COLLISION_ENABLED
CMiniHeap *G2VertSpaceClient = 0;
#endif

#if defined __USEA3D && defined __A3D_GEOM
	void hA3Dg_ExportRenderGeom (refexport_t *incoming_re);
#endif

extern void SV_BotFrame( int time );
void CL_CheckForResend( void );
void CL_ShowIP_f(void);
void CL_ServerStatus_f(void);
void CL_ServerStatusResponse( netadr_t from, msg_t *msg );

/*
===============
CL_Video_f

video
video [filename]
===============
*/
void CL_Video_f( void )
{
	char  filename[ MAX_OSPATH ];
	int   i, last;

	if( !clc.demoplaying )
		{
			Com_Printf( "The video command can only be used when playing back demos\n" );
			return;
		}

	if( Cmd_Argc( ) == 2 )
		{
			// explicit filename
			Com_sprintf( filename, MAX_OSPATH, "videos/%s.avi", Cmd_Argv( 1 ) );
		}
	else
		{
			// scan for a free filename
			for( i = 0; i <= 9999; i++ )
				{
					int a, b, c, d;

					last = i;

					a = last / 1000;
					last -= a * 1000;
					b = last / 100;
					last -= b * 100;
					c = last / 10;
					last -= c * 10;
					d = last;

					Com_sprintf( filename, MAX_OSPATH, "videos/video%d%d%d%d.avi",
						     a, b, c, d );

					if( !FS_FileExists( filename ) )
						break; // file doesn't exist
				}

			if( i > 9999 )
				{
					Com_Printf( S_COLOR_RED "ERROR: no free file names to create video\n" );
					return;
				}
		}

	CL_OpenAVIForWriting( filename );
}

/*
===============
CL_StopVideo_f
===============
*/
void CL_StopVideo_f( void )
{
	CL_CloseAVI( );
}

/*
======================
EternalJK2MV Features
======================
*/
typedef struct bitInfo_S {
	const char	*string;
} bitInfo_t;

static bitInfo_t colors[] = {
	{ "^0BLACK" },
	{ "^1RED" },
	{ "^2GREEN" },
	{ "^3YELLOW" },
	{ "^4BLUE" },
	{ "^5CYAN" },
	{ "^6MAGENTA" },
	{ "^7WHITE" }/*,
	{ "^8ORANGE" },
	{ "^9GRAY" }*/
};
static const int MAX_COLORS = ARRAY_LEN(colors);

static void CL_ColorString_f(void) {
	if (Cmd_Argc() == 1) {
		int i = 0;
		for (i = 0; i < MAX_COLORS; i++) {
			if ((cl_colorString->integer & (1 << i))) {
				Com_Printf("%2d [X] %s\n", i, colors[i].string);
			}
			else {
				Com_Printf("%2d [ ] %s\n", i, colors[i].string);
			}
		}
		return;
	}
	else if (Cmd_Argc() == 2) {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_COLORS) - 1;
		
		Cmd_ArgvBuffer(1, arg, sizeof(arg));
		index = atoi(arg);

		if (index == -1) {
			Cvar_Set("cl_colorString", "0");
			Com_Printf("colorString: All colors are now ^1disabled\n");
			return;
		}

		if (index == MAX_COLORS) {
			Cvar_Set("cl_colorString", "1023");
			Com_Printf("colorString: All colors are now ^2enabled\n");
			return;
		}

		if (index < 0 || index >= MAX_COLORS) {
			Com_Printf("colorString: Invalid range: %i [0, %i]\n", index, MAX_COLORS);
			return;
		}

		Cvar_Set("cl_colorString", va("%i", (1 << index) ^ (cl_colorString->integer & mask)));

		Com_Printf("%s %s^7\n", colors[index].string, ((cl_colorString->integer & (1 << index))
			? "^2Enabled" : "^1Disabled"));
	}
	else {
		char arg[8] = { 0 };
		int i, argc, index[10], bits = 0;
		const uint32_t mask = (1 << MAX_COLORS) - 1;

		if ((argc = Cmd_Argc() - 1) > MAX_COLORS) {
			Com_Printf("colorString: More than %i arguments were entered", MAX_COLORS);
			return;
		}

		for (i = 0; i < argc; i++) {
			Cmd_ArgvBuffer(i+1, arg, sizeof(arg));
			index[i] = atoi(arg);

			if (index[i] < 0 || index[i] >= 10) {
				Com_Printf("colorString: Invalid range: %i [0, %i]\n", index[i], 9);
				return;
			}

			if ((bits & mask) & (1 << index[i])) {
				Com_Printf("colorString: %s ^7was entered more than once\n", colors[index[i]].string);
				return;
			}

			bits = (1 << index[i]) ^ (bits & mask);
		}

		Cvar_Set("cl_colorString", va("%i", bits));

		for (i = 0; i < MAX_COLORS; i++) {
			if ((cl_colorString->integer & (1 << i))) {
				Com_Printf("%2d [X] %s\n", i, colors[i].string);
			}
			else {
				Com_Printf("%2d [ ] %s\n", i, colors[i].string);
			}
		}
	}
}

void CL_RandomizeColors(const char* in, char *out) {
	int count = cl_colorStringCount->integer;
	int i, random, j = 0, store = 0;
	const char *p = in;
	char *s = out, c;
	
	while ((c = *p++)) {
		if (c == '^' && *p != '\0' && *p >= '0' && *p <= '9') {
			*s++ = c;
			*s++ = *p++;
			c = *p++;
			store = 0;
		}
		else if (count == 1) {
			if (store == 0) {
				for (i = 0; i < 10; i++) {
					if ((cl_colorString->integer & (1 << i))) {
						store = i;
						*s++ = '^';
						*s++ = i + '0';
						break;
					}
				}
			}
		}
		else if (store != (random = irand(1, count*(store == 0
			? 1 : cl_colorStringRandom->integer))) && random <= count) {
			for (i = 0; i < 10; i++) {
				if ((cl_colorString->integer & (1 << i)) && (random - 1) == j++) {
					store = random;
					*s++ = '^';
					*s++ = i + '0';
				}
			}
			j = 0;
		}
		*s++ = c;
	}
	*s = '\0';
}

static void CL_ColorName_f(void) {
	char name[MAX_TOKEN_CHARS];
	char coloredName[MAX_TOKEN_CHARS];
	int storebits = cl_colorString->integer;
	int storebitcount = cl_colorStringCount->integer;

	Cvar_VariableStringBuffer("name", name, sizeof(name));
	Q_StripColor(name);
	if (Cmd_Argc() == 1) {
		CL_RandomizeColors(name, coloredName);
		Cvar_Set("name", va("%s", coloredName));
		return;
	}
	else if (Cmd_Argc() == 2) {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << 10) - 1;

		Cmd_ArgvBuffer(1, arg, sizeof(arg));
		index = atoi(arg);

		if (index == -1) {
			Cvar_Set("name", va("%s", name));
			return;
		}

		if (index == 10) {
			cl_colorString->integer = 1023;
			cl_colorStringCount->integer = 10;
			CL_RandomizeColors(name, coloredName);
			Cvar_Set("name", va("%s", coloredName));
			cl_colorString->integer = storebits;
			cl_colorStringCount->integer = storebitcount;
			return;
		}

		if (index < 0 || index >= 10) {
			Com_Printf("colorName: Invalid range: %i [0, %i]\n", index, 9);
			return;
		}

		cl_colorStringCount->integer = 1;
		cl_colorString->integer = 0;
		cl_colorString->integer = (1 << index) ^ (cl_colorString->integer & mask);
	}
	else {
		char arg[8] = { 0 };
		int i, argc, index[10], bits = 0;
		const uint32_t mask = (1 << 10) - 1;

		if ((argc = Cmd_Argc() - 1) > 10) {
			Com_Printf("colorName: More than 10 arguments were entered");
			return;
		}

		for (i = 0; i < argc; i++) {
			Cmd_ArgvBuffer(i + 1, arg, sizeof(arg));
			index[i] = atoi(arg);

			if (index[i] < 0 || index[i] >= 10) {
				Com_Printf("colorName: Invalid range: %i [0, %i]\n", index[i], 9);
				return;
			}

			if ((bits & mask) & (1 << index[i])) {
				Com_Printf("colorName: %s ^7was entered more than once\n", colors[index[i]].string);
				return;
			}

			bits = (1 << index[i]) ^ (bits & mask);
		}

		cl_colorStringCount->integer = argc;
		cl_colorString->integer = bits;
	}

	CL_RandomizeColors(name, coloredName);
	Cvar_Set("name", va("%s", coloredName));
	cl_colorString->integer = storebits;
	cl_colorStringCount->integer = storebitcount;
}

/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
======================
CL_AddReliableCommand

The given command will be transmitted to the server, and is gauranteed to
not have future usercmd_t executed before it is executed
======================
*/
void CL_AddReliableCommand( const char *cmd ) {
	int		index;

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	if ( clc.reliableSequence - clc.reliableAcknowledge > MAX_RELIABLE_COMMANDS ) {
		Com_Error( ERR_DROP, "Client command overflow" );
	}
	clc.reliableSequence++;
	index = clc.reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	Q_strncpyz( clc.reliableCommands[ index ], cmd, sizeof( clc.reliableCommands[ index ] ) );
}

/*
======================
CL_ChangeReliableCommand
======================
*/
void CL_ChangeReliableCommand( void ) {
	int r, index, l;

	r = clc.reliableSequence - ((int)(qrandom()) * 5);
	index = clc.reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	l = (int)strlen(clc.reliableCommands[index]);
	if ( l >= MAX_STRING_CHARS - 1 ) {
		l = MAX_STRING_CHARS - 2;
	}
	clc.reliableCommands[ index ][ l ] = '\n';
	clc.reliableCommands[ index ][ l+1 ] = '\0';
}

/*
=======================================================================

CLIENT SIDE DEMO RECORDING

=======================================================================
*/

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage ( msg_t *msg, int headerBytes ) {
	int		len, swlen;

	// write the packet sequence
	len = clc.serverMessageSequence;
	swlen = LittleLong( len );
	FS_Write (&swlen, 4, clc.demofile);

	// skip the packet sequencing information
	len = msg->cursize - headerBytes;
	swlen = LittleLong(len);
	FS_Write (&swlen, 4, clc.demofile);
	FS_Write ( msg->data + headerBytes, len, clc.demofile );
}


/*
====================
CL_StopRecording_f

stop recording a demo
====================
*/
void CL_StopRecord_f( void ) {
	int		len;

	if ( !clc.demorecording ) {
		Com_Printf ("Not recording a demo.\n");
		return;
	}

	// finish up
	len = -1;
	FS_Write (&len, 4, clc.demofile);
	FS_Write (&len, 4, clc.demofile);
	FS_FCloseFile (clc.demofile);
	clc.demofile = 0;
	clc.demorecording = qfalse;
	clc.spDemoRecording = qfalse;
	Com_Printf ("Stopped demo.\n");
}

/*
==================
CL_DemoFilename
==================
*/
void CL_DemoFilename( char *buf, int bufSize ) {
	time_t rawtime;
	char timeStr[32] = {0}; // should really only reach ~19 chars

	time( &rawtime );
	strftime( timeStr, sizeof( timeStr ), "%Y-%m-%d_%H-%M-%S", localtime( &rawtime ) ); // or gmtime

	Com_sprintf( buf, bufSize, "demo%s", timeStr );
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/

void CL_Record_f( void ) {
	char		name[MAX_OSPATH];
	byte		bufData[MAX_MSGLEN];
	msg_t	buf;
	int			i;
	int			len;
	entityState_t	*ent;
	entityState_t	nullstate;
	char		*s;
	char		demoName[MAX_OSPATH];	//bufsize was MAX_QPATH, but it should be MAX_OSPATH since this is the assumed buffersize in CL_DemoFilename

	if ( Cmd_Argc() > 2 ) {
		Com_Printf ("record <demoname>\n");
		return;
	}

	if ( clc.demorecording ) {
		if (!clc.spDemoRecording) {
			Com_Printf ("Already recording.\n");
		}
		return;
	}

	if ( cls.state != CA_ACTIVE ) {
		Com_Printf ("You must be in a level to record.\n");
		return;
	}

	#if 0 //annoyance
	if ( !Cvar_VariableValue( "g_synchronousClients" ) ) {
		Com_Printf ("The server must have 'g_synchronousClients 1' set for demos\n");
		return;
	}
	#endif

	if ( Cmd_Argc() == 2 ) {
		s = Cmd_Argv(1);
		Q_strncpyz( demoName, s, sizeof( demoName ) );
		Com_sprintf (name, sizeof(name), "demos/%s.dm_%d", demoName, MV_GetCurrentProtocol() );
	} else {
		// timestamp the file
		CL_DemoFilename( demoName, sizeof( demoName ) );

		Com_sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, MV_GetCurrentProtocol());

		if ( FS_FileExists( name ) ) {
			Com_Printf( "Record: Couldn't create a file\n");
			return;
 		}
	}

	// open the demo file

	Com_Printf ("recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite( name );
	if ( !clc.demofile ) {
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}
	clc.demorecording = qtrue;
	if (Cvar_VariableValue("ui_recordSPDemo")) {
	  clc.spDemoRecording = qtrue;
	} else {
	  clc.spDemoRecording = qfalse;
	}


	Q_strncpyz( clc.demoName, demoName, sizeof( clc.demoName ) );

	// don't start saving messages until a non-delta compressed message is received
	clc.demowaiting = qtrue;

	// write out the gamestate message
	MSG_Init (&buf, bufData, sizeof(bufData));
	MSG_Bitstream(&buf);

	// NOTE, MRE: all server->client messages now acknowledge
	MSG_WriteLong( &buf, clc.reliableSequence );

	MSG_WriteByte (&buf, svc_gamestate);
	MSG_WriteLong (&buf, clc.serverCommandSequence );

	// configstrings
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( !cl.gameState.stringOffsets[i] ) {
			continue;
		}
		s = cl.gameState.stringData + cl.gameState.stringOffsets[i];
		MSG_WriteByte (&buf, svc_configstring);
		MSG_WriteShort (&buf, i);
		MSG_WriteBigString (&buf, s);
	}

	// baselines
	Com_Memset (&nullstate, 0, sizeof(nullstate));
	for ( i = 0; i < MAX_GENTITIES ; i++ ) {
		ent = &cl.entityBaselines[i];
		if ( !ent->number ) {
			continue;
		}
		MSG_WriteByte (&buf, svc_baseline);
		MSG_WriteDeltaEntity (&buf, &nullstate, ent, qtrue );
	}

	MSG_WriteByte( &buf, svc_EOF );

	// finished writing the gamestate stuff

	// write the client num
	MSG_WriteLong(&buf, clc.clientNum);
	// write the checksum feed
	MSG_WriteLong(&buf, clc.checksumFeed);

	// finished writing the client packet
	MSG_WriteByte( &buf, svc_EOF );

	// write it to the demo file
	len = LittleLong( clc.serverMessageSequence - 1 );
	FS_Write (&len, 4, clc.demofile);

	len = LittleLong (buf.cursize);
	FS_Write (&len, 4, clc.demofile);
	FS_Write (buf.data, buf.cursize, clc.demofile);

	// the rest of the demo file will be copied from net messages
}

/*
=======================================================================

CLIENT SIDE DEMO PLAYBACK

=======================================================================
*/

/*
=================
CL_DemoCompleted
=================
*/
void CL_DemoCompleted( void ) {
	if (cl_timedemo && cl_timedemo->integer) {
		int	time;

		time = Sys_Milliseconds() - clc.timeDemoStart;
		if ( time > 0 ) {
			Com_Printf ("%i frames, %3.1f seconds: %3.1f fps\n", clc.timeDemoFrames,
			time/1000.0, clc.timeDemoFrames*1000.0 / time);
		}
	}

	CL_NextDemo();
	CL_Disconnect_f();
	// disconnect here does long jump, don't put anything after
}

/*
=================
CL_ReadDemoMessage
=================
*/
void CL_ReadDemoMessage( void ) {
	int			r;
	msg_t		buf;
	byte		bufData[ MAX_MSGLEN ];
	int			s;

	if ( !clc.demofile ) {
		CL_DemoCompleted ();
		return;
	}

	// get the sequence number
	r = FS_Read( &s, 4, clc.demofile);
	if ( r != 4 ) {
		CL_DemoCompleted ();
		return;
	}
	clc.serverMessageSequence = LittleLong( s );

	// init the message
	MSG_Init( &buf, bufData, sizeof( bufData ) );

	// get the length
	r = FS_Read (&buf.cursize, 4, clc.demofile);
	if ( r != 4 ) {
		CL_DemoCompleted ();
		return;
	}
	buf.cursize = LittleLong( buf.cursize );
	if ( buf.cursize == -1 ) {
		CL_DemoCompleted ();
		return;
	}
	if ( buf.cursize > buf.maxsize ) {
		Com_Error (ERR_DROP, "CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN");
	}
	r = FS_Read( buf.data, buf.cursize, clc.demofile );
	if ( r != buf.cursize ) {
		Com_Printf( "Demo file was truncated.\n");
		CL_DemoCompleted ();
		return;
	}

	clc.lastPacketTime = cls.realtime;
	buf.readcount = 0;
	CL_ParseServerMessage( &buf );
}

/*
====================
CL_PlayDemo_f

demo <demoname>

====================
*/

bool demoCheckFor103 = false;	//When a protocol15 demo has been started, we need to ascertain whether the demo is 1.03 or 1.02 version.

qboolean CL_ServerVersionIs103 (const char *versionstr) {
	return strstr(versionstr, "v1.03") ? qtrue : qfalse;
}

void CL_PlayDemo_f( void ) {
	char		name[MAX_OSPATH]/*, extension[32]*/;
	char		arg[MAX_OSPATH];

	if (Cmd_Argc() != 2) {
		Com_Printf ("demo <demoname>\n");
		return;
	}

	Q_strncpyz(arg, Cmd_Argv(1), sizeof(arg));

	// make sure a local server is killed
	Cvar_Set( "sv_killserver", "1" );
	SV_Frame( 0 );

	CL_Disconnect( qtrue );

	/* MrE: 2000-09-13: now called in CL_DownloadsComplete
	CL_FlushMemory( );
	*/

	// open the demo file
	if ( !Q_stricmp( arg + strlen(arg) - strlen(".dm_15"), ".dm_15" ) || !Q_stricmp( arg + strlen(arg) - strlen(".dm_16"), ".dm_16" ) )
	{ // Load "dm_15" and "dm_16" demos.
		Com_sprintf (name, sizeof(name), "demos/%s", arg);

		FS_FOpenFileRead( name, &clc.demofile, qtrue );
		if (!clc.demofile)
		{
			if (!Q_stricmp(arg, "(null)"))
			{
				Com_Error( ERR_DROP, "%s", SP_GetStringTextString("CON_TEXT_NO_DEMO_SELECTED") );
			}
			else
			{
				Com_Error( ERR_DROP, "couldn't open %s", name);
			}
			return;
		}
	}
	else
	{
		// Check for both, "dm_15" and "dm_16".
		Com_sprintf(name, sizeof(name), "demos/%s.dm_15", arg);
		FS_FOpenFileRead( name, &clc.demofile, qtrue );
		if ( !clc.demofile )
		{
			Com_sprintf(name, sizeof(name), "demos/%s.dm_16", arg);
			FS_FOpenFileRead( name, &clc.demofile, qtrue );
			if ( !clc.demofile )
			{
				if (!Q_stricmp(arg, "(null)"))
				{
					Com_Error( ERR_DROP, "%s", SP_GetStringTextString("CON_TEXT_NO_DEMO_SELECTED") );
				}
				else
				{
					Com_Error( ERR_DROP, "couldn't open demos/%s.dm_15 or demos/%s.dm_16", arg, arg);
				}
				return;
			}
		}
	}
	Q_strncpyz( clc.demoName, arg, sizeof( clc.demoName ) );

	Con_Close();

	cls.state = CA_CONNECTED;
	clc.demoplaying = qtrue;
	com_demoplaying = qtrue;

	Q_strncpyz( cls.servername, arg, sizeof( cls.servername ) );

	// Set the protocol according to the the demo-file.
	if ( !Q_stricmp( name + strlen(name) - strlen(".dm_15"), ".dm_15" ) ) {
		MV_SetCurrentGameversion(VERSION_1_02);
		demoCheckFor103 = true;	//if this demo happens to be a 1.03 demo, check for that in CL_ParseGamestate
	}
	else if ( !Q_stricmp( name + strlen(name) - strlen(".dm_16"), ".dm_16" ) ) {
		MV_SetCurrentGameversion(VERSION_1_04);
	}

	// read demo messages until connected
	while ( cls.state >= CA_CONNECTED && cls.state < CA_PRIMED ) {
		CL_ReadDemoMessage();
	}
	// don't get the first snapshot this frame, to prevent the long
	// time from the gamestate load from messing causing a time skip
	clc.firstDemoFrameSkipped = qfalse;
}


/*
====================
CL_StartDemoLoop

Closing the main menu will restart the demo loop
====================
*/
void CL_StartDemoLoop( void ) {
	// start the demo loop again
	Cbuf_AddText ("d1\n");
	cls.keyCatchers = 0;
}

/*
==================
CL_NextDemo

Called when a demo or cinematic finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
void CL_NextDemo( void ) {
	char	v[MAX_STRING_CHARS];

	Q_strncpyz( v, Cvar_VariableString ("nextdemo"), sizeof(v) );
	v[MAX_STRING_CHARS-1] = 0;
	Com_DPrintf("CL_NextDemo: %s\n", v );
	if (!v[0]) {
		return;
	}

	Cvar_Set ("nextdemo","");
	Cbuf_AddText (v);
	Cbuf_AddText ("\n");
	Cbuf_Execute();
}


//======================================================================

/*
=====================
CL_ShutdownAll
=====================
*/
void CL_ShutdownAll(void) {
	CL_KillDownload();

	// stop recording
	CL_CloseAVI();
	// clear sounds
	S_DisableSounds();
	// shutdown CGame
	CL_ShutdownCGame();
	// shutdown UI
	CL_ShutdownUI();

	// shutdown the renderer
	if ( re.Shutdown ) {
		re.Shutdown( qfalse );		// don't destroy window or context
	}

	cls.uiStarted = qfalse;
	cls.cgameStarted = qfalse;
	cls.rendererStarted = qfalse;
	cls.soundRegistered = qfalse;
}

/*
=================
CL_FlushMemory

Called by CL_MapLoading, CL_Connect_f, CL_PlayDemo_f, and CL_ParseGamestate the only
ways a client gets into a game
Also called by Com_Error
=================
*/
extern void FixGhoul2InfoLeaks(bool);

void CL_FlushMemory( qboolean disconnecting ) {

	// shutdown all the client stuff
	CL_ShutdownAll();

	// if not running a server clear the whole hunk
	if ( !com_sv_running->integer ) {
		// clear collision map data
		FixGhoul2InfoLeaks(false);
		CM_ClearMap();
		// clear the whole hunk
		Hunk_Clear();
	}
	else {
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	if (disconnecting && !com_sv_running->integer) {
		MV_SetCurrentGameversion(VERSION_UNDEF);

		FS_PureServerSetReferencedPaks("", "");
		// change checksum feed so that next FS_ConditionalRestart()
		// works when connecting back to the same server
		clc.checksumFeed = 0;
		FS_Restart(clc.checksumFeed);
	}

	CL_StartHunkUsers();
}

/*
=====================
CL_MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void CL_MapLoading( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}

	// Set this to localhost.
	Cvar_Set( "cl_currentServerAddress", "Localhost");

	Con_Close();
	cls.keyCatchers = 0;

	// if we are already connected to the local host, stay connected
	if ( cls.state >= CA_CONNECTED && !Q_stricmp( cls.servername, "localhost" ) ) {
		cls.state = CA_CONNECTED;		// so the connect screen is drawn
		Com_Memset( cls.updateInfoString, 0, sizeof( cls.updateInfoString ) );
		Com_Memset( clc.serverMessage, 0, sizeof( clc.serverMessage ) );
		Com_Memset( &cl.gameState, 0, sizeof( cl.gameState ) );
		clc.lastPacketSentTime = -9999;
		SCR_UpdateScreen();
	} else {
		// clear nextmap so the cinematic shutdown doesn't execute it
		Cvar_Set( "nextmap", "" );
		CL_Disconnect( qtrue );
		Q_strncpyz( cls.servername, "localhost", sizeof(cls.servername) );
		cls.state = CA_CHALLENGING;		// so the connect screen is drawn
		cls.keyCatchers = 0;
		SCR_UpdateScreen();
		clc.connectTime = -RETRANSMIT_TIMEOUT;
		NET_StringToAdr( cls.servername, &clc.serverAddress);
		// we don't need a challenge on the localhost

		CL_CheckForResend();
	}
}

/*
=====================
CL_ClearState

Called before parsing a gamestate
=====================
*/
void CL_ClearState (void) {

//	S_StopAllSounds();
	Com_Memset( &cl, 0, sizeof( cl ) );
}


/*
=====================
CL_Disconnect

Called when a connection, demo, or cinematic is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect( qboolean showMainMenu ) {
	if ( !com_cl_running || !com_cl_running->integer ) {
		return;
	}

	// shutting down the client so enter full screen ui mode
	Cvar_Set("r_uiFullScreen", "1");

	if ( clc.demorecording ) {
		CL_StopRecord_f ();
	}

	com_demoplaying = qfalse;

	CL_KillDownload();

	if ( clc.demofile ) {
		FS_FCloseFile( clc.demofile );
		clc.demofile = 0;
	}

	CL_BlacklistWriteCloseFile();

	if ( uivm && showMainMenu ) {
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE );
	}

	Cvar_Set("timescale", "1");	//in case we dropped from a timescaled demo, ensure we are at normal speed again.

	SCR_StopCinematic ();
	S_ClearSoundBuffer();

	// send a disconnect message to the server
	// send it a few times in case one is dropped
	if ( cls.state >= CA_CONNECTED ) {
		CL_AddReliableCommand( "disconnect" );
		CL_WritePacket();
		CL_WritePacket();
		CL_WritePacket();
	}

	CL_ClearState ();

	// wipe the client connection
	Com_Memset( &clc, 0, sizeof( clc ) );

	cls.state = CA_DISCONNECTED;

	// not connected to a pure server anymore
	cl_connectedToPureServer = qfalse;

	if( CL_VideoRecording( ) ) {
		// Finish rendering current frame
		SCR_UpdateScreen( );
		CL_CloseAVI( );
	}

	WIN_SetTaskbarState(TBS_NOTIFY, 0, 0);
}


/*
===================
CL_ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void CL_ForwardCommandToServer( const char *string ) {
	char	*cmd;

	cmd = Cmd_Argv(0);

	// ignore key up commands
	if ( cmd[0] == '-' ) {
		return;
	}

	if ( clc.demoplaying || cls.state < CA_CONNECTED || cmd[0] == '+' ) {
		Com_Printf ("Unknown command \"%s\"\n", cmd);
		return;
	}

	if ( Cmd_Argc() > 1 ) {
		CL_AddReliableCommand( string );
	} else {
		CL_AddReliableCommand( cmd );
	}
}

/*
===================
CL_RequestMotd

===================
*/
void CL_RequestMotd( void ) {
	char		info[MAX_INFO_STRING];

	if ( !cl_motd->integer ) {
		return;
	}
	Com_Printf( "Resolving %s\n", UPDATE_SERVER_NAME );
	if ( !NET_StringToAdr( UPDATE_SERVER_NAME, &cls.updateServer  ) ) {
		Com_Printf( "Couldn't resolve address\n" );
		return;
	}
	cls.updateServer.port = BigShort( PORT_UPDATE );
	Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", UPDATE_SERVER_NAME,
		cls.updateServer.ip[0], cls.updateServer.ip[1],
		cls.updateServer.ip[2], cls.updateServer.ip[3],
		BigShort( cls.updateServer.port ) );

	info[0] = 0;
  // NOTE TTimo xoring against Com_Milliseconds, otherwise we may not have a true randomization
  // only srand I could catch before here is tr_noise.c l:26 srand(1001)
  // https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=382
  // NOTE: the Com_Milliseconds xoring only affects the lower 16-bit word,
  //   but I decided it was enough randomization
	Com_sprintf( cls.updateChallenge, sizeof( cls.updateChallenge ), "%i", ((rand() << 16) ^ rand()) ^ Com_Milliseconds());


	Info_SetValueForKey( info, "challenge", cls.updateChallenge );
	Info_SetValueForKey( info, "renderer", cls.glconfig.renderer_string );
//	Info_SetValueForKey( info, "version", com_version->string );

	// Always send the jk2mv "version" to the MOTD server
	Info_SetValueForKey( info, "version", va("%s %s %s", Q3_VERSION, PLATFORM_STRING, __DATE__ ) );
	Info_SetValueForKey( info, "JK2MV", JK2MV_VERSION );

	NET_OutOfBandPrint( NS_CLIENT, cls.updateServer, "getmotd \"%s\"\n", info );
}

/*
======================================================================

CONSOLE COMMANDS

======================================================================
*/

/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f( void ) {
	if ( cls.state != CA_ACTIVE || clc.demoplaying ) {
		Com_Printf ("Not connected to a server.\n");
		return;
	}

	// don't forward the first argument
	if ( Cmd_Argc() > 1 ) {
		CL_AddReliableCommand( Cmd_Args() );
	}
}

/*
==================
CL_Setenv_f

Mostly for controlling voodoo environment variables
==================
*/
void CL_Setenv_f( void ) {
	int argc = Cmd_Argc();

	if ( argc > 2 ) {
		char buffer[1024];

		Q_strncpyz( buffer, Cmd_Argv(1), sizeof( buffer ) );
		Q_strcat( buffer, sizeof( buffer ), "=" );
		Q_strcat( buffer, sizeof( buffer ), Cmd_ArgsFrom(2) );

		if ( putenv( buffer ) != 0 )
			Com_Printf( "Unable to set environment variable\n" );
	} else if ( argc == 2 ) {
		char *env = getenv( Cmd_Argv(1) );

		if ( env ) {
			Com_Printf( "%s=%s\n", Cmd_Argv(1), env );
		} else {
			Com_Printf( "%s undefined\n", Cmd_Argv(1) );
		}
	}
}


/*
==================
CL_Disconnect_f
==================
*/
void CL_Disconnect_f( void ) {
	SCR_StopCinematic();
	Cvar_Set("ui_singlePlayerActive", "0");
	if ( cls.state != CA_DISCONNECTED && cls.state != CA_CINEMATIC ) {
		Com_Error (ERR_DISCONNECT, "Disconnected from server");
	}
}


/*
================
CL_Reconnect_f

================
*/
void CL_Reconnect_f( void ) {
	if ( !strlen( cls.servername ) || !strcmp( cls.servername, "localhost" ) ) {
		Com_Printf( "Can't reconnect to localhost.\n" );
		return;
	}
	Cvar_Set("ui_singlePlayerActive", "0");
	Cbuf_AddText( va("connect %s\n", cls.servername ) );
}

/*
================
CL_Connect_f

================
*/
void CL_Connect_f( void ) {
	char	server[MAX_OSPATH];

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: connect [server]\n");
		return;
	}

	Cvar_Set("ui_singlePlayerActive", "0");

	com_demoplaying = qfalse;

	// fire a message off to the motd server
	CL_RequestMotd();

	// clear any previous "server full" type messages
	clc.serverMessage[0] = 0;

	Q_strncpyz(server, Cmd_Argv(1), sizeof(server));

	if ( com_sv_running->integer && !strcmp( server, "localhost" ) ) {
		// if running a local server, kill it
		SV_Shutdown( "Server quit" );
	}

	// make sure a local server is killed
	Cvar_Set( "sv_killserver", "1" );
	SV_Frame( 0 );

	CL_Disconnect( qtrue );
	Con_Close();

	Q_strncpyz( cls.servername, server, sizeof(cls.servername) );

	if (!NET_StringToAdr( cls.servername, &clc.serverAddress) ) {
		Com_Printf ("Bad server address\n");
		cls.state = CA_DISCONNECTED;
		return;
	}
	if (clc.serverAddress.port == 0) {
		clc.serverAddress.port = BigShort( PORT_SERVER );
	}
	Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", cls.servername,
		clc.serverAddress.ip[0], clc.serverAddress.ip[1],
		clc.serverAddress.ip[2], clc.serverAddress.ip[3],
		BigShort( clc.serverAddress.port ) );

	// if we aren't playing on a lan, we need to authenticate
	// with the cd key
	if ( NET_IsLocalAddress( clc.serverAddress ) ) {
		cls.state = CA_CHALLENGING;
	} else {
		cls.state = CA_CONNECTING;
	}



	cls.keyCatchers = 0;
	clc.connectTime = -99999;	// CL_CheckForResend() will fire immediately
	clc.connectPacketCount = 0;

	// server connection string
	Cvar_Set( "cl_currentServerAddress", server );
}

#define MAX_RCON_MESSAGE 1024

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f( void ) {
	char	message[MAX_RCON_MESSAGE];

	if (!strlen(rcon_client_password->string)) {
		Com_Printf("You must set 'rconpassword' before\n"
			"issuing an rcon command.\n");
		return;
	}

	message[0] = -1;
	message[1] = -1;
	message[2] = -1;
	message[3] = -1;
	message[4] = 0;

	Q_strcat(message, MAX_RCON_MESSAGE, "rcon ");

	Q_strcat(message, MAX_RCON_MESSAGE, rcon_client_password->string);
	Q_strcat(message, MAX_RCON_MESSAGE, " ");

	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
	Q_strcat(message, MAX_RCON_MESSAGE, Cmd_Cmd() + 5);

	if (cls.state >= CA_CONNECTED) {
		rcon_address = clc.netchan.remoteAddress;
	} else {
		if (!strlen(rconAddress->string)) {
			Com_Printf("You must either be connected,\n"
				"or set the 'rconAddress' cvar\n"
				"to issue rcon commands\n");

			return;
		}
		NET_StringToAdr(rconAddress->string, &rcon_address);
		if (rcon_address.port == 0) {
			rcon_address.port = BigShort(PORT_SERVER);
		}
	}

	NET_SendPacket(NS_CLIENT, (int)strlen(message) + 1, message, rcon_address);
}

/*
============
CL_Silent_f
============
*/
void CL_Silent_f(void)
{
	Com_BeginRedirect(NULL, 0, NULL, qtrue);

	Cmd_DropArg(0);
	Cmd_Execute();

	Com_EndRedirect();
}

/*
==================
CL_CompleteRedirect
==================
*/
static void CL_CompleteRedirect( char *args, int argNum )
{
	// skip first command
	char *p = Com_SkipTokens( args, 1, " " );

	if( p > args )
		Field_CompleteCommand( p, qtrue, qtrue, qtrue );
}

/*
==================
CL_CompleteDemoName
==================
*/
static void CL_CompleteDemoName( char *args, int argNum )
{
	if( argNum == 2 )
		Field_CompleteFilename( "demos", ".dm_15|.dm_16", qfalse );
}

/*
==================
CL_CompleteModelName
==================
*/
static void CL_CompleteModelName( char *args, int argNum )
{
	if( argNum == 2 )
		Field_CompleteModelname();
}

/*
=================
CL_SendPureChecksums
=================
*/
void CL_SendPureChecksums( void ) {
	const char *pChecksums;
	char cMsg[MAX_INFO_VALUE];
	int i;

	// if we are pure we need to send back a command with our referenced pk3 checksums
	pChecksums = FS_ReferencedPakPureChecksums();

	// "cp"
	// "Yf"
	Com_sprintf(cMsg, sizeof(cMsg), "Yf ");
	Q_strcat(cMsg, sizeof(cMsg), pChecksums);
	for (i = 0; i < 2; i++) {
		cMsg[i] += 10;
	}
	CL_AddReliableCommand( cMsg );
}

/*
=================
CL_ResetPureClientAtServer
=================
*/
void CL_ResetPureClientAtServer( void ) {
	CL_AddReliableCommand( "vdr" );
}

/*
=================
CL_Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/
void CL_Vid_Restart_f( void ) {

	// Settings may have changed so stop recording now
	CL_CloseAVI( );
	// don't let them loop during the restart
	S_StopAllSounds();
	// shutdown the UI
	CL_ShutdownUI();
	// shutdown the CGame
	CL_ShutdownCGame();
	// shutdown the renderer and clear the renderer interface
	CL_ShutdownRef();
	// client is no longer pure untill new checksums are sent
	CL_ResetPureClientAtServer();
	// clear pak references
	FS_ClearPakReferences( FS_UI_REF | FS_CGAME_REF );
	// reinitialize the filesystem if the game directory or checksum has changed
	FS_ConditionalRestart( clc.checksumFeed );

	cls.rendererStarted = qfalse;
	cls.uiStarted = qfalse;
	cls.cgameStarted = qfalse;
	cls.soundRegistered = qfalse;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set( "cl_paused", "0" );

	// if not running a server clear the whole hunk
	if ( !com_sv_running->integer ) {
		CM_ClearMap();
		// clear the whole hunk
		Hunk_Clear();
	}
	else {
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	// initialize the renderer interface
	CL_InitRef();

	// startup all the client stuff
	CL_StartHunkUsers();

	// start the cgame if connected
	if ( cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC ) {
		cls.cgameStarted = qtrue;
		CL_InitCGame();
		// send pure checksums
		CL_SendPureChecksums();
	}
}

/*
=================
CL_Fs_Restart_f

Restart the filesystem
=================
*/

void CL_FS_Restart_f( void ) {
	FS_Restart( clc.checksumFeed ); //xD
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
extern void S_UnCacheDynamicMusic( void );
void CL_Snd_Restart_f( void ) {
	S_Shutdown();
	S_Init();

	S_FreeAllSFXMem();
	S_UnCacheDynamicMusic();

//	CL_Vid_Restart_f();

	extern qboolean	s_soundMuted;
	s_soundMuted = qfalse;		// we can play again

	extern void S_RestartMusic( void );
	S_RestartMusic();
}


/*
==================
CL_PK3List_f
==================
*/
void CL_OpenedPK3List_f( void ) {
	Com_Printf("Opened PK3 Names: %s\n", FS_LoadedPakNames());
}

/*
==================
CL_PureList_f
==================
*/
void CL_ReferencedPK3List_f( void ) {
	Com_Printf("Referenced PK3 Names: %s\n", FS_ReferencedPakNames());
}

/*
==================
CL_Configstrings_f
==================
*/
void CL_Configstrings_f( void ) {
	int		i;
	int		ofs;

	if ( cls.state != CA_ACTIVE ) {
		Com_Printf( "Not connected to a server.\n");
		return;
	}

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		ofs = cl.gameState.stringOffsets[ i ];
		if ( !ofs ) {
			continue;
		}
		Com_Printf( "%4i: %s\n", i, cl.gameState.stringData + ofs );
	}
}

/*
==============
CL_Clientinfo_f
==============
*/
void CL_Clientinfo_f( void ) {
	Com_Printf( "--------- Client Information ---------\n" );
	Com_Printf( "state: %i\n", cls.state );
	Com_Printf( "Server: %s\n", cls.servername );
	Com_Printf ("User info settings:\n");
	Info_Print( Cvar_InfoString( CVAR_USERINFO ) );
	Com_Printf( "--------------------------------------\n" );
}


//====================================================================

/*
=================
CL_BlacklistCurrentFile
=================
*/
void CL_BlacklistCurrentFile() {
	// create a new buffer
	blacklistentry_t *entrys = (blacklistentry_t *)Z_Malloc((int)((cls.downloadBlacklistLen + 1) * sizeof(blacklistentry_t)), TAG_DOWNLOADBLACKLIST, qtrue);
	
	// in case we already had a blacklist, copy it
	if (cls.downloadBlacklist) {
		Com_Memcpy(entrys, cls.downloadBlacklist, cls.downloadBlacklistLen * sizeof(blacklistentry_t));
		Z_Free(cls.downloadBlacklist);
	}

	// write new blacklist entry to the end
	blacklistentry_t *entry = &entrys[cls.downloadBlacklistLen++];
	Q_strncpyz(entry->name, cl_downloadName->string, sizeof(entry->name));
	entry->checksum = clc.downloadChksums[clc.downloadIndex];
	entry->time = time(0);
	entry->server = clc.serverAddress;

	cls.downloadBlacklist = entrys;
}

/*
=================
CL_ReadBlacklistFile

Reads dlblacklist.dat which contains a list of files which
the user blocked permanently.
=================
*/
void CL_ReadBlacklistFile() {
	fileHandle_t fblacklist;
	int len;

	cls.downloadBlacklistLen = 0;
	len = FS_SV_FOpenFileRead("dlblacklist.dat", &fblacklist);
	if (len >= (int)sizeof(uint8_t)) {
		uint8_t version;

		FS_Read(&version, sizeof(uint8_t), fblacklist);
		if (version == BLACKLIST_FILE_VERSION) {
			int entryslen = len - sizeof(uint8_t);
			if (entryslen) {
				cls.downloadBlacklist = (blacklistentry_t *)Z_Malloc((int)entryslen, TAG_DOWNLOADBLACKLIST, qtrue);
				FS_Read(cls.downloadBlacklist, entryslen, fblacklist);
				cls.downloadBlacklistLen = entryslen / (int)sizeof(blacklistentry_t);
			}
		} else {
			Com_Printf("blacklist file version mismatch\n");
		}

		FS_FCloseFile(fblacklist);
	}
}

/*
=================
CL_BlacklistRemoveFile

Remove a file from the blacklist
=================
*/
qboolean CL_BlacklistRemoveFile(const blacklistentry_t *file) {
	int index = file - cls.downloadBlacklist;

	if (index < 0 || index >= cls.downloadBlacklistLen) {
		return qtrue;
	}

	if (index < cls.downloadBlacklistLen - 1) {
		memmove(&cls.downloadBlacklist[index], &cls.downloadBlacklist[index + 1], (cls.downloadBlacklistLen - index - 1) * sizeof(blacklistentry_t));
	}

	cls.downloadBlacklistLen--;
	return qfalse;
}

/*
=================
CL_BlacklistWriteCloseFile

Writes dlblacklist.dat which contains a list of files which
the user blocked permanently.
=================
*/
void CL_BlacklistWriteCloseFile() {
	if (cls.downloadBlacklist) {
		fileHandle_t fblacklist = FS_SV_FOpenFileWrite("dlblacklist.dat");
		uint8_t version = BLACKLIST_FILE_VERSION;

		if (fblacklist) {
			if (FS_Write(&version, sizeof(version), fblacklist)) {
				if (FS_Write(cls.downloadBlacklist, (int)(cls.downloadBlacklistLen * sizeof(blacklistentry_t)), fblacklist)) {
					Com_DPrintf("blacklist file written to dlblacklist.dat\n");
				}
			}

			FS_FCloseFile(fblacklist);
		} else {
			Com_Printf("could not write blacklist file dlblacklist.dat\n");
		}

		Z_Free(cls.downloadBlacklist);
		cls.downloadBlacklist = NULL;
		cls.downloadBlacklistLen = 0;
	}
}

/*
=================
CL_DownloadsComplete

Called when all downloading has been completed
=================
*/
void CL_DownloadsComplete( void ) {
	CL_BlacklistWriteCloseFile();

	// if we downloaded files we need to restart the file system
	if (clc.downloadRestart) {
		clc.downloadRestart = qfalse;

		FS_Restart(clc.checksumFeed); // We possibly downloaded a pak, restart the file system to load it

		// inform the server so we get new gamestate info
		CL_AddReliableCommand( "donedl" );
		cls.ignoreNextDownloadList = qtrue;

		// by sending the donenl command we request a new gamestate
		// so we don't want to load stuff yet
		return;
	}

	// let the client game init and load data
	cls.state = CA_LOADING;

	// Pump the loop, this may change gamestate!
	Com_EventLoop();

	// if the gamestate was changed by calling Com_EventLoop
	// then we loaded everything already and we don't want to do it again.
	if ( cls.state != CA_LOADING ) {
		return;
	}

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// flush client memory and start loading stuff
	// this will also (re)load the UI
	// if this is a local client then only the client part of the hunk
	// will be cleared, note that this is done after the hunk mark has been set
	CL_FlushMemory( qfalse );

	// initialize the CGame
	cls.cgameStarted = qtrue;
	CL_InitCGame();

	// set pure checksums
	CL_SendPureChecksums();

	CL_WritePacket();
	CL_WritePacket();
	CL_WritePacket();
}

/*
=================
CL_BeginDownload

Requests a file to download from the server.
The UI opens a window in which the user can decide to download it.
=================
*/
void CL_BeginDownload( const char *localName, const char *remoteName ) {
	// in case the file has been blacklisted, skip it immediately
	if (cls.downloadBlacklist) {
		for (int i = 0; i < cls.downloadBlacklistLen; i++) {
			if (cls.downloadBlacklist[i].checksum == clc.downloadChksums[clc.downloadIndex]) {
				// file is blacklisted
				Com_Printf("Skipping download for blacklisted file %s\n", remoteName);
				clc.downloadIndex++;
				CL_NextDownload();
				return;
			}
		}
	}

	// the ui decides if this file is going to be downloaded
	Cvar_Set("cl_downloadName", remoteName);
	Cvar_Set("cl_downloadLocalName", localName);
	Cvar_SetValue("cl_downloadSize", 0);
	Cvar_SetValue("cl_downloadCount", 0);
	Cvar_SetValue("cl_downloadTime", 0);

	if (clc.httpdl[0]) {
		// downloading over http has priority
		Cvar_Set("cl_downloadProtocol", "HTTP");
	} else {
		Cvar_Set("cl_downloadProtocol", "UDP");
	}

	// to get the download popup jk2mvmenu must be loaded instaed of the normal ui
	// CL_DownloadsComplete restarts the ui after the download process anyway
	CL_InitMVMenu();

	VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_MV_DOWNLOAD_POPUP);
	WIN_SetTaskbarState(TBS_NOTIFY, 0, 0);
}

/*
=================
CL_ContinueCurrentDownload

After the user decided wether the file should be downloaded.
Stores it in the current game directory.
=================
*/
void CL_ContinueCurrentDownload(dldecision_t decision) {
	if (decision == DL_ABORT) {
		// user disallowed the file
		clc.downloadIndex++;
		CL_NextDownload();
	} else if (decision == DL_ABORT_BLACKLIST) {
		// user disallowed the file and never wants to be asked again
		Com_DPrintf("Blacklisted file with checksum %i", clc.downloadChksums[clc.downloadIndex]);
		CL_BlacklistCurrentFile();

		clc.downloadIndex++;
		CL_NextDownload();
	} else {
		// user accepted the file
		clc.downloadIndex++;

		Com_Printf("^1****** ^7File Download ^1******^7\n"
			"Localname: %s\n"
			"Remotename: %s\n"
			"Protocol: %s\n"
			"^1***************************\n", cl_downloadLocalName->string, cl_downloadName->string, cl_downloadProtocol->string);

		// Set so UI gets access to it
		Cvar_SetValue("cl_downloadTime", (float)cls.realtime);
		Q_strncpyz(clc.downloadName, cl_downloadLocalName->string, sizeof(clc.downloadName));
		Com_sprintf(clc.downloadTempName, sizeof(clc.downloadTempName), "%s.tmp", cl_downloadLocalName->string);

		if (!Q_stricmp(cl_downloadProtocol->string, "HTTP")) {
			char remotepath[MAX_STRING_CHARS];

			Com_sprintf(remotepath, sizeof(remotepath), "%s/%s", clc.httpdl, cl_downloadName->string);
			Com_DPrintf("HTTP URL: %s\n", remotepath);

			char *tmp_os_path = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), clc.downloadTempName);
			
			clc.httpHandle = NET_HTTP_StartDownload(remotepath, tmp_os_path, CL_EndHTTPDownload, CL_ProcessHTTPDownload, Q3_VERSION, va("jk2://%s", NET_AdrToString(clc.serverAddress)));
		} else {
			clc.downloadBlock = 0; // Starting new file
			clc.downloadCount = 0;

			CL_AddReliableCommand(va("download %s", cl_downloadName->string));
		}
	}
}

/*
=================
CL_NextDownload

A download completed or failed
=================
*/
void CL_NextDownload(void) {
	char *s;
	char *remoteName, *localName;
	char remoteNameCpy[MAX_QPATH], localNameCpy[MAX_QPATH];

	// We are looking to start a download here
	if (*clc.downloadList) {
		s = clc.downloadList;

		// format is:
		//  @remotename@localname@remotename@localname, etc.

		if (*s == '@')
			s++;
		remoteName = s;

		if ( (s = strchr(s, '@')) == NULL ) {
			CL_DownloadsComplete();
			return;
		}

		*s++ = 0;
		localName = s;
		if ( (s = strchr(s, '@')) != NULL )
			*s++ = 0;
		else
			s = localName + strlen(localName); // point at the nul byte

		clc.downloadRestart = qtrue;

		Q_strncpyz(remoteNameCpy, remoteName, sizeof(remoteNameCpy));

		if (Cvar_VariableIntegerValue("fs_globalcfg") && Q_stricmpn(localName, "base/", 4)) {
			Com_sprintf(localNameCpy, sizeof(localNameCpy), "base/%s", Q_strchrs(localName, "/")+1);
		}
		else {
			Q_strncpyz(localNameCpy, localName, sizeof(localNameCpy));
		}

		// move over the rest
		memmove( clc.downloadList, s, strlen(s) + 1);

		CL_BeginDownload(localNameCpy, remoteNameCpy);

		return;
	}

	CL_DownloadsComplete();
}

/*
=================
CL_InitDownloads

After receiving a valid game state, we valid the cgame and local zip files here
and determine if we need to download them
=================
*/
void CL_InitDownloads(void) {
	char missingfiles[1024];

	clc.downloadIndex = 0;

	if (cls.ignoreNextDownloadList) {
	  cls.ignoreNextDownloadList = qfalse;
	} else if ( !mv_allowDownload->integer ) {
		// autodownload is disabled on the client
		// but it's possible that some referenced files on the server are missing
		if (FS_ComparePaks( missingfiles, sizeof( missingfiles ), NULL, 0, qfalse ) ) {
			// NOTE TTimo I would rather have that printed as a modal message box
			//   but at this point while joining the game we don't know wether we will successfully join or not
			Com_Printf( "\nWARNING: You are missing some files referenced by the server:\n%s"
				"You might not be able to join the game\n"
				"Go to the setting menu to turn on autodownload, or get the file elsewhere\n\n", missingfiles );
		}
	} else if ( FS_ComparePaks( clc.downloadList, sizeof(clc.downloadList), clc.downloadChksums, sizeof(clc.downloadChksums) / sizeof(int), qtrue ) ) {
		Com_Printf("Need paks: %s\n", clc.downloadList );

		CL_ReadBlacklistFile();

		if (*clc.downloadList && (clc.udpdl || clc.httpdl[0])) {
			cls.state = CA_CONNECTED;

			CL_NextDownload();
			return;
		}

	}

	CL_DownloadsComplete();
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend( void ) {
	int		port;
	char	info[MAX_INFO_STRING];
	char	data[MAX_INFO_STRING + 10];

	// don't send anything if playing back a demo
	if ( clc.demoplaying ) {
		return;
	}

	// resend if we haven't gotten a reply yet
	if ( cls.state != CA_CONNECTING && cls.state != CA_CHALLENGING ) {
		return;
	}

	if ( cls.realtime - clc.connectTime < RETRANSMIT_TIMEOUT ) {
		return;
	}

	clc.connectTime = cls.realtime;	// for retransmit requests
	clc.connectPacketCount++;


	switch ( cls.state ) {
	case CA_CONNECTING:
		// requesting a challenge
		clc.httpdl[0] = 0;
		clc.httpdlvalid = qfalse;
		clc.udpdl = -1;
#ifdef MV_MFDOWNLOADS
		NET_OutOfBandPrint(NS_CLIENT, clc.serverAddress, "jk2mfport");
#endif
		NET_OutOfBandPrint(NS_CLIENT, clc.serverAddress, "getinfo"); // for mvhttp
		NET_OutOfBandPrint(NS_CLIENT, clc.serverAddress, "getstatus"); // for sv_allowdownload
		NET_OutOfBandPrint(NS_CLIENT, clc.serverAddress, "getchallenge");
		break;

	case CA_CHALLENGING:
		if (MV_GetCurrentGameversion() == VERSION_UNDEF || ( ( !clc.httpdlvalid || clc.udpdl == -1 ) && com_dedicated->integer) )
			break;

		// sending back the challenge
		port = (int) Cvar_VariableValue ("net_qport");

		Q_strncpyz( info, Cvar_InfoString( CVAR_USERINFO ), sizeof( info ) );
		Info_SetValueForKey( info, "protocol", va("%i", MV_GetCurrentProtocol() ) );
		Info_SetValueForKey( info, "qport", va("%i", port ) );
		Info_SetValueForKey( info, "challenge", va("%i", clc.challenge ) );
		Com_sprintf(data, sizeof(data), "connect \"%s\"", info );
		NET_OutOfBandData( NS_CLIENT, clc.serverAddress, (unsigned char *)data, (int)strlen(data) );

		// the most current userinfo has been sent, so watch for any
		// newer changes to userinfo variables
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		break;

	default:
		Com_Error( ERR_FATAL, "CL_CheckForResend: bad cls.state" );
	}
}


/*
===================
CL_DisconnectPacket

Sometimes the server can drop the client and the netchan based
disconnect can be lost.  If the client continues to send packets
to the server, the server will send out of band disconnect packets
to the client so it doesn't have to wait for the full timeout period.
===================
*/
void CL_DisconnectPacket( netadr_t from ) {
	if ( cls.state < CA_AUTHORIZING ) {
		return;
	}

	// if not from our server, ignore it
	if ( !NET_CompareAdr( from, clc.netchan.remoteAddress ) ) {
		return;
	}

	// if we have received packets within three seconds, ignore it
	// (it might be a malicious spoof)
	if ( cls.realtime - clc.lastPacketTime < 3000 ) {
		return;
	}

	// drop the connection (FIXME: connection dropped dialog)
	Com_Printf( "Server disconnected for unknown reason\n" );
	CL_Disconnect( qtrue );
}


/*
===================
CL_MotdPacket

===================
*/
void CL_MotdPacket( netadr_t from ) {
	char	*challenge;
	char	*info;

	// if not from our server, ignore it
	if ( !NET_CompareAdr( from, cls.updateServer ) ) {
		return;
	}

	info = Cmd_Argv(1);

	// check challenge
	challenge = Info_ValueForKey( info, "challenge" );
	if ( strcmp( challenge, cls.updateChallenge ) ) {
		return;
	}

	challenge = Info_ValueForKey( info, "motd" );

	Q_strncpyz( cls.updateInfoString, info, sizeof( cls.updateInfoString ) );
	Cvar_Set( "cl_motdString", challenge );
}

/*
===================
CL_InitServerInfo
===================
*/
void CL_InitServerInfo( serverInfo_t *server, serverAddress_t *address ) {
	server->adr.type  = NA_IP;
	server->adr.ip[0] = address->ip[0];
	server->adr.ip[1] = address->ip[1];
	server->adr.ip[2] = address->ip[2];
	server->adr.ip[3] = address->ip[3];
	server->adr.port  = address->port;
	server->clients = 0;
	server->bots = 0; // botfilter
	server->hostName[0] = '\0';
	server->mapName[0] = '\0';
	server->maxClients = 0;
	server->maxPing = 0;
	server->minPing = 0;
	server->ping = -1;
	server->game[0] = '\0';
	server->gameType = 0;
	server->netType = 0;
	server->needPassword = qfalse;
	server->trueJedi = 0;
	server->weaponDisable = 0;
	server->forceDisable = 0;
	//server->pure = qfalse;
	server->gameVersion = VERSION_UNDEF; // For 1.03 in the menu...
}

#define MAX_SERVERSPERPACKET	256

// multimaster
serverInfo_t *IsAlreadyInGlobalServerList(serverAddress_t *addr) {
	int j;

	for (j = 0; j < cls.numglobalservers && j < MAX_GLOBAL_SERVERS; j++) {
		if (cls.globalServers[j].adr.ip[0] == addr->ip[0] && cls.globalServers[j].adr.ip[1] == addr->ip[1] &&
			cls.globalServers[j].adr.ip[2] == addr->ip[2] &&
			cls.globalServers[j].adr.ip[3] == addr->ip[3] &&
			cls.globalServers[j].adr.port == addr->port) {
			return &cls.globalServers[j];
		}
	}

	return NULL;
}

/*
===================
CL_ServersResponsePacket
===================
*/
void CL_ServersResponsePacket( netadr_t from, msg_t *msg ) {
	int				i, count, max, total;
	serverAddress_t addresses[MAX_SERVERSPERPACKET];
	int				numservers;
	byte*			buffptr;
	byte*			buffend;

	Com_Printf("CL_ServersResponsePacket\n");

	if (cls.numglobalservers == -1) {
		// state to detect lack of servers or lack of response
		cls.numglobalservers = 0;
		cls.numGlobalServerAddresses = 0;
	}

	if (cls.nummplayerservers == -1) {
		cls.nummplayerservers = 0;
	}

	// parse through server response string
	numservers = 0;
	buffptr    = msg->data;
	buffend    = buffptr + msg->cursize;
	while (buffptr+1 < buffend) {
		// advance to initial token
		do {
			if (*buffptr++ == '\\')
				break;
		}
		while (buffptr < buffend);

		if ( buffptr >= buffend - 6 ) {
			break;
		}

		// parse out ip
		addresses[numservers].ip[0] = *buffptr++;
		addresses[numservers].ip[1] = *buffptr++;
		addresses[numservers].ip[2] = *buffptr++;
		addresses[numservers].ip[3] = *buffptr++;

		// parse out port
		addresses[numservers].port = (unsigned short) ((*buffptr++)<<8);
		addresses[numservers].port += (unsigned short) (*buffptr++);
		addresses[numservers].port = BigShort( addresses[numservers].port );

		// syntax check
		if (*buffptr != '\\') {
			break;
		}

		Com_DPrintf( "server: %d ip: %d.%d.%d.%d:%d\n",numservers,
				addresses[numservers].ip[0],
				addresses[numservers].ip[1],
				addresses[numservers].ip[2],
				addresses[numservers].ip[3],
				addresses[numservers].port );

		numservers++;
		if (numservers >= MAX_SERVERSPERPACKET) {
			break;
		}

		// parse out EOT
		if (buffptr[1] == 'E' && buffptr[2] == 'O' && buffptr[3] == 'T') {
			break;
		}
	}

	if (cls.masterNum == 0) {
		count = cls.numglobalservers;
		max = MAX_GLOBAL_SERVERS;
	} else {
		count = cls.nummplayerservers;
		max = MAX_OTHER_SERVERS;
	}

	for (i = 0; i < numservers && count < max; i++) {
		serverInfo_t *server;

		// multimaster
		server = IsAlreadyInGlobalServerList(&addresses[i]);
		if (cls.masterNum != 0 || !server) {
			server = (cls.masterNum == 0) ? &cls.globalServers[count] : &cls.mplayerServers[count];
			CL_InitServerInfo(server, &addresses[i]);
			count++;
		}
	}

	// if getting the global list
	if (cls.masterNum == 0) {
		if ( cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS ) {
			// if we couldn't store the servers in the main list anymore
			for (; i < numservers && count >= max; i++) {
				serverAddress_t *addr;

				// multimaster
				if (cls.masterNum == 0 && IsAlreadyInGlobalServerList(&addresses[i])) {
					continue;
				}

				// just store the addresses in an additional list
				addr = &cls.globalServerAddresses[cls.numGlobalServerAddresses++];
				addr->ip[0] = addresses[i].ip[0];
				addr->ip[1] = addresses[i].ip[1];
				addr->ip[2] = addresses[i].ip[2];
				addr->ip[3] = addresses[i].ip[3];
				addr->port  = addresses[i].port;
			}
		}
	}

	if (cls.masterNum == 0) {
		cls.numglobalservers = count;
		total = count + cls.numGlobalServerAddresses;
	} else {
		cls.nummplayerservers = count;
		total = count;
	}

	Com_Printf("%d servers parsed (total %d)\n", numservers, total);
}

#ifndef MAX_STRIPED_SV_STRING
#define MAX_STRIPED_SV_STRING 1024
#endif
static void CL_CheckSVStripEdRef(char *buf, const char *str)
{ //I don't really like doing this. But it utilizes the system that was already in place.
	int i = 0;
	int b = 0;
	int strLen = 0;
	qboolean gotStrip = qfalse;

	if (!str || !str[0])
	{
		if (str)
		{
			strcpy(buf, str);
		}
		return;
	}

	strcpy(buf, str);

	strLen = (int)strlen(str);

	if (strLen >= MAX_STRIPED_SV_STRING)
	{
		return;
	}

	while (i < strLen && str[i])
	{
		gotStrip = qfalse;

		if (str[i] == '@' && (i+1) < strLen)
		{
			if (str[i+1] == '@' && (i+2) < strLen)
			{
				if (str[i+2] == '@' && (i+3) < strLen)
				{ //@@@ should mean to insert a striped reference here, so insert it into buf at the current place
					char stripRef[MAX_STRIPED_SV_STRING];
					int r = 0;

					while (i < strLen && str[i] == '@')
					{
						i++;
					}

					while (i < strLen && str[i] && str[i] != ' ' && str[i] != ':' && str[i] != '.' && str[i] != '\n')
					{
						stripRef[r] = str[i];
						r++;
						i++;
					}
					stripRef[r] = 0;

					buf[b] = 0;
					Q_strcat(buf, MAX_STRIPED_SV_STRING, SP_GetStringTextString(va("SVINGAME_%s", stripRef)));
					b = (int)strlen(buf);
				}
			}
		}

		if (!gotStrip)
		{
			buf[b] = str[i];
			b++;
		}
		i++;
	}

	buf[b] = 0;
}


/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket( netadr_t from, msg_t *msg ) {
	char	*s;
	char	*c;
	int challenge = 0;

	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );	// skip the -1

	s = MSG_ReadStringLine( msg );

	Cmd_TokenizeString( s );

	c = Cmd_Argv(0);

	Com_DPrintf ("CL packet %s: %s\n", NET_AdrToString(from), c);

	// challenge from the server we are connecting to
	if ( !Q_stricmp(c, "challengeResponse") ) {
		if (cls.state != CA_CONNECTING) {
			Com_DPrintf("Unwanted challenge response received.  Ignored.\n");
			return;
		}

		c = Cmd_Argv(2);
		if (*c)
			challenge = atoi(c);

		if (!NET_CompareAdr(from, clc.serverAddress)) {
			// This challenge response is not coming from the expected address.
			// Check whether we have a matching client challenge to prevent
			// connection hi-jacking.

			if (!*c || challenge != clc.challenge) {
				Com_DPrintf("Challenge response received from unexpected source. Ignored.\n");
				return;
			}
		}

		// start sending challenge response instead of challenge request packets
		clc.challenge = atoi(Cmd_Argv(1));
		cls.state = CA_CHALLENGING;
		clc.connectPacketCount = 0;
		clc.connectTime = -99999;

		// take this address as the new server address.  This allows
		// a server proxy to hand off connections to multiple servers
		clc.serverAddress = from;
		Com_DPrintf("challengeResponse: %d\n", clc.challenge);
		return;
	}

	// server connection
	if ( !Q_stricmp(c, "connectResponse") ) {
		if ( cls.state >= CA_CONNECTED ) {
			Com_DPrintf ("Dup connect received.  Ignored.\n");
			return;
		}
		if ( cls.state != CA_CHALLENGING ) {
			Com_DPrintf ("connectResponse packet while not connecting. Ignored.\n");
			return;
		}
		if ( !NET_CompareAdr( from, clc.serverAddress ) ) {
			Com_DPrintf( "connectResponse from a different address.  Ignored.\n" );
			Com_DPrintf( "%s should have been %s\n", NET_AdrToString( from ),
				NET_AdrToString( clc.serverAddress ) );
			return;
		}
		Netchan_Setup (NS_CLIENT, &clc.netchan, from, Cvar_VariableValue( "net_qport" ) );
		cls.state = CA_CONNECTED;
		clc.lastPacketSentTime = -9999;		// send first packet immediately
		return;
	}

	// server responding to an info broadcast
	if ( !Q_stricmp(c, "infoResponse") ) {
		CL_ServerInfoPacket( from, msg );
		return;
	}

	// server responding to a get playerlist
	if ( !Q_stricmp(c, "statusResponse") ) {
		CL_ServerStatusResponse( from, msg );
		return;
	}

	// a disconnect message from the server, which will happen if the server
	// dropped the connection but it is still getting packets from us
	if (!Q_stricmp(c, "disconnect")) {
		CL_DisconnectPacket( from );
		return;
	}

	// echo request from server
	if ( !Q_stricmp(c, "echo") ) {
		NET_OutOfBandPrint( NS_CLIENT, from, "%s", Cmd_Argv(1) );
		return;
	}

	// global MOTD from id
	if ( !Q_stricmp(c, "motd") ) {
		CL_MotdPacket( from );
		return;
	}

	// echo request from server
	if ( !Q_stricmp(c, "print") )
	{
		if (NET_CompareAdr(from, clc.serverAddress) || NET_CompareAdr(from, rcon_address)) {
			char sTemp[MAX_STRIPED_SV_STRING];

			s = MSG_ReadString( msg );
			CL_CheckSVStripEdRef(sTemp, s);
			Q_strncpyz( clc.serverMessage, sTemp, sizeof( clc.serverMessage ) );
			Com_Printf( "%s", sTemp );
		}
		return;
	}

	// echo request from server
//	if ( !Q_stricmp(c, "getserversResponse\\") ) {
	if ( !Q_strncmp(c, "getserversResponse", 18) ) {
		CL_ServersResponsePacket( from, msg );
		return;
	}

#ifdef MV_MFDOWNLOADS
	if (strtol(c, NULL, 10) && cls.state == CA_CONNECTING && NET_CompareAdr(from, clc.serverAddress)) {
		Com_sprintf(clc.httpdl, sizeof(clc.httpdl), "http://%i.%i.%i.%i:%s",
			clc.serverAddress.ip[0], clc.serverAddress.ip[1],
			clc.serverAddress.ip[2], clc.serverAddress.ip[3], c);

		return;
	}
#endif

	Com_DPrintf ("Unknown connectionless packet command.\n");
}


/*
=================
CL_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CL_PacketEvent( netadr_t from, msg_t *msg ) {
	int		headerBytes;

	clc.lastPacketTime = cls.realtime;

	if ( msg->cursize >= 4 && *(int *)msg->data == -1 ) {
		CL_ConnectionlessPacket( from, msg );
		return;
	}

	if ( cls.state < CA_CONNECTED ) {
		return;		// can't be a valid sequenced packet
	}

	if ( msg->cursize < 4 ) {
		Com_Printf ("%s: Runt packet\n",NET_AdrToString( from ));
		return;
	}

	//
	// packet from server
	//
	if ( !NET_CompareAdr( from, clc.netchan.remoteAddress ) ) {
		Com_DPrintf ("%s:sequenced packet without connection\n"
			,NET_AdrToString( from ) );
		// FIXME: send a client disconnect?
		return;
	}

	if (!CL_Netchan_Process( &clc.netchan, msg) ) {
		return;		// out of order, duplicated, etc
	}

	// the header is different lengths for reliable and unreliable messages
	headerBytes = msg->readcount;

	// track the last message received so it can be returned in
	// client messages, allowing the server to detect a dropped
	// gamestate
	clc.serverMessageSequence = LittleLong( *(int *)msg->data );

	clc.lastPacketTime = cls.realtime;
	CL_ParseServerMessage( msg );

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if ( clc.demorecording && !clc.demowaiting ) {
		CL_WriteDemoMessage( msg, headerBytes );
	}
}

/*
==================
CL_CheckTimeout

==================
*/
void CL_CheckTimeout( void ) {
	//
	// check timeout
	//
	if ( ( !cl_paused->integer || !sv_paused->integer )
		&& cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC
	    && cls.realtime - clc.lastPacketTime > cl_timeout->value*1000) {
		if (++cl.timeoutcount > 5) {	// timeoutcount saves debugger
			const char *psTimedOut = SP_GetStringTextString("SVINGAME_SERVER_CONNECTION_TIMED_OUT");
			Com_Printf ("\n%s\n",psTimedOut);
			Com_Error(ERR_DROP, "%s", psTimedOut);
			//CL_Disconnect( qtrue );
			return;
		}
	} else {
		cl.timeoutcount = 0;
	}
}


//============================================================================

/*
==================
CL_CheckUserinfo

==================
*/
void CL_CheckUserinfo( void ) {
	// don't add reliable commands when not yet connected
	if ( cls.state < CA_CHALLENGING ) {
		return;
	}
	// don't overflow the reliable command buffer when paused
	if ( cl_paused->integer ) {
		return;
	}
	// send a reliable userinfo update if needed
	if ( cvar_modifiedFlags & CVAR_USERINFO ) {
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		CL_AddReliableCommand( va("userinfo \"%s\"", Cvar_InfoString( CVAR_USERINFO ) ) );
	}

}

#ifdef G2_COLLISION_ENABLED
extern CMiniHeap *G2VertSpaceServer;
#endif

extern cvar_t	*con_notifywords;
#define			MAX_NOTIFYWORDS 8
char			notifyWords[MAX_NOTIFYWORDS][32];

static void CL_AddNotificationName(char *str) {
	int i;

	//Com_Printf("Adding %s\n", str);
	for (i = 0; i<MAX_NOTIFYWORDS; i++) {
		//Com_Printf("Slot is %s", notifyWords[i]);
		if (!strcmp(notifyWords[i], "")) {
			//Com_Printf("Copying to %i\n", i);
			Q_strncpyz(notifyWords[i], str, sizeof(notifyWords[i]));
			return;
		}
	}
	//Error, max words
}

static void CL_UpdateNotificationWords(void) {
	char * pch;
	char words[MAX_CVAR_VALUE_STRING];

	Q_strncpyz(words, con_notifywords->string, sizeof(words));
	memset(notifyWords, 0, sizeof(notifyWords));
	pch = strtok(words, " ");
	while (pch != NULL) {
		CL_AddNotificationName(pch);
		pch = strtok(NULL, " ");
	}
}

static int lastModifiedColors = 0;
static int lastModifiedNotifyName = 0;
static void CL_CheckCvarUpdate(void) {
	if (lastModifiedColors != cl_colorString->modificationCount) {
		// recalculate cl_colorStringCount
		lastModifiedColors = cl_colorString->modificationCount;
		int count = cl_colorString->integer;
		count = count - ((count >> 1) & 0x55555555);
		count = (count & 0x33333333) + ((count >> 2) & 0x33333333);
		count = (((count + (count >> 4)) & 0x0f0f0f0f) * 0x01010101) >> 24;
		Cvar_Set("cl_colorStringCount", va("%i", count));
	}

	if (lastModifiedNotifyName != con_notifywords->modificationCount) {
		lastModifiedNotifyName = con_notifywords->modificationCount;
		CL_UpdateNotificationWords();
	}
}

/*
==================
CL_Frame

==================
*/
static unsigned int frameCount;
static float avgFrametime=0.0;
extern void SP_CheckForLanguageUpdates(void);
void CL_Frame ( int msec ) {
	qboolean render = qfalse;

	if ( !com_cl_running->integer ) {
		return;
	}

	if ((com_renderfps->integer <= 0) || ((cls.realtime >= cls.lastDrawTime + (1000 / com_renderfps->integer)))) {
		render = qtrue;
		cls.lastDrawTime = cls.realtime;
	}

	SP_CheckForLanguageUpdates();	// will take zero time to execute unless language changes, then will reload strings.
									//	of course this still doesn't work for menus...

	if ( cls.state == CA_DISCONNECTED && !( cls.keyCatchers & KEYCATCH_UI )
		&& !com_sv_running->integer ) {
		// if disconnected, bring up the menu
		S_StopAllSounds();
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
	}

	// if recording an avi, lock to a fixed fps
	if ( CL_VideoRecording() && cl_aviFrameRate->integer && msec) {
		// save the current screen
		if ( cls.state == CA_ACTIVE || cl_forceavidemo->integer) {
			static double	overflow = 0.0;
			double			frameTime;

			CL_TakeVideoFrame();

			frameTime = (1000.0 / abs(cl_aviFrameRate->integer)) * com_timescale->value;
			frameTime += overflow;

			msec = floor(frameTime);
			if (msec == 0)
				msec = 1;

			overflow = frameTime - msec;
		}
	}

	if (cl_autoDemo->integer && !clc.demoplaying) {
		if (cls.state != CA_ACTIVE && clc.demorecording)
			demoAutoComplete();
		else if (cls.state == CA_ACTIVE && !clc.demorecording)
			demoAutoRecord();
	}

	// save the msec before checking pause
	cls.realFrametime = msec;

	// decide the simulation time
	cls.frametime = msec;
	if(cl_framerate->integer)
	{
		avgFrametime+=msec;
		char mess[256];
		if(!(frameCount&0x1f))
		{
			sprintf(mess,"Frame rate=%f\n\n",1000.0f*(1.0/(avgFrametime/32.0f)));
	//		OutputDebugString(mess);
			Com_Printf("%s", mess);
			avgFrametime=0.0f;
		}
		frameCount++;
	}

	cls.realtime += cls.frametime;

#ifdef _DONETPROFILE_
	if(cls.state==CA_ACTIVE)
	{
		ClReadProf().IncTime(cls.frametime);
	}
#endif

	if ( cl_timegraph->integer ) {
		SCR_DebugGraph ( cls.realFrametime * 0.25, 0 );
	}

	CL_CheckCvarUpdate();

	// see if we need to update any userinfo
	CL_CheckUserinfo();

	// if we haven't gotten a packet in a long time,
	// drop the connection
	CL_CheckTimeout();

	// send intentions now
	CL_SendCmd();

	// resend a connection request if necessary
	CL_CheckForResend();

	// decide on the serverTime to render
	CL_SetCGameTime();

	// update the screen
	if (render) {
		SCR_UpdateScreen();

		// update audio
		S_Update();
	}

	// advance local effects for next frame
	SCR_RunCinematic();

	Con_RunConsole();
#ifdef G2_COLLISION_ENABLED
	// reset the heap for Ghoul2 vert transform space gameside
	if (G2VertSpaceServer)
	{
		G2VertSpaceServer->ResetHeap();
	}
#endif

	cls.framecount++;
}


//============================================================================

/*
================
CL_RefPrintf

DLL glue
================
*/
 __attribute__ ((format (printf, 2, 3)))
void QDECL CL_RefPrintf( int print_level, const char *fmt, ...) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	if ( print_level == PRINT_ALL ) {
		Com_Printf ("%s", msg);
	} else if ( print_level == PRINT_WARNING ) {
		Com_Printf (S_COLOR_YELLOW "%s", msg);		// yellow
	} else if ( print_level == PRINT_DEVELOPER ) {
		Com_DPrintf (S_COLOR_RED "%s", msg);		// red
	}
}



/*
============
CL_ShutdownRef
============
*/
void CL_ShutdownRef( void ) {
	if ( !re.Shutdown ) {
		return;
	}
	re.Shutdown( qtrue );
	Com_Memset( &re, 0, sizeof( re ) );
}

/*
============
CL_InitRenderer
============
*/
void CL_InitRenderer( void ) {
	// this sets up the renderer and calls R_Init
	re.BeginRegistration( &cls.glconfig );

	// load character sets
#ifdef _JK2
	cls.charSetShader = re.RegisterShaderNoMip("gfx/2d/charsgrid_med");
#else
	cls.charSetShader = re.RegisterShaderNoMip( "gfx/2d/bigchars" );
#endif

	cls.whiteShader = re.RegisterShader( "white" );
	cls.consoleShader = re.RegisterShader( "console" );
	cls.recordingShader = re.RegisterShaderNoMip("gfx/2d/demorec");
	cls.xadjust = (float) SCREEN_WIDTH / cls.glconfig.vidWidth;
	cls.yadjust = (float) SCREEN_HEIGHT / cls.glconfig.vidHeight;

	cls.cgxadj = 1;
	cls.cgyadj = 1;
	cls.uixadj = 1;
	cls.uiyadj = 1;

	kg.yankIndex = -1;
}

/*
============================
CL_StartHunkUsers

After the server has cleared the hunk, these will need to be restarted
This is the only place that any of these functions are called from
============================
*/
void CL_StartHunkUsers( void ) {
	if (!com_cl_running) {
		return;
	}

	if ( !com_cl_running->integer ) {
		return;
	}

	if ( !cls.rendererStarted ) {
		cls.rendererStarted = qtrue;
		CL_InitRenderer();
	}

	if ( !cls.soundStarted ) {
		cls.soundStarted = qtrue;
		S_Init();
	}

	if ( !cls.soundRegistered ) {
		cls.soundRegistered = qtrue;
		S_BeginRegistration();
	}

	if ( !cls.uiStarted ) {
		CL_InitUI(MV_GetCurrentGameversion() == VERSION_UNDEF ? qtrue : qfalse);
	}
}

/*
============
CL_RefMalloc
============
*/
void *CL_RefMalloc( int size ) {
	return Z_Malloc( size, TAG_RENDERER, qtrue );
}

int CL_ScaledMilliseconds(void) {
	return Sys_Milliseconds() * (double)com_timescale->value;
}

/*
============
CL_InitRef
============
*/
void CL_InitRef( void ) {
	refimport_t	ri;
	refexport_t	*ret;

	Com_Printf( "----- Initializing Renderer ----\n" );

	ri.Cmd_AddCommand = Cmd_AddCommand;
	ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
	ri.Cmd_Argc = Cmd_Argc;
	ri.Cmd_Argv = Cmd_Argv;
	ri.Cmd_ExecuteText = Cbuf_ExecuteText;
	ri.Cmd_ArgsBuffer = Cmd_ArgsBuffer;
	ri.Printf = CL_RefPrintf;
	ri.Error = Com_Error;
	ri.Milliseconds = CL_ScaledMilliseconds;
	ri.Malloc = Z_Malloc;//CL_RefMalloc;
	ri.Free = Z_Free;
#ifdef HUNK_DEBUG
	ri.Hunk_AllocDebug = Hunk_AllocDebug;
#else
	ri.Hunk_Alloc = Hunk_Alloc;
#endif
	ri.Hunk_AllocateTempMemory = Hunk_AllocateTempMemory;
	ri.Hunk_FreeTempMemory = Hunk_FreeTempMemory;
	ri.CM_DrawDebugSurface = CM_DrawDebugSurface;
	ri.FS_ReadFile = FS_ReadFile;
	ri.FS_FreeFile = FS_FreeFile;
	ri.FS_WriteFile = FS_WriteFile;
	ri.FS_FreeFileList = FS_FreeFileList;
	ri.FS_ListFiles = FS_ListFiles;
	ri.FS_FileIsInPAK = FS_FileIsInPAK;
	ri.FS_FileExists = FS_FileExists;
	ri.Cvar_Get = Cvar_Get;
	ri.Cvar_Set = Cvar_Set;
	ri.Cvar_SetValue = Cvar_SetValue;

	// cinematic stuff

	ri.CIN_UploadCinematic = CIN_UploadCinematic;
	ri.CIN_PlayCinematic = CIN_PlayCinematic;
	ri.CIN_RunCinematic = CIN_RunCinematic;

	ri.CL_WriteAVIVideoFrame = CL_WriteAVIVideoFrame;

	ri.CM_PointContents = CM_PointContents;

	ret = GetRefAPI( REF_API_VERSION, &ri );

#if defined __USEA3D && defined __A3D_GEOM
	hA3Dg_ExportRenderGeom (ret);
#endif

	Com_Printf( "-------------------------------\n");

	if ( !ret ) {
		Com_Error (ERR_FATAL, "Couldn't initialize refresh" );
	}

	re = *ret;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set( "cl_paused", "0" );
}


//===========================================================================================

void CL_SetModel_f( void ) {
	char	*arg;
	char	name[256];

	arg = Cmd_Argv( 1 );
	if (arg[0]) {
		Cvar_Set( "model", arg );
		Cvar_Set( "team_model", arg );
	} else {
		Cvar_VariableStringBuffer( "model", name, sizeof(name) );
		Com_Printf("model is set to %s\n", name);
		//Cvar_VariableStringBuffer("team_model", name, sizeof(name));
		//Com_Printf("team_model is set to %s\n", name);
	}
}

static void CL_SetTeamModel_f( void ) {
	char	*arg;
	char	name[256];

	arg = Cmd_Argv( 1 );
	if (arg[0]) {
		Cvar_Set( "team_model", arg );
	} else {
		Cvar_VariableStringBuffer( "team_model", name, sizeof(name) );
		Com_Printf("team_model is set to %s\n", name);
	}
}

void CL_SetForcePowers_f( void ) {
	return;
}

#ifdef G2_COLLISION_ENABLED
#define G2_VERT_SPACE_CLIENT_SIZE 256
#endif

/*
====================
CL_Init
====================
*/
void CL_Init( void ) {
	Com_Printf( "----- Client Initialization -----\n" );

	Con_Init ();

	CL_ClearState ();

	cls.state = CA_DISCONNECTED;	// no longer CA_UNINITIALIZED

	cls.realtime = 0;

	CL_InitInput ();

	//
	// register our variables
	//
	cl_noprint = Cvar_Get( "cl_noprint", "0", 0 );
	cl_motd = Cvar_Get ("cl_motd", "1", 0);

	cl_timeout = Cvar_Get ("cl_timeout", "200", 0);

	cl_timeNudge = Cvar_Get ("cl_timeNudge", "0", CVAR_TEMP );
	cl_shownet = Cvar_Get ("cl_shownet", "0", CVAR_TEMP );
	cl_showSend = Cvar_Get ("cl_showSend", "0", CVAR_TEMP );
	cl_showTimeDelta = Cvar_Get ("cl_showTimeDelta", "0", CVAR_TEMP );
	cl_freezeDemo = Cvar_Get ("cl_freezeDemo", "0", CVAR_TEMP );
	rcon_client_password = Cvar_Get ("rconPassword", "", CVAR_TEMP );
	cl_activeAction = Cvar_Get( "activeAction", "", CVAR_TEMP );
	
	cl_drawRecording = Cvar_Get ("cl_drawRecording", "1", CVAR_ARCHIVE | CVAR_GLOBAL );

	cl_timedemo = Cvar_Get ("timedemo", "0", 0);
	cl_aviFrameRate = Cvar_Get ("cl_aviFrameRate", "30", CVAR_ARCHIVE);
	cl_aviMotionJpeg = Cvar_Get ("cl_aviMotionJpeg", "1", CVAR_ARCHIVE);
	cl_aviMotionJpegQuality = Cvar_Get("cl_aviMotionJpegQuality", "90", CVAR_ARCHIVE);
	cl_forceavidemo = Cvar_Get ("cl_forceavidemo", "0", 0);

	rconAddress = Cvar_Get ("rconAddress", "", 0);

	cl_yawspeed = Cvar_Get("cl_yawspeed", "140", CVAR_ARCHIVE | CVAR_GLOBAL);
	cl_pitchspeed = Cvar_Get("cl_pitchspeed", "140", CVAR_ARCHIVE | CVAR_GLOBAL);
	cl_anglespeedkey = Cvar_Get("cl_anglespeedkey", "1.5", CVAR_ARCHIVE | CVAR_GLOBAL);

	cl_maxpackets = Cvar_Get("cl_maxpackets", "125", CVAR_ARCHIVE | CVAR_GLOBAL);
	cl_packetdup = Cvar_Get("cl_packetdup", "1", CVAR_ARCHIVE | CVAR_GLOBAL);

	cl_run = Cvar_Get ("cl_run", "1", CVAR_ARCHIVE | CVAR_GLOBAL);
	cl_sensitivity = Cvar_Get("sensitivity", "5", CVAR_ARCHIVE | CVAR_GLOBAL);
	cl_mouseAccel = Cvar_Get("cl_mouseAccel", "0", CVAR_ARCHIVE | CVAR_GLOBAL);
	cl_freelook = Cvar_Get("cl_freelook", "1", CVAR_ARCHIVE | CVAR_GLOBAL);

	cl_showMouseRate = Cvar_Get ("cl_showmouserate", "0", 0);
	cl_framerate	= Cvar_Get ("cl_framerate", "0", CVAR_TEMP);
	mv_allowDownload = Cvar_Get("mv_allowDownload", "1", CVAR_ARCHIVE | CVAR_GLOBAL); // renamed so old configs do not override

	cl_autolodscale = Cvar_Get("cl_autolodscale", "1", CVAR_ARCHIVE | CVAR_GLOBAL);

	cl_conXOffset = Cvar_Get ("cl_conXOffset", "0", 0);

	cl_inGameVideo = Cvar_Get("r_inGameVideo", "1", CVAR_ARCHIVE | CVAR_GLOBAL);

	cl_serverStatusResendTime = Cvar_Get ("cl_serverStatusResendTime", "750", 0);

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	Cvar_Get("cg_autoswitch", "1", CVAR_ARCHIVE | CVAR_GLOBAL);

	m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE | CVAR_GLOBAL);
	m_yaw = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE | CVAR_GLOBAL);
	m_forward = Cvar_Get("m_forward", "0.25", CVAR_ARCHIVE | CVAR_GLOBAL);
	m_side = Cvar_Get("m_side", "0.25", CVAR_ARCHIVE | CVAR_GLOBAL);
#ifdef MACOS_X
		// Input is jittery on OS X w/o this
	m_filter = Cvar_Get ("m_filter", "1", CVAR_ARCHIVE | CVAR_GLOBAL);
#else
	m_filter = Cvar_Get("m_filter", "0", CVAR_ARCHIVE | CVAR_GLOBAL);
#endif

	cl_motdString = Cvar_Get( "cl_motdString", "", CVAR_ROM );

	Cvar_Get("cl_maxPing", "800", CVAR_ARCHIVE | CVAR_GLOBAL);

	// userinfo
	Cvar_Get("name", "Padawan", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_GLOBAL);
	Cvar_Get("rate", "50000", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_GLOBAL);
	Cvar_Get("snaps", "30", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_GLOBAL);
	Cvar_Get("model", "kyle/default", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_GLOBAL);
//	Cvar_Get ("headmodel", "kyle/default", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get("team_model", "kyle/default", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_GLOBAL);
//	Cvar_Get ("team_headmodel", "kyle/default", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("forcepowers", "7-1-032330000000001333", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("g_redTeam", "Empire", CVAR_SERVERINFO | CVAR_ARCHIVE);
	Cvar_Get ("g_blueTeam", "Rebellion", CVAR_SERVERINFO | CVAR_ARCHIVE);
	Cvar_Get ("color1",  "4", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("color2", "5", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("teamtask", "0", CVAR_USERINFO );
	Cvar_Get ("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get("JK2MV", JK2MV_VERSION, CVAR_USERINFO | CVAR_ROM);

	Cvar_Get ("password", "", CVAR_USERINFO);
	Cvar_Get ("cg_predictItems", "1", CVAR_USERINFO | CVAR_ARCHIVE );


	// cgame might not be initialized before menu is used
	Cvar_Get ("cg_viewsize", "100", CVAR_ARCHIVE );
	
	// autorecord
	cl_autoDemo = Cvar_Get ("cl_autoDemo", "0", CVAR_ARCHIVE | CVAR_GLOBAL );
	cl_autoDemoFormat = Cvar_Get ("cl_autoDemoFormat", "%t_%m", CVAR_ARCHIVE | CVAR_GLOBAL );

	// mv cvars
	mv_slowrefresh = Cvar_Get("mv_slowrefresh", "3", CVAR_ARCHIVE | CVAR_GLOBAL);
	mv_coloredTextShadows	= Cvar_Get("mv_coloredTextShadows"	, "2", CVAR_ARCHIVE | CVAR_GLOBAL);
	mv_consoleShiftRequirement = Cvar_Get("mv_consoleShiftRequirement", "0", CVAR_ARCHIVE | CVAR_GLOBAL);
	mv_menuOverride = Cvar_Get("mv_menuOverride", "0", CVAR_INIT | CVAR_VM_NOWRITE);

	cl_downloadName = Cvar_Get("cl_downloadName", "", CVAR_INTERNAL);
	cl_downloadLocalName = Cvar_Get("cl_downloadLocalName", "", CVAR_INTERNAL);
	cl_downloadSize = Cvar_Get("cl_downloadSize", "", CVAR_INTERNAL);
	cl_downloadCount = Cvar_Get("cl_downloadCount", "", CVAR_INTERNAL);
	cl_downloadTime = Cvar_Get("cl_downloadTime", "", CVAR_INTERNAL);
	cl_downloadProtocol = Cvar_Get("cl_downloadProtocol", "", CVAR_INTERNAL);

	//EternalJK2MV
	cl_colorString = Cvar_Get("cl_colorString", "0", CVAR_ARCHIVE|CVAR_GLOBAL/*, "Bit value of selected colors in colorString, configure chat colors with /colorstring"*/);
	cl_colorStringCount = Cvar_Get("cl_colorStringCount", "0", CVAR_INTERNAL|CVAR_ROM|CVAR_ARCHIVE|CVAR_GLOBAL);
	cl_colorStringRandom = Cvar_Get("cl_colorStringRandom", "2", CVAR_ARCHIVE|CVAR_GLOBAL/*, "Randomness of the colors changing, higher numbers are less random"*/);

	cl_logChat = Cvar_Get("cl_logChat", "1", CVAR_ARCHIVE|CVAR_GLOBAL);


	//
	// register our commands
	//
	Cmd_AddCommand ("cmd", CL_ForwardToServer_f);
	Cmd_AddCommand ("configstrings", CL_Configstrings_f);
	Cmd_AddCommand ("clientinfo", CL_Clientinfo_f);
	Cmd_AddCommand ("fs_restart", CL_FS_Restart_f);
	Cmd_AddCommand ("snd_restart", CL_Snd_Restart_f);
	Cmd_AddCommand ("vid_restart", CL_Vid_Restart_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("demo", CL_PlayDemo_f);
	Cmd_SetCommandCompletionFunc( "demo", CL_CompleteDemoName );
	Cmd_AddCommand ("cinematic", CL_PlayCinematic_f);
	Cmd_AddCommand ("stoprecord", CL_StopRecord_f);
	Cmd_AddCommand ("connect", CL_Connect_f);
	Cmd_AddCommand ("reconnect", CL_Reconnect_f);
	Cmd_AddCommand ("localservers", CL_LocalServers_f);
	Cmd_AddCommand ("globalservers", CL_GlobalServers_f);
	Cmd_AddCommand ("rcon", CL_Rcon_f);
	Cmd_SetCommandCompletionFunc( "rcon", CL_CompleteRedirect );
	Cmd_AddCommand ("setenv", CL_Setenv_f );
	Cmd_AddCommand ("ping", CL_Ping_f );
	Cmd_AddCommand ("serverstatus", CL_ServerStatus_f );
	Cmd_AddCommand ("showip", CL_ShowIP_f );
	Cmd_AddCommand ("fs_openedList", CL_OpenedPK3List_f );
	Cmd_AddCommand ("fs_referencedList", CL_ReferencedPK3List_f );
	Cmd_AddCommand ("model", CL_SetModel_f );
	Cmd_SetCommandCompletionFunc( "model", CL_CompleteModelName );
	Cmd_AddCommand ("team_model", CL_SetTeamModel_f );
	Cmd_SetCommandCompletionFunc( "team_model", CL_CompleteModelName );
	Cmd_AddCommand ("forcepowers", CL_SetForcePowers_f );
	Cmd_AddCommand ("saveDemo", demoAutoSave_f);
	Cmd_AddCommand ("saveDemoLast", demoAutoSaveLast_f);
	Cmd_AddCommand ("video", CL_Video_f);
	Cmd_AddCommand ("stopvideo", CL_StopVideo_f);
	Cmd_AddCommand ("silent", CL_Silent_f);
	Cmd_SetCommandCompletionFunc( "silent", CL_CompleteRedirect );

	//EternalJK2MV
	Cmd_AddCommand("colorstring", CL_ColorString_f/*, "Color say text"*/);
	Cmd_AddCommand("colorname", CL_ColorName_f/*, "Color name"*/);

	CL_InitRef();

	SCR_Init ();

	Cbuf_Execute ();

	Cvar_Set( "cl_running", "1" );

#ifdef G2_COLLISION_ENABLED
	G2VertSpaceClient = new CMiniHeap(G2_VERT_SPACE_CLIENT_SIZE * 1024);
#endif

	Com_Printf( "----- Client Initialization Complete -----\n" );
}


/*
===============
CL_Shutdown

===============
*/
void CL_Shutdown( void ) {
	static qboolean recursive = qfalse;

	Com_Printf( "----- CL_Shutdown -----\n" );

	if ( recursive ) {
		printf ("recursive shutdown\n");
		return;
	}
	recursive = qtrue;

#ifdef G2_COLLISION_ENABLED
	if (G2VertSpaceClient)
	{
		delete G2VertSpaceClient;
		G2VertSpaceClient = 0;
	}
#endif

	CL_Disconnect( qtrue );

	CL_ShutdownRef();	//must be before shutdown all so the images get dumped in RE_Shutdown

	// RJ: added the shutdown all to close down the cgame (to free up some memory, such as in the fx system)
	CL_ShutdownAll();

	S_Shutdown();
	//CL_ShutdownUI();

	Cmd_RemoveCommand ("cmd");
	Cmd_RemoveCommand ("configstrings");
	Cmd_RemoveCommand ("clientinfo");
	Cmd_RemoveCommand ("snd_restart");
	Cmd_RemoveCommand ("vid_restart");
	Cmd_RemoveCommand ("disconnect");
	Cmd_RemoveCommand ("record");
	Cmd_RemoveCommand ("demo");
	Cmd_RemoveCommand ("cinematic");
	Cmd_RemoveCommand ("stoprecord");
	Cmd_RemoveCommand ("connect");
	Cmd_RemoveCommand ("reconnect");
	Cmd_RemoveCommand ("localservers");
	Cmd_RemoveCommand ("globalservers");
	Cmd_RemoveCommand ("rcon");
	Cmd_RemoveCommand ("setenv");
	Cmd_RemoveCommand ("ping");
	Cmd_RemoveCommand ("serverstatus");
	Cmd_RemoveCommand ("showip");
	Cmd_RemoveCommand ("fs_openedList");
	Cmd_RemoveCommand ("fs_referencedList");
	Cmd_RemoveCommand ("model");
	Cmd_RemoveCommand ("forcepowers");
	Cmd_RemoveCommand ("saveDemo");
	Cmd_RemoveCommand ("saveDemoLast");
	Cmd_RemoveCommand ("video");
	Cmd_RemoveCommand ("stopvideo");

	Cvar_Set( "cl_running", "0" );

	recursive = qfalse;

	Com_Memset( &cls, 0, sizeof( cls ) );

	Com_Printf( "-----------------------\n" );

}

void QDECL CL_LogPrintf(fileHandle_t fileHandle, const char *fmt, ...) {
	va_list argptr;
	char string[1024] = { 0 };
	size_t len;
	time_t rawtime;
	time(&rawtime);

	strftime(string, sizeof(string), "[%Y-%m-%d] [%H:%M:%S] ", localtime(&rawtime));

	len = strlen(string);

	va_start(argptr, fmt);
	Q_vsnprintf(string + len, sizeof(string) - len, fmt, argptr);
	va_end(argptr);

	if (!fileHandle)
		return;

	FS_Write(string, strlen(string), fileHandle);
}

static void CL_SetServerInfo(serverInfo_t *server, const char *info, int ping) {
	if (server) {
		if (info) {
			//server->clients = atoi(Info_ValueForKey(info, "clients"));
			Q_strncpyz(server->hostName,Info_ValueForKey(info, "hostname"), MAX_NAME_LENGTH);
			Q_strncpyz(server->mapName, Info_ValueForKey(info, "mapname"), MAX_NAME_LENGTH);
			server->maxClients = atoi(Info_ValueForKey(info, "sv_maxclients"));
			Q_strncpyz(server->game,Info_ValueForKey(info, "game"), MAX_NAME_LENGTH);
			server->gameType = atoi(Info_ValueForKey(info, "gametype"));
			server->netType = atoi(Info_ValueForKey(info, "nettype"));
			server->minPing = atoi(Info_ValueForKey(info, "minping"));
			server->maxPing = atoi(Info_ValueForKey(info, "maxping"));
//			server->allowAnonymous = atoi(Info_ValueForKey(info, "sv_allowAnonymous"));
			server->needPassword = (qboolean)!!atoi(Info_ValueForKey(info, "needpass" ));
			server->trueJedi = atoi(Info_ValueForKey(info, "truejedi" ));
			server->weaponDisable = atoi(Info_ValueForKey(info, "wdisable" ));
			server->forceDisable = atoi(Info_ValueForKey(info, "fdisable" ));
			server->protocol = atoi(Info_ValueForKey(info, "protocol"));
//			server->pure = (qboolean)!!atoi(Info_ValueForKey(info, "pure" ));
		}
		server->ping = ping;
	}
}

static void CL_SetServerInfoByAddress(netadr_t from, const char *info, int ping) {
	int i;

	for (i = 0; i < MAX_OTHER_SERVERS; i++) {
		if (NET_CompareAdr(from, cls.localServers[i].adr)) {
			CL_SetServerInfo(&cls.localServers[i], info, ping);
		}
	}

	for (i = 0; i < MAX_OTHER_SERVERS; i++) {
		if (NET_CompareAdr(from, cls.mplayerServers[i].adr)) {
			CL_SetServerInfo(&cls.mplayerServers[i], info, ping);
		}
	}

	for (i = 0; i < MAX_GLOBAL_SERVERS; i++) {
		if (NET_CompareAdr(from, cls.globalServers[i].adr)) {
			CL_SetServerInfo(&cls.globalServers[i], info, ping);
		}
	}

	for (i = 0; i < MAX_OTHER_SERVERS; i++) {
		if (NET_CompareAdr(from, cls.favoriteServers[i].adr)) {
			CL_SetServerInfo(&cls.favoriteServers[i], info, ping);
		}
	}

}

// For 1.03 in the menu...
// now also used for botfiltering, leave serverInfo_t.clients untouched for backwards compatibility with old menu VM's
// the new one just uses clients - bots = realplayers
void MV_SetServerFakeInfoByAddress( netadr_t from, mvversion_t version, int clients, int bots ) {
	int i;

	for (i = 0; i < MAX_OTHER_SERVERS; i++) {
		if (NET_CompareAdr(from, cls.localServers[i].adr)) {
			if (version != VERSION_UNDEF)
				cls.localServers[i].gameVersion = version;

			if (clients != -1) {
				cls.localServers[i].clients = clients;
				cls.localServers[i].bots = bots;
			}
		}
	}

	for (i = 0; i < MAX_OTHER_SERVERS; i++) {
		if (NET_CompareAdr(from, cls.mplayerServers[i].adr)) {
			if (version != VERSION_UNDEF)
				cls.mplayerServers[i].gameVersion = version;

			if (clients != -1) {
				cls.mplayerServers[i].clients = clients;
				cls.mplayerServers[i].bots = bots;
			}
		}
	}

	for (i = 0; i < MAX_GLOBAL_SERVERS; i++) {
		if (NET_CompareAdr(from, cls.globalServers[i].adr)) {
			if (version != VERSION_UNDEF)
				cls.globalServers[i].gameVersion = version;

			if (clients != -1) {
				cls.globalServers[i].clients = clients;
				cls.globalServers[i].bots = bots;
			}
		}
	}

	for (i = 0; i < MAX_OTHER_SERVERS; i++) {
		if (NET_CompareAdr(from, cls.favoriteServers[i].adr)) {
			if (version != VERSION_UNDEF)
				cls.favoriteServers[i].gameVersion = version;

			if (clients != -1) {
				cls.favoriteServers[i].clients = clients;
				cls.favoriteServers[i].bots = bots;
			}
		}
	}
}

/*
===================
CL_ServerInfoPacket
===================
*/
void CL_ServerInfoPacket( netadr_t from, msg_t *msg ) {
	int		i, type;
	char	info[MAX_INFO_STRING];
	char*	str;
	char	*infoString;
	mvprotocol_t prot;

	infoString = MSG_ReadString( msg );

	// if this isn't the correct protocol version, ignore it
	prot = (mvprotocol_t)atoi(Info_ValueForKey(infoString, "protocol"));
	if (prot != PROTOCOL15 && prot != PROTOCOL16) {
		Com_DPrintf( "Different protocol info packet: %s\n", infoString );
	}

	// multiprotocol support
	if (cls.state == CA_CONNECTING && NET_CompareAdr(from, clc.serverAddress)) {
		if ( MV_GetCurrentGameversion() == VERSION_UNDEF )
		{
			switch ( prot )
			{
				case PROTOCOL15:
					MV_SetCurrentGameversion(VERSION_1_02);
					break;
				case PROTOCOL16:
				default:
					MV_SetCurrentGameversion(VERSION_1_04);
					break;
			}
		}

		if (!clc.httpdl[0]) {
			char *val;
			
			val = Info_ValueForKey(infoString, "mvhttp");
			if (strtol(val, NULL, 10)) {
				Com_sprintf(clc.httpdl, sizeof(clc.httpdl), "http://%i.%i.%i.%i:%s",
					clc.serverAddress.ip[0], clc.serverAddress.ip[1],
					clc.serverAddress.ip[2], clc.serverAddress.ip[3], val);
			} else if ((val = Info_ValueForKey(infoString, "mvhttpurl")) && Q_stristr(val, "http://")) {
				Q_strncpyz(clc.httpdl, val, sizeof(clc.httpdl));

				// make sure there is no "/" on the end
				// so it always is in the format "http://a.org"
				size_t len = strlen(clc.httpdl);
				if (clc.httpdl[len - 1] == '/') {
					clc.httpdl[len - 1] = 0;
				}
			}

			clc.httpdlvalid = qtrue;
		}

		return;
	}

	// iterate servers waiting for ping response
	for (i=0; i<MAX_PINGREQUESTS; i++)
	{
		if ( cl_pinglist[i].adr.port && !cl_pinglist[i].time && NET_CompareAdr( from, cl_pinglist[i].adr ) )
		{
			// calc ping time
			cl_pinglist[i].time = cls.realtime - cl_pinglist[i].start + 1;
			Com_DPrintf( "ping time %dms from %s\n", cl_pinglist[i].time, NET_AdrToString( from ) );

			// save of info
			Q_strncpyz( cl_pinglist[i].info, infoString, sizeof( cl_pinglist[i].info ) );

			// tack on the net type
			// NOTE: make sure these types are in sync with the netnames strings in the UI
			switch (from.type)
			{
				case NA_BROADCAST:
				case NA_IP:
					str = "udp";
					type = 1;
					break;

				default:
					str = "???";
					type = 0;
					break;
			}
			Info_SetValueForKey( cl_pinglist[i].info, "nettype", va("%d", type) );
			CL_SetServerInfoByAddress(from, infoString, cl_pinglist[i].time);

			return;
		}
	}

	// if not just sent a local broadcast or pinging local servers
	if (cls.pingUpdateSource != AS_LOCAL) {
		return;
	}

	for ( i = 0 ; i < MAX_OTHER_SERVERS ; i++ ) {
		// empty slot
		if ( cls.localServers[i].adr.port == 0 ) {
			break;
		}

		// avoid duplicate
		if ( NET_CompareAdr( from, cls.localServers[i].adr ) ) {
			return;
		}
	}

	if ( i == MAX_OTHER_SERVERS ) {
		Com_DPrintf( "MAX_OTHER_SERVERS hit, dropping infoResponse\n" );
		return;
	}

	// add this to the list
	cls.numlocalservers = i+1;
	cls.localServers[i].adr = from;
	cls.localServers[i].clients = 0;
	cls.localServers[i].hostName[0] = '\0';
	cls.localServers[i].mapName[0] = '\0';
	cls.localServers[i].maxClients = 0;
	cls.localServers[i].maxPing = 0;
	cls.localServers[i].minPing = 0;
	cls.localServers[i].ping = -1;
	cls.localServers[i].game[0] = '\0';
	cls.localServers[i].gameType = 0;
	cls.localServers[i].netType = from.type;
//	cls.localServers[i].allowAnonymous = 0;
	cls.localServers[i].needPassword = qfalse;
	cls.localServers[i].trueJedi = 0;
	cls.localServers[i].weaponDisable = 0;
	cls.localServers[i].forceDisable = 0;
//	cls.localServers[i].pure = qfalse;

	Q_strncpyz( info, MSG_ReadString( msg ), MAX_INFO_STRING );
	if (strlen(info)) {
		if (info[strlen(info)-1] != '\n') {
			Q_strcat(info, sizeof(info), "\n");
		}
		Com_Printf( "%s: %s", NET_AdrToString( from ), info );
	}
}

/*
===================
CL_GetServerStatus
===================
*/
serverStatus_t *CL_GetServerStatus( netadr_t from ) {
	serverStatus_t *serverStatus;
	int i, oldest, oldestTime;

	serverStatus = NULL;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if ( NET_CompareAdr( from, cl_serverStatusList[i].address ) ) {
			return &cl_serverStatusList[i];
		}
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if ( cl_serverStatusList[i].retrieved ) {
			return &cl_serverStatusList[i];
		}
	}
	oldest = -1;
	oldestTime = 0;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if (oldest == -1 || cl_serverStatusList[i].startTime < oldestTime) {
			oldest = i;
			oldestTime = cl_serverStatusList[i].startTime;
		}
	}
	if (oldest != -1) {
		return &cl_serverStatusList[oldest];
	}
	serverStatusCount++;
	return &cl_serverStatusList[serverStatusCount & (MAX_SERVERSTATUSREQUESTS-1)];
}

/*
===================
CL_ServerStatus
===================
*/
int CL_ServerStatus( const char *serverAddress, char *serverStatusString, int maxLen ) {
	int i;
	netadr_t	to;
	serverStatus_t *serverStatus;

	// if no server address then reset all server status requests
	if ( !serverAddress ) {
		for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
			cl_serverStatusList[i].address.port = 0;
			cl_serverStatusList[i].retrieved = qtrue;
		}
		return qfalse;
	}
	// get the address
	if ( !NET_StringToAdr( serverAddress, &to ) ) {
		return qfalse;
	}
	serverStatus = CL_GetServerStatus( to );
	// if no server status string then reset the server status request for this address
	if ( !serverStatusString ) {
		serverStatus->retrieved = qtrue;
		return qfalse;
	}

	// if this server status request has the same address
	if ( NET_CompareAdr( to, serverStatus->address) ) {
		// if we recieved an response for this server status request
		if (!serverStatus->pending) {
			Q_strncpyz(serverStatusString, serverStatus->string, maxLen);
			serverStatus->retrieved = qtrue;
			serverStatus->startTime = 0;
			return qtrue;
		}
		// resend the request regularly
		else if ( serverStatus->startTime < Com_Milliseconds() - cl_serverStatusResendTime->integer ) {
			serverStatus->print = qfalse;
			serverStatus->pending = qtrue;
			serverStatus->retrieved = qfalse;
			serverStatus->time = 0;
			serverStatus->startTime = Com_Milliseconds();
			NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );
			return qfalse;
		}
	}
	// if retrieved
	else if ( serverStatus->retrieved ) {
		serverStatus->address = to;
		serverStatus->print = qfalse;
		serverStatus->pending = qtrue;
		serverStatus->retrieved = qfalse;
		serverStatus->startTime = Com_Milliseconds();
		serverStatus->time = 0;
		NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );
		return qfalse;
	}
	return qfalse;
}

/*
===================
CL_ServerStatusResponse
===================
*/
void CL_ServerStatusResponse( netadr_t from, msg_t *msg ) {
	char	*s;
	char	info[MAX_INFO_STRING];
	int		i, l, score, ping;
	int		len;
	int		bots;
	serverStatus_t *serverStatus;

	char *versionString;

	s = MSG_ReadStringLine(msg);

	// For 1.03 in the menu...
	versionString = Info_ValueForKey(s, "version");
	if (versionString && CL_ServerVersionIs103(versionString))
	{
		MV_SetServerFakeInfoByAddress(from, VERSION_1_03, -1, -1);
	} else
	{
		mvprotocol_t prot;
		prot = (mvprotocol_t)atoi(Info_ValueForKey(s, "protocol"));

		switch (prot)
		{
		case PROTOCOL15:
			MV_SetServerFakeInfoByAddress(from, VERSION_1_02, -1, -1);
			break;
		case PROTOCOL16:
			MV_SetServerFakeInfoByAddress(from, VERSION_1_04, -1, -1);
			break;
		default:
			MV_SetServerFakeInfoByAddress(from, VERSION_UNDEF, -1, -1);
			break;
		}
	}

	// multiprotocol support
	if (cls.state == CA_CONNECTING && NET_CompareAdr(from, clc.serverAddress))
	{
		char *versionString;
		versionString = Info_ValueForKey(s, "version");

		// We used to seperate "1.02" and "1.04" by protocol "15" and "16". As "1.03" is using protocol "15", too we just look at the "version" to detect "1.03". If we don't find "1.03" we handle by protocol again.
		if ( versionString && CL_ServerVersionIs103(versionString) )
		{
			MV_SetCurrentGameversion(VERSION_1_03);
		}
		else
		{
			mvprotocol_t prot;
			prot = (mvprotocol_t)atoi(Info_ValueForKey(s, "protocol"));

			switch ( prot )
			{
				case PROTOCOL15:
					MV_SetCurrentGameversion(VERSION_1_02);
					break;
				case PROTOCOL16:
				default:
					MV_SetCurrentGameversion(VERSION_1_04);
					break;
			}
		}

		clc.udpdl = atoi(Info_ValueForKey(s, "sv_allowdownload"));

		return;
	}

	serverStatus = NULL;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if ( NET_CompareAdr( from, cl_serverStatusList[i].address ) ) {
			serverStatus = &cl_serverStatusList[i];
			break;
		}
	}

	// if we didn't request this server status
	if (!serverStatus) {
		return;
	}

	len = 0;
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len, "%s", s);

	if (serverStatus->print) {
		Com_Printf("Server settings:\n");
		// print cvars
		while (*s) {
			for (i = 0; i < 2 && *s; i++) {
				if (*s == '\\')
					s++;
				l = 0;
				while (*s) {
					info[l++] = *s;
					if (l >= MAX_INFO_STRING-1)
						break;
					s++;
					if (*s == '\\') {
						break;
					}
				}
				info[l] = '\0';
				if (i) {
					Com_Printf("%s\n", info);
				}
				else {
					Com_Printf("%-24s", info);
				}
			}
		}
	}

	len = (int)strlen(serverStatus->string);
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len, "\\");

	if (serverStatus->print) {
		Com_Printf("\nPlayers:\n");
		Com_Printf("num: score: ping: name:\n");
	}

	bots = 0;
	for (i = 0, s = MSG_ReadStringLine( msg ); *s; s = MSG_ReadStringLine( msg ), i++) {
		len = (int)strlen(serverStatus->string);
		Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len, "\\%s", s);

		score = ping = 0;
		sscanf(s, "%d %d", &score, &ping);
		s = strchr(s, ' ');
		if (s)
			s = strchr(s+1, ' ');
		if (s)
			s++;
		else
			s = "unknown";

		if (ping == 0) {
			bots++;
		}

		if (serverStatus->print) {
			Com_Printf("%-2d   %-3d    %-3d   %s\n", i, score, ping, s);
		}
	}

	MV_SetServerFakeInfoByAddress(from, VERSION_UNDEF, i, bots);

	len = (int)strlen(serverStatus->string);
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len, "\\");

	serverStatus->time = Com_Milliseconds();
	serverStatus->address = from;
	serverStatus->pending = qfalse;
	if (serverStatus->print) {
		serverStatus->retrieved = qtrue;
	}
}

/*
==================
CL_LocalServers_f
==================
*/
void CL_LocalServers_f( void ) {
	char		*message;
	int			i, j;
	netadr_t	to;

	Com_Printf( "Scanning for servers on the local network...\n");

	// reset the list, waiting for response
	cls.numlocalservers = 0;
	cls.pingUpdateSource = AS_LOCAL;

	for (i = 0; i < MAX_OTHER_SERVERS; i++) {
		qboolean b = cls.localServers[i].visible;
		Com_Memset(&cls.localServers[i], 0, sizeof(cls.localServers[i]));
		cls.localServers[i].visible = b;
	}
	Com_Memset( &to, 0, sizeof( to ) );

	// The 'xxx' in the message is a challenge that will be echoed back
	// by the server.  We don't care about that here, but master servers
	// can use that to prevent spoofed server responses from invalid ip
	message = "\377\377\377\377getinfo xxx";

	// send each message twice in case one is dropped
	for ( i = 0 ; i < 2 ; i++ ) {
		// send a broadcast packet on each server port
		// we support multiple server ports so a single machine
		// can nicely run multiple servers
		for ( j = 0 ; j < NUM_SERVER_PORTS ; j++ ) {
			to.port = BigShort( (short)(PORT_SERVER + j) );

			to.type = NA_BROADCAST;
			NET_SendPacket( NS_CLIENT, (int)strlen( message ), message, to );
		}
	}
}

/*
==================
CL_GlobalServers_f
==================
*/
void CL_GlobalServers_f( void ) {
	netadr_t	to;
	int			i;
	const char	*s;
	const char	*keywords;

	if ( Cmd_Argc() < 3) {
		Com_Printf( "usage: globalservers <master# 0-%d> <protocols> [keywords]\n", MAX_MASTER_SERVERS - 1);
		return;
	}

	cls.masterNum = atoi( Cmd_Argv(1) );
	cls.numglobalservers = -1;
	cls.pingUpdateSource = AS_GLOBAL;

	keywords = Cmd_ArgsFrom(3);

	// multimaster
	for (i = 0; i < MAX_MASTER_SERVERS; i++) {
		cvar_t *master = Cvar_FindVar(va("sv_master%i", i + 1));

		if (master == NULL || !strlen(master->string))
			continue;

		Com_Printf("Requesting servers from the master %s...\n", master->string);

		// reset the list, waiting for response
		// -1 is used to distinguish a "no response"
		NET_StringToAdr(master->string, &to);
		to.type = NA_IP;
		to.port = BigShort(PORT_MASTER);

		s = Cmd_Argv(2);
		while (Q_isdigit(*s)) {
			NET_OutOfBandPrint(NS_SERVER, to, "getservers %d %s", atoi(s), keywords);

			// skip to next protocol number, separated by anything other than digits
			while (Q_isdigit(*s)) s++;
			while (*s && !Q_isdigit(*s)) s++;
		}
	}

	CL_RequestMotd();
}


/*
==================
CL_GetPing
==================
*/
void CL_GetPing( int n, char *buf, int buflen, int *pingtime )
{
	const char	*str;
	int		time;
	int		maxPing;

	if (n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port)
	{
		// empty slot
		buf[0]    = '\0';
		*pingtime = 0;
		return;
	}

	str = NET_AdrToString( cl_pinglist[n].adr );
	Q_strncpyz( buf, str, buflen );

	time = cl_pinglist[n].time;
	if (!time)
	{
		// check for timeout
		time = cls.realtime - cl_pinglist[n].start;
		maxPing = Cvar_VariableIntegerValue( "cl_maxPing" );
		if( maxPing < 100 ) {
			maxPing = 100;
		}
		if (time < maxPing)
		{
			// not timed out yet
			time = 0;
		}
	}

	CL_SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info, cl_pinglist[n].time);

	*pingtime = time;
}

/*
==================
CL_UpdateServerInfo
==================
*/
void CL_UpdateServerInfo( int n )
{
	if (!cl_pinglist[n].adr.port)
	{
		return;
	}

	CL_SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info, cl_pinglist[n].time );
}

/*
==================
CL_GetPingInfo
==================
*/
void CL_GetPingInfo( int n, char *buf, int buflen )
{
	if (n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port)
	{
		// empty slot
		if (buflen)
			buf[0] = '\0';
		return;
	}

	Q_strncpyz( buf, cl_pinglist[n].info, buflen );
}

/*
==================
CL_ClearPing
==================
*/
void CL_ClearPing( int n )
{
	if (n < 0 || n >= MAX_PINGREQUESTS)
		return;

	cl_pinglist[n].adr.port = 0;
}

/*
==================
CL_GetPingQueueCount
==================
*/
int CL_GetPingQueueCount( void )
{
	int		i;
	int		count;
	ping_t*	pingptr;

	count   = 0;
	pingptr = cl_pinglist;

	for (i=0; i<MAX_PINGREQUESTS; i++, pingptr++ ) {
		if (pingptr->adr.port) {
			count++;
		}
	}

	return (count);
}

/*
==================
CL_GetFreePing
==================
*/
ping_t* CL_GetFreePing( void )
{
	ping_t*	pingptr;
	ping_t*	best;
	int		oldest;
	int		i;
	int		time;

	pingptr = cl_pinglist;
	for (i=0; i<MAX_PINGREQUESTS; i++, pingptr++ )
	{
		// find free ping slot
		if (pingptr->adr.port)
		{
			if (!pingptr->time)
			{
				if (cls.realtime - pingptr->start < 500)
				{
					// still waiting for response
					continue;
				}
			}
			else if (pingptr->time < 500)
			{
				// results have not been queried
				continue;
			}
		}

		// clear it
		pingptr->adr.port = 0;
		return (pingptr);
	}

	// use oldest entry
	pingptr = cl_pinglist;
	best    = cl_pinglist;
	oldest  = INT_MIN;
	for (i=0; i<MAX_PINGREQUESTS; i++, pingptr++ )
	{
		// scan for oldest
		time = cls.realtime - pingptr->start;
		if (time > oldest)
		{
			oldest = time;
			best   = pingptr;
		}
	}

	return (best);
}

/*
==================
CL_Ping_f
==================
*/
void CL_Ping_f( void ) {
	netadr_t	to;
	ping_t*		pingptr;
	char*		server;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: ping [server]\n");
		return;
	}

	Com_Memset( &to, 0, sizeof(netadr_t) );

	server = Cmd_Argv(1);

	if ( !NET_StringToAdr( server, &to ) ) {
		return;
	}

	pingptr = CL_GetFreePing();

	memcpy( &pingptr->adr, &to, sizeof (netadr_t) );
	pingptr->start = cls.realtime;
	pingptr->time  = 0;

	CL_SetServerInfoByAddress(pingptr->adr, NULL, 0);

	NET_OutOfBandPrint( NS_CLIENT, to, "getinfo xxx" );
}

/*
==================
CL_UpdateVisiblePings_f
==================
*/
qboolean CL_UpdateVisiblePings_f(int source) {
	int			slots, i, max_req;
	char		buff[MAX_STRING_CHARS];
	int			pingTime;
	int			max;
	qboolean status = qfalse;

	if (source < 0 || source > AS_FAVORITES) {
		return qfalse;
	}

	cls.pingUpdateSource = source;

	// some providers will block a large amount of UDP packets to a huge range of IP addresses in a very short time
	if (mv_slowrefresh->integer) {
		max_req = mv_slowrefresh->integer;
	} else {
		max_req = MAX_PINGREQUESTS;
	}

	slots = CL_GetPingQueueCount();
	if (slots < max_req) {
		serverInfo_t *server = NULL;

		max = (source == AS_GLOBAL) ? MAX_GLOBAL_SERVERS : MAX_OTHER_SERVERS;
		switch (source) {
			case AS_LOCAL :
				server = &cls.localServers[0];
				max = cls.numlocalservers;
			break;
			case AS_MPLAYER :
				server = &cls.mplayerServers[0];
				max = cls.nummplayerservers;
			break;
			case AS_GLOBAL :
				server = &cls.globalServers[0];
				max = cls.numglobalservers;
			break;
			case AS_FAVORITES :
				server = &cls.favoriteServers[0];
				max = cls.numfavoriteservers;
			break;
		}
		for (i = 0; i < max; i++) {
			if (server[i].visible) {
				if (server[i].ping == -1) {
					int j;

					if (slots >= max_req) {
						break;
					}
					for (j = 0; j < max_req; j++) {
						if (!cl_pinglist[j].adr.port) {
							continue;
						}
						if (NET_CompareAdr( cl_pinglist[j].adr, server[i].adr)) {
							// already on the list
							break;
						}
					}
					if (j >= max_req) {
						status = qtrue;
						for (j = 0; j < max_req; j++) {
							if (!cl_pinglist[j].adr.port) {
								break;
							}
						}

						memcpy(&cl_pinglist[j].adr, &server[i].adr, sizeof(netadr_t));
						cl_pinglist[j].start = cls.realtime;
						cl_pinglist[j].time = 0;

						NET_OutOfBandPrint( NS_CLIENT, cl_pinglist[j].adr, "getinfo" );

						serverStatus_t *serverStatus = CL_GetServerStatus(cl_pinglist[j].adr);
						serverStatus->address = cl_pinglist[j].adr;
						serverStatus->print = qfalse;
						serverStatus->pending = qtrue;
						serverStatus->retrieved = qfalse;
						serverStatus->startTime = Com_Milliseconds();
						serverStatus->time = 0;
						NET_OutOfBandPrint(NS_CLIENT, cl_pinglist[j].adr, "getstatus");

						slots++;
					}
				}
				// if the server has a ping higher than cl_maxPing or
				// the ping packet got lost
				else if (server[i].ping == 0) {
					// if we are updating global servers
					if (source == AS_GLOBAL) {
						//
						if ( cls.numGlobalServerAddresses > 0 ) {
							// overwrite this server with one from the additional global servers
							cls.numGlobalServerAddresses--;
							CL_InitServerInfo(&server[i], &cls.globalServerAddresses[cls.numGlobalServerAddresses]);
							// NOTE: the server[i].visible flag stays untouched
						}
					}
				}
			}
		}
	}

	if (slots) {
		status = qtrue;
	}
	for (i = 0; i < max_req; i++) {
		if (!cl_pinglist[i].adr.port) {
			continue;
		}
		CL_GetPing(i, buff, max_req, &pingTime);
		if (pingTime != 0) {
			CL_ClearPing(i);
			status = qtrue;
		}
	}

	return status;
}

/*
==================
CL_ServerStatus_f
==================
*/
void CL_ServerStatus_f(void) {
	netadr_t	to;
	char		*server;
	serverStatus_t *serverStatus;

	Com_Memset( &to, 0, sizeof(netadr_t) );

	if ( Cmd_Argc() != 2 ) {
		if ( cls.state != CA_ACTIVE || clc.demoplaying ) {
			Com_Printf ("Not connected to a server.\n");
			Com_Printf( "Usage: serverstatus [server]\n");
			return;
		}
		server = cls.servername;
	}
	else {
		server = Cmd_Argv(1);
	}

	if ( !NET_StringToAdr( server, &to ) ) {
		return;
	}

	NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );

	serverStatus = CL_GetServerStatus( to );
	serverStatus->address = to;
	serverStatus->print = qtrue;
	serverStatus->pending = qtrue;
}

/*
==================
CL_ShowIP_f
==================
*/
void CL_ShowIP_f(void) {
	Sys_ShowIP();
}

/*
====================
CL_GetVMGLConfig
====================
*/
void CL_GetVMGLConfig(vmglconfig_t *vmglconfig) {
	Q_strncpyz(vmglconfig->renderer_string, cls.glconfig.renderer_string, sizeof(vmglconfig->renderer_string));
	Q_strncpyz(vmglconfig->vendor_string, cls.glconfig.vendor_string, sizeof(vmglconfig->vendor_string));
	Q_strncpyz(vmglconfig->version_string, cls.glconfig.version_string, sizeof(vmglconfig->version_string));
	Q_strncpyz(vmglconfig->extensions_string, cls.glconfig.extensions_string, sizeof(vmglconfig->extensions_string));

	vmglconfig->maxTextureSize = cls.glconfig.maxTextureSize;
	vmglconfig->maxActiveTextures = cls.glconfig.maxActiveTextures;

	vmglconfig->colorBits = cls.glconfig.colorBits;
	vmglconfig->depthBits = cls.glconfig.depthBits;
	vmglconfig->stencilBits = cls.glconfig.stencilBits;

	vmglconfig->deviceSupportsGamma = (qboolean)(cls.glconfig.deviceSupportsGamma || cls.glconfig.deviceSupportsPostprocessingGamma);
	vmglconfig->textureCompression = cls.glconfig.textureCompression;
	vmglconfig->textureEnvAddAvailable = cls.glconfig.textureEnvAddAvailable;
	vmglconfig->textureFilterAnisotropicAvailable = cls.glconfig.textureFilterAnisotropicMax == 0.0f ? qfalse : qtrue;
	vmglconfig->clampToEdgeAvailable = cls.glconfig.clampToEdgeAvailable;

	vmglconfig->vidWidth = cls.glconfig.vidWidth;
	vmglconfig->vidHeight = cls.glconfig.vidHeight;
	vmglconfig->windowAspect = cls.glconfig.windowAspect;
	vmglconfig->displayFrequency = cls.glconfig.displayFrequency;
	vmglconfig->isFullscreen = cls.glconfig.isFullscreen;
	vmglconfig->stereoEnabled = cls.glconfig.stereoEnabled;
	vmglconfig->smpActive = cls.glconfig.smpActive;
}
