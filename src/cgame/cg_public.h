// Copyright (C) 1999-2000 Id Software, Inc.
//


#define	CMD_BACKUP			512//JAPRO - FPS UNLOCK ENGINE	
#define	CMD_MASK			(CMD_BACKUP - 1)
// allow a lot of command backups for very fast systems
// multiple commands may be combined into a single packet, so this
// needs to be larger than PACKET_BACKUP


#define	MAX_ENTITIES_IN_SNAPSHOT	256

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
typedef struct {
	int				snapFlags;			// SNAPFLAG_RATE_DELAYED, etc
	int				ping;

	int				serverTime;		// server time the message is valid for (in msec)

	byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	playerState_t	ps;						// complete information about the current player at this time

	int				numEntities;			// all of the entities that need to be presented
	entityState_t	entities[MAX_ENTITIES_IN_SNAPSHOT];	// at the time of this snapshot

	int				numServerCommands;		// text based server commands to execute when this
	int				serverCommandSequence;	// snapshot becomes current
} snapshot_t;

typedef struct {
	int				snapFlags;			// SNAPFLAG_RATE_DELAYED, etc
	int				ping;

	int				serverTime;		// server time the message is valid for (in msec)

	byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	playerState15_t	ps;						// complete information about the current player at this time

	int				numEntities;			// all of the entities that need to be presented
	entityState_t	entities[MAX_ENTITIES_IN_SNAPSHOT];	// at the time of this snapshot

	int				numServerCommands;		// text based server commands to execute when this
	int				serverCommandSequence;	// snapshot becomes current
} snapshot15_t;

enum {
  CGAME_EVENT_NONE,
  CGAME_EVENT_TEAMMENU,
  CGAME_EVENT_SCOREBOARD,
  CGAME_EVENT_EDITHUD
};


/*
==================================================================

functions imported from the main executable

==================================================================
*/

#define	CGAME_IMPORT_API_VERSION	5

