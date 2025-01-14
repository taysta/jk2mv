#include "tr_local.h"

volatile renderCommandList_t	*renderCommandList;

volatile qboolean	renderThreadActive;


/*
=====================
R_PerformanceCounters
=====================
*/
void R_PerformanceCounters( void ) {
	if ( !r_speeds->integer ) {
		// clear the counters even if we aren't printing
		Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
		Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
		return;
	}

	if (r_speeds->integer == 1) {
		const float texSize = R_SumOfUsedImages( qfalse )/(8*1048576.0f)*(r_texturebits->integer?r_texturebits->integer:glConfig.colorBits);
		ri.Printf (PRINT_ALL, "%i/%i shdrs/srfs %i leafs %i vrts %i/%i tris %.2fMB tex %.2f dc\n",
			backEnd.pc.c_shaders, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes,
			backEnd.pc.c_indexes/3, backEnd.pc.c_totalIndexes/3,
			texSize, backEnd.pc.c_overDraw / (float)(glConfig.vidWidth * glConfig.vidHeight) );
	} else if (r_speeds->integer == 2) {
		ri.Printf (PRINT_ALL, "(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out,
			tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out );
		ri.Printf (PRINT_ALL, "(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out,
			tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out );
	} else if (r_speeds->integer == 3) {
		ri.Printf (PRINT_ALL, "viewcluster: %i\n", tr.viewCluster );
	} else if (r_speeds->integer == 4) {
		if ( backEnd.pc.c_dlightVertexes ) {
			ri.Printf (PRINT_ALL, "dlight srf:%i  culled:%i  verts:%i  tris:%i\n",
				tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
				backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3 );
		}
	}
	else if (r_speeds->integer == 5 )
	{
		ri.Printf( PRINT_ALL, "zFar: %.0f\n", tr.viewParms.zFar );
	}
	else if (r_speeds->integer == 6 )
	{
		ri.Printf( PRINT_ALL, "flare adds:%i tests:%i renders:%i\n",
			backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders );
	}
	else if (r_speeds->integer == 7) {
		const float texSize = R_SumOfUsedImages(qtrue) / (1048576.0f);
		const float backBuff= glConfig.vidWidth * glConfig.vidHeight * glConfig.colorBits / (8.0f * 1024*1024);
		const float depthBuff= glConfig.vidWidth * glConfig.vidHeight * glConfig.depthBits / (8.0f * 1024*1024);
		const float stencilBuff= glConfig.vidWidth * glConfig.vidHeight * glConfig.stencilBits / (8.0f * 1024*1024);
		ri.Printf (PRINT_ALL, "Tex MB %.2f + buffers %.2f MB = Total %.2fMB\n",
			texSize, backBuff*2+depthBuff+stencilBuff, texSize+backBuff*2+depthBuff+stencilBuff);
	}

	Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
	Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}

/*
====================
R_IssueRenderCommands
====================
*/
int	c_blockedOnRender;
int	c_blockedOnMain;

void R_IssueRenderCommands( qboolean runPerformanceCounters ) {
	renderCommandList_t	*cmdList;

	cmdList = &backEndData->commands;
	assert(cmdList);
	// add an end-of-list command
	*(int *)(cmdList->cmds + cmdList->used) = RC_END_OF_LIST;

	// clear it out, in case this is a sync and not a buffer flip
	cmdList->used = 0;

	// at this point, the back end thread is idle, so it is ok
	// to look at it's performance counters
	if (runPerformanceCounters) {
		R_PerformanceCounters();
	}

	// actually start the commands going
	if (!r_skipBackEnd->integer && !com_minimized->integer) {
		// let it start on the new batch
		RB_ExecuteRenderCommands(cmdList->cmds);
	}
}


/*
====================
R_SyncRenderThread

Issue any pending commands and wait for them to complete.
====================
*/
void R_SyncRenderThread( void ) {
	if (!tr.registered) {
		return;
	}

	R_IssueRenderCommands(qfalse);
}