typedef enum {
	CG_PRINT = 0,
	CG_ERROR,
	CG_MILLISECONDS,
	CG_CVAR_REGISTER,
	CG_CVAR_UPDATE,
	CG_CVAR_SET,
	CG_CVAR_VARIABLESTRINGBUFFER,
	CG_ARGC,
	CG_ARGV,
	CG_ARGS,
	CG_FS_FOPENFILE,
	CG_FS_READ,
	CG_FS_WRITE,
	CG_FS_FCLOSEFILE,
	CG_SENDCONSOLECOMMAND,
	CG_ADDCOMMAND,
	CG_REMOVECOMMAND,
	CG_SENDCLIENTCOMMAND,
	CG_UPDATESCREEN,
	CG_CM_LOADMAP,
	CG_CM_NUMINLINEMODELS,
	CG_CM_INLINEMODEL,
	CG_CM_TEMPBOXMODEL,
	CG_CM_TEMPCAPSULEMODEL,
	CG_CM_POINTCONTENTS,
	CG_CM_TRANSFORMEDPOINTCONTENTS,
	CG_CM_BOXTRACE,
	CG_CM_CAPSULETRACE,
	CG_CM_TRANSFORMEDBOXTRACE,
	CG_CM_TRANSFORMEDCAPSULETRACE,
	CG_CM_MARKFRAGMENTS,
	CG_S_MUTESOUND,
	CG_S_STARTSOUND,
	CG_S_STARTLOCALSOUND,
	CG_S_CLEARLOOPINGSOUNDS,
	CG_S_ADDLOOPINGSOUND,
	CG_S_UPDATEENTITYPOSITION,
	CG_S_ADDREALLOOPINGSOUND,
	CG_S_STOPLOOPINGSOUND,
	CG_S_RESPATIALIZE,
	CG_S_REGISTERSOUND,
	CG_S_STARTBACKGROUNDTRACK,
	CG_R_LOADWORLDMAP,
	CG_R_REGISTERMODEL,
	CG_R_REGISTERSKIN,
	CG_R_REGISTERSHADER,
	CG_R_REGISTERSHADERNOMIP,
	CG_R_REGISTERFONT,
	CG_R_FONT_STRLENPIXELS,
	CG_R_FONT_STRLENCHARS,
	CG_R_FONT_STRHEIGHTPIXELS,
	CG_R_FONT_DRAWSTRING,
	CG_LANGUAGE_ISASIAN,
	CG_LANGUAGE_USESSPACES,
	CG_ANYLANGUAGE_READCHARFROMSTRING,

	CGAME_MEMSET = 100,
	CGAME_MEMCPY,
	CGAME_STRNCPY,
	CGAME_SIN,
	CGAME_COS,
	CGAME_ATAN2,
	CGAME_SQRT,
	CGAME_MATRIXMULTIPLY,
	CGAME_ANGLEVECTORS,
	CGAME_PERPENDICULARVECTOR,
	CGAME_FLOOR,
	CGAME_CEIL,
	CGAME_ACOS,
	CGAME_ASIN,

	CG_R_CLEARSCENE = 200,
	CG_R_ADDREFENTITYTOSCENE,
	CG_R_ADDPOLYTOSCENE,
	CG_R_ADDPOLYSTOSCENE,
	CG_R_LIGHTFORPOINT,
	CG_R_ADDLIGHTTOSCENE,
	CG_R_ADDADDITIVELIGHTTOSCENE,
	CG_R_RENDERSCENE,
	CG_R_SETCOLOR,
	CG_R_DRAWSTRETCHPIC,
	CG_R_MODELBOUNDS,
	CG_R_LERPTAG,
	CG_R_DRAWROTATEPIC,
	CG_R_DRAWROTATEPIC2,
	CG_R_REMAP_SHADER,
	CG_R_GET_LIGHT_STYLE,
	CG_R_SET_LIGHT_STYLE,
	CG_R_GET_BMODEL_VERTS,
	CG_FX_ADDLINE,

	CG_GETGLCONFIG,
	CG_GETGAMESTATE,
	CG_GETCURRENTSNAPSHOTNUMBER,
	CG_GETSNAPSHOT,
	CG_GETSERVERCOMMAND,
	CG_GETCURRENTCMDNUMBER,
	CG_GETUSERCMD,
	CG_SETUSERCMDVALUE,
	CG_SETCLIENTFORCEANGLE,
	CG_SETCLIENTTURNEXTENT,
	CG_OPENUIMENU,
	CG_TESTPRINTINT,
	CG_TESTPRINTFLOAT,
	CG_MEMORY_REMAINING,
	CG_KEY_ISDOWN,
	CG_KEY_GETCATCHER,
	CG_KEY_SETCATCHER,
	CG_KEY_GETKEY,

 	CG_PC_ADD_GLOBAL_DEFINE,
	CG_PC_LOAD_SOURCE,
	CG_PC_FREE_SOURCE,
	CG_PC_READ_TOKEN,
	CG_PC_SOURCE_FILE_AND_LINE,
	CG_PC_LOAD_GLOBAL_DEFINES,
	CG_PC_REMOVE_ALL_GLOBAL_DEFINES,

	CG_S_STOPBACKGROUNDTRACK,
	CG_REAL_TIME,
	CG_SNAPVECTOR,
	CG_CIN_PLAYCINEMATIC,
	CG_CIN_STOPCINEMATIC,
	CG_CIN_RUNCINEMATIC,
	CG_CIN_DRAWCINEMATIC,
	CG_CIN_SETEXTENTS,

	CG_GET_ENTITY_TOKEN,
	CG_R_INPVS,
	CG_FX_REGISTER_EFFECT,
	CG_FX_PLAY_SIMPLE_EFFECT,
	CG_FX_PLAY_EFFECT,
	CG_FX_PLAY_ENTITY_EFFECT,
	CG_FX_PLAY_SIMPLE_EFFECT_ID,
	CG_FX_PLAY_EFFECT_ID,
	CG_FX_PLAY_ENTITY_EFFECT_ID,
	CG_FX_PLAY_BOLTED_EFFECT_ID,
	CG_FX_ADD_SCHEDULED_EFFECTS,
	CG_FX_INIT_SYSTEM,
	CG_FX_FREE_SYSTEM,
	CG_FX_ADJUST_TIME,
	CG_FX_ADDPOLY,
	CG_FX_ADDBEZIER,
	CG_FX_ADDPRIMITIVE,
	CG_FX_ADDSPRITE,

	CG_SP_PRINT,
	CG_SP_GETSTRINGTEXTSTRING,
	CG_SP_REGISTER,

	CG_ROFF_CLEAN,
	CG_ROFF_UPDATE_ENTITIES,
	CG_ROFF_CACHE,
	CG_ROFF_PLAY,
	CG_ROFF_PURGE_ENT,

/*
Ghoul2 Insert Start
*/
	CG_G2_LISTSURFACES,
	CG_G2_LISTBONES,
	CG_G2_SETMODELS,
	CG_G2_HAVEWEGHOULMODELS,
	CG_G2_GIVEMEVECTORFROMMATRIX,
	CG_G2_GETBOLT,
	CG_G2_GETBOLT_NOREC,
	CG_G2_GETBOLT_NOREC_NOROT,
	CG_G2_INITGHOUL2MODEL,
	CG_G2_COLLISIONDETECT,
	CG_G2_CLEANMODELS,
	CG_G2_ANGLEOVERRIDE,
	CG_G2_PLAYANIM,
	CG_G2_GETGLANAME,
	CG_G2_COPYGHOUL2INSTANCE,
	CG_G2_COPYSPECIFICGHOUL2MODEL,
	CG_G2_DUPLICATEGHOUL2INSTANCE,
	CG_G2_HASGHOUL2MODELONINDEX,
	CG_G2_REMOVEGHOUL2MODEL,
	CG_G2_ADDBOLT,
	CG_G2_SETBOLTON,
	CG_G2_SETROOTSURFACE,
	CG_G2_SETSURFACEONOFF,
	CG_G2_SETNEWORIGIN,

	CG_SET_SHARED_BUFFER,
/*
Ghoul2 Insert End
*/
} cgameImport_t;


/*
==================================================================

functions exported to the main executable

==================================================================
*/

typedef enum {
	CG_INIT,
//	void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum )
	// called when the level loads or when the renderer is restarted
	// all media should be registered at this time
	// cgame will display loading status by calling SCR_Update, which
	// will call CG_DrawInformation during the loading process
	// reliableCommandSequence will be 0 on fresh loads, but higher for
	// demos, tourney restarts, or vid_restarts

	CG_SHUTDOWN,
//	void (*CG_Shutdown)( void );
	// oportunity to flush and close any open files

	CG_CONSOLE_COMMAND,
//	qboolean (*CG_ConsoleCommand)( void );
	// a console command has been issued locally that is not recognized by the
	// main game system.
	// use Cmd_Argc() / Cmd_Argv() to read the command, return qfalse if the
	// command is not known to the game

	CG_DRAW_ACTIVE_FRAME,
//	void (*CG_DrawActiveFrame)( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
	// Generates and draws a game scene and status information at the given time.
	// If demoPlayback is set, local movement prediction will not be enabled

	CG_CROSSHAIR_PLAYER,
//	int (*CG_CrosshairPlayer)( void );

	CG_LAST_ATTACKER,
//	int (*CG_LastAttacker)( void );

	CG_KEY_EVENT,
//	void	(*CG_KeyEvent)( int key, qboolean down );

	CG_MOUSE_EVENT,
//	void	(*CG_MouseEvent)( int dx, int dy );
	CG_EVENT_HANDLING,
//	void (*CG_EventHandling)(int type);

	CG_POINT_CONTENTS,
//	int	CG_PointContents( const vec3_t point, int passEntityNum );

	CG_GET_LERP_ORIGIN,
//	void CG_LerpOrigin(int num, vec3_t result);

	CG_GET_LERP_ANGLES,
	CG_GET_MODEL_SCALE,
	CG_GET_GHOUL2,
	CG_GET_MODEL_LIST,

	CG_CALC_LERP_POSITIONS,
//	void CG_CalcEntityLerpPositions(int num);

	CG_TRACE,
//void CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
//					 int skipNumber, int mask );

	CG_GET_ORIGIN,		// int entnum, vec3_t origin
	CG_GET_ANGLES,		// int entnum, vec3_t angle

	CG_GET_BOLT_POS,	// int entnum, vec3_t origin, vec3_t angle

	CG_GET_ORIGIN_TRAJECTORY,		// int entnum
	CG_GET_ANGLE_TRAJECTORY,		// int entnum

	CG_ROFF_NOTETRACK_CALLBACK,		// int entnum, char *notetrack

	CG_IMPACT_MARK,
//void CG_ImpactMark( qhandle_t markShader, const vec3_t origin, const vec3_t dir,
//				   float orientation, float red, float green, float blue, float alpha,
//				   qboolean alphaFade, float radius, qboolean temporary )

	CG_MAP_CHANGE,

} cgameExport_t;

// CG_POINT_CONTENTS
typedef struct
{
	vec3_t		mPoint;			// input
	int			mPassEntityNum;	// input
} TCGPointContents;

// CG_GET_BOLT_POS
typedef struct
{
	vec3_t		mPoint;			// output
	vec3_t		mAngle;			// output
	int			mEntityNum;		// input
} TCGBoltPos;

// CG_IMPACT_MARK
typedef struct
{
	int		mHandle;
	vec3_t	mPoint;
	vec3_t	mAngle;
	float	mRotation;
	float	mRed;
	float	mGreen;
	float	mBlue;
	float	mAlphaStart;
	float	mSizeStart;
} TCGImpactMark;

// CG_GET_LERP_ORIGIN
// CG_GET_LERP_ANGLES
// CG_GET_MODEL_SCALE
typedef struct
{
	int			mEntityNum;		// input
	vec3_t		mPoint;			// output
} TCGVectorData;

// CG_TRACE
typedef struct
{
	trace_t mResult;					// output
	vec3_t	mStart, mMins, mMaxs, mEnd;	// input
	int		mSkipNumber, mMask;			// input
} TCGTrace;

// CG_FX_CAMERASHAKE
typedef struct
{
	vec3_t	mOrigin;					// input
	float	mIntensity;					// input
	int		mRadius;					// input
	int		mTime;						// input
} TCGCameraShake;

// CG_MISC_ENT
typedef struct
{
	char	mModel[MAX_QPATH];			// input
	vec3_t	mOrigin, mAngles, mScale;	// input
} TCGMiscEnt;

typedef struct
{
	refEntity_t		ent;				// output
	void			*ghoul2;			// input
	int				modelIndex;			// input
	int				boltIndex;			// input
	vec3_t			origin;				// input
	vec3_t			angles;				// input
	vec3_t			modelScale;			// input
} TCGPositionOnBolt;

#define	MAX_CG_SHARED_BUFFER_SIZE		2048

//----------------------------------------------