/*
============
R_GetCommandBufferReserved

make sure there is enough command space, waiting on the
render thread if needed.
============
*/
void *R_GetCommandBufferReserved( unsigned int bytes, int reservedBytes ) {
	renderCommandList_t	*cmdList;

	cmdList = &backEndData->commands;
	bytes = PAD( bytes, sizeof( void * ) );

	// always leave room for the end of list command
	if ( cmdList->used + bytes + sizeof( int ) + reservedBytes > MAX_RENDER_COMMANDS ) {
		if ( bytes > MAX_RENDER_COMMANDS - sizeof( int ) ) {
			ri.Error( ERR_FATAL, "R_GetCommandBuffer: bad size %i", bytes );
		}
		// if we run out of room, just start dropping commands
		return NULL;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;
}

/*
============
R_GetCommandBuffer

returns NULL if there is not enough space for important commands
============
*/
void *R_GetCommandBuffer( unsigned int bytes ) {
	return R_GetCommandBufferReserved( bytes, PAD( sizeof ( swapBuffersCommand_t ), sizeof( void * ) ) );
}


/*
=============
R_AddDrawSurfCmd

=============
*/
void	R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	drawSurfsCommand_t	*cmd;

	cmd = (drawSurfsCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_SURFS;

	cmd->drawSurfs = drawSurfs;
	cmd->numDrawSurfs = numDrawSurfs;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}


/*
=============
RE_SetColor

Passing NULL will set the color to white
=============
*/
void	RE_SetColor( const vec4_t rgba ) {
	setColorCommand_t	*cmd;

	cmd = (setColorCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SET_COLOR;
	if ( !rgba ) {
		static const float colorWhite[4] = { 1, 1, 1, 1 };

		rgba = colorWhite;
	}

	cmd->color[0] = rgba[0];
	cmd->color[1] = rgba[1];
	cmd->color[2] = rgba[2];
	cmd->color[3] = rgba[3];
}


/*
=============
RE_StretchPic

x, y, w and h are in virtual screen coordinates
xadjust is 640 / virtual screen width
yadjust is 480 / virtual screen height
=============
*/
void RE_StretchPic ( float x, float y, float w, float h, float s1, float t1,
	float s2, float t2, qhandle_t hShader, float xadjust, float yadjust )
{
	stretchPicCommand_t	*cmd;

	cmd = (stretchPicCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_STRETCH_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x * xadjust;
	cmd->y = y * yadjust;
	cmd->w = w * xadjust;
	cmd->h = h * yadjust;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

/*
=============
RE_RotatePic

x, y, w and h are in virtual screen coordinates
xadjust is 640 / virtual screen width
yadjust is 480 / virtual screen height
=============
*/
void RE_RotatePic ( float x, float y, float w, float h, float s1, float t1,
	float s2, float t2,float a, qhandle_t hShader, float xadjust, float yadjust ) {
	transformPicCommand_t	*cmd;
	float s, c;

	cmd = (transformPicCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}

	a = DEG2RAD( a );
	s = sinf( a );
	c = cosf( a );

	cmd->commandId = RC_TRANSFORM_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->m[0][0] = xadjust * w * c;
	cmd->m[0][1] = xadjust * h * -s;
	cmd->m[1][0] = yadjust * w * s;
	cmd->m[1][1] = yadjust * h * c;
	// rotate around top-right corner
	cmd->x = xadjust * x - cmd->m[0][0] + xadjust * w;
	cmd->y = yadjust * y - cmd->m[1][0];
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

/*
=============
RE_RotatePic2

x, y, w and h are in virtual screen coordinates
xadjust is 640 / virtual screen width
yadjust is 480 / virtual screen height
=============
*/
void RE_RotatePic2 ( float x, float y, float w, float h, float s1, float t1,
	float s2, float t2,float a, qhandle_t hShader, float xadjust, float yadjust ) {
	transformPicCommand_t	*cmd;
	float s, c;

	cmd = (transformPicCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}

	a = DEG2RAD( a );
	s = sinf( a );
	c = cosf( a );

	cmd->commandId = RC_TRANSFORM_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->m[0][0] = xadjust * w * c;
	cmd->m[0][1] = xadjust * h * -s;
	cmd->m[1][0] = yadjust * w * s;
	cmd->m[1][1] = yadjust * h * c;
	cmd->x = xadjust * x - 0.5f * (cmd->m[0][0] + cmd->m[0][1]);
	cmd->y = yadjust * y - 0.5f * (cmd->m[1][0] + cmd->m[1][1]);
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

/*
====================
RE_BeginFrame

If running in stereo, RE_BeginFrame will be called twice
for each RE_EndFrame
====================
*/
void RE_BeginFrame( stereoFrame_t stereoFrame ) {
	drawBufferCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	glState.finishCalled = qfalse;

	tr.frameCount++;
	tr.frameSceneNum = 0;

	//
	// do overdraw measurement
	//
	if ( r_measureOverdraw->integer )
	{
		if ( glConfig.stencilBits < 4 )
		{
			ri.Printf( PRINT_ALL, "Warning: not enough stencil bits to measure overdraw: %d\n", glConfig.stencilBits );
			ri.Cvar_Set( "r_measureOverdraw", "0" );
			r_measureOverdraw->modified = qfalse;
		}
		else if ( r_shadows->integer == 2 )
		{
			ri.Printf( PRINT_ALL, "Warning: stencil shadows and overdraw measurement are mutually exclusive\n" );
			ri.Cvar_Set( "r_measureOverdraw", "0" );
			r_measureOverdraw->modified = qfalse;
		}
		else
		{
			R_SyncRenderThread();
			qglEnable( GL_STENCIL_TEST );
			qglStencilMask( ~0U );
			qglClearStencil( 0U );
			qglStencilFunc( GL_ALWAYS, 0U, ~0U );
			qglStencilOp( GL_KEEP, GL_INCR, GL_INCR );
		}
		r_measureOverdraw->modified = qfalse;
	}
	else
	{
		// this is only reached if it was on and is now off
		if ( r_measureOverdraw->modified ) {
			R_SyncRenderThread();
			qglDisable( GL_STENCIL_TEST );
		}
		r_measureOverdraw->modified = qfalse;
	}

	//
	// texturemode stuff
	//
	if ( r_textureMode->modified || r_ext_texture_filter_anisotropic->modified) {
		R_SyncRenderThread();
		GL_TextureMode( r_textureMode->string );
		r_textureMode->modified = qfalse;
		r_ext_texture_filter_anisotropic->modified = qfalse;
	}

	if ( glConfig.textureLODBiasAvailable && r_textureLODBias->modified ) {
		qglTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, r_textureLODBias->value );
		r_textureLODBias->modified = qfalse;
	}

	//
	// gamma stuff
	//
	if ( r_gamma->modified ) {
		r_gamma->modified = qfalse;

		R_SyncRenderThread();
		R_SetColorMappings();
	}

	if (r_overBrightBits->modified) {
		char mapname[MAX_QPATH] = {0};

		R_SyncRenderThread();
		R_SetColorMappings();

		if (tr.world)
		{
			Q_strncpyz(mapname, tr.world->name, sizeof(mapname));
			tr.worldMapLoaded = qfalse;
			tr.world = NULL;
			RE_LoadWorldMap(mapname);
		}

		//reset these last...
		if (r_overBrightBits->modified)
			r_overBrightBits->modified = qfalse;
	}

	//
	// console font stuff
	//
	if ( r_consoleFont->modified ) {
		r_consoleFont->modified = qfalse;

		if ( r_consoleFont->integer == 1 )
			R_RemapShader("gfx/2d/charsgrid_med", "gfx/2d/code_new_roman", 0);
		else if ( r_consoleFont->integer == 2 )
			R_RemapShader("gfx/2d/charsgrid_med", "gfx/2d/mplus_1m_bold", 0);
		else
			R_RemapShader("gfx/2d/charsgrid_med", "gfx/2d/charsgrid_med", 0);
	}

	// check for errors
	if ( !r_ignoreGLErrors->integer ) {
		int	err;

		R_SyncRenderThread();
		if ( ( err = qglGetError() ) != GL_NO_ERROR ) {
			ri.Error( ERR_FATAL, "RE_BeginFrame() - glGetError() failed (0x%x)!", err );
		}
	}

	//
	// draw buffer stuff
	//
	cmd = (drawBufferCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_BUFFER;

	if ( glConfig.stereoEnabled ) {
		if ( stereoFrame == STEREO_LEFT ) {
			cmd->buffer = (int)GL_BACK_LEFT;
		} else if ( stereoFrame == STEREO_RIGHT ) {
			cmd->buffer = (int)GL_BACK_RIGHT;
		} else {
			ri.Error( ERR_FATAL, "RE_BeginFrame: Stereo is enabled, but stereoFrame was %i", stereoFrame );
		}
	} else {
		if ( stereoFrame != STEREO_CENTER ) {
			ri.Error( ERR_FATAL, "RE_BeginFrame: Stereo is disabled, but stereoFrame was %i", stereoFrame );
		}
		if ( !Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) ) {
			cmd->buffer = (int)GL_FRONT;
		} else {
			cmd->buffer = (int)GL_BACK;
		}
	}
}

/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
void RE_EndFrame( int *frontEndMsec, int *backEndMsec ) {
	swapBuffersCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	cmd = (swapBuffersCommand_t *)R_GetCommandBufferReserved( sizeof( *cmd ), 0 );
	if (!cmd) {
		return;
	}

	cmd->commandId = RC_SWAP_BUFFERS;
	R_IssueRenderCommands( qtrue );

	R_InitNextFrame();

	if ( frontEndMsec ) {
		*frontEndMsec = tr.frontEndMsec;
	}
	tr.frontEndMsec = 0;
	if ( backEndMsec ) {
		*backEndMsec = backEnd.pc.msec;
	}
	backEnd.pc.msec = 0;
}


/*
=============
RE_TakeVideoFrame
=============
*/
void RE_TakeVideoFrame( int width, int height, qboolean motionJpeg, int motionJpegQuality )
{
	videoFrameCommand_t	*cmd;

	if( !tr.registered ) {
		return;
	}

	cmd = (videoFrameCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if( !cmd ) {
		return;
	}

	cmd->commandId = RC_VIDEOFRAME;

	cmd->width = width;
	cmd->height = height;
	cmd->motionJpeg = motionJpeg;
	cmd->motionJpegQuality = motionJpegQuality;
}
