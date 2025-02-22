// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

unsigned	frame_msec;
int			old_com_frameTime;

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as argv(1) so it can be matched up with the release.

argv(2) will be set to the time the event happened, which allows exact
control even at low framerates when the down and up events may both get qued
at the same time.

===============================================================================
*/


kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_strafe, in_speed;
kbutton_t	in_up, in_down;

kbutton_t	in_buttons[16];


qboolean	in_mlooking;


void IN_MLookDown( void ) {
	in_mlooking = qtrue;
}

void IN_MLookUp( void ) {
	in_mlooking = qfalse;
	if ( !cl_freelook->integer ) {
		IN_CenterView ();
	}
}

void IN_GenCMD1( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_SABERSWITCH;
}

void IN_GenCMD2( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_ENGAGE_DUEL;
}

void IN_GenCMD3( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_HEAL;
}

void IN_GenCMD4( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_SPEED;
}

void IN_GenCMD5( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_PULL;
}

void IN_GenCMD6( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_DISTRACT;
}

void IN_GenCMD7( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_RAGE;
}

void IN_GenCMD8( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_PROTECT;
}

void IN_GenCMD9( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_ABSORB;
}

void IN_GenCMD10( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_HEALOTHER;
}

void IN_GenCMD11( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_FORCEPOWEROTHER;
}

void IN_GenCMD12( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_SEEING;
}

void IN_GenCMD13( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_SEEKER;
}

void IN_GenCMD14( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_FIELD;
}

void IN_GenCMD15( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_BACTA;
}

void IN_GenCMD16( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_ELECTROBINOCULARS;
}

void IN_GenCMD17( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_ZOOM;
}

void IN_GenCMD18( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_SENTRY;
}

void IN_GenCMD19( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_SABERATTACKCYCLE;
}

void IN_GenCMD20( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_THROW;
}

void IN_KeyDown( kbutton_t *b ) {
	int		k;
	char	*c;

	c = Cmd_Argv(1);
	if ( c[0] ) {
		k = atoi(c);
	} else {
		k = -1;		// typed manually at the console for continuous down
	}

	if ( k == b->down[0] || k == b->down[1] ) {
		return;		// repeating key
	}

	if ( !b->down[0] ) {
		b->down[0] = k;
	} else if ( !b->down[1] ) {
		b->down[1] = k;
	} else {
		Com_Printf ("Three keys down for a button!\n");
		return;
	}

	if ( b->active ) {
		return;		// still down
	}

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	b->downtime = atoi(c);

	b->active = qtrue;
	b->wasPressed = qtrue;
}

void IN_KeyUp( kbutton_t *b ) {
	int		k;
	char	*c;
	unsigned	uptime;

	c = Cmd_Argv(1);
	if ( c[0] ) {
		k = atoi(c);
	} else {
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->active = qfalse;
		return;
	}

	if ( b->down[0] == k ) {
		b->down[0] = 0;
	} else if ( b->down[1] == k ) {
		b->down[1] = 0;
	} else {
		return;		// key up without coresponding down (menu pass through)
	}
	if ( b->down[0] || b->down[1] ) {
		return;		// some other key is still holding it down
	}

	b->active = qfalse;

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	uptime = atoi(c);
	if ( uptime ) {
		b->msec += uptime - b->downtime;
	} else {
		b->msec += frame_msec / 2;
	}

	b->active = qfalse;
}



/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
float CL_KeyState( kbutton_t *key ) {
	float		val;
	int			msec;

	msec = key->msec;
	key->msec = 0;

	if ( key->active ) {
		// still down
		if ( !key->downtime ) {
			msec = com_frameTime;
		} else {
			msec += com_frameTime - key->downtime;
		}
		if (!cl_idrive->integer)
			key->downtime = com_frameTime;//Loda - Not sure what the fuck this is doing here, downtime is supposed to store time of when the key was initially pressed, not the most recent time its been held down..
	}

#if 0
	if (msec) {
		Com_Printf ("%i ", msec);
	}
#endif

	val = (float)msec / frame_msec;
	if ( val < 0 ) {
		val = 0;
	}
	if ( val > 1 ) {
		val = 1;
	}

	return val;
}

//do we have any force powers that we would normally need to cycle to?
//dumb clone of the qbool check in cg_main, don't want to break compatibility for 1 silly VM call
qboolean CL_NoUseableForce(void)
{
	int i = FP_HEAL;
	while (i < NUM_FORCE_POWERS)
	{
		if (i != FP_SABERTHROW &&
			i != FP_SABERATTACK &&
			i != FP_SABERDEFEND &&
			i != FP_LEVITATION)
		{ //valid selectable power
			if (cl.snap.ps.fd.forcePowersKnown & (1 << i))
			{ //we have it
				return qfalse;
			}
		}
		i++;
	}

	//no useable force powers, I guess.
	return qtrue;
}

void IN_UpDown(void) {IN_KeyDown(&in_up);}
void IN_UpUp(void) {IN_KeyUp(&in_up);}
void IN_DownDown(void) {IN_KeyDown(&in_down);}
void IN_DownUp(void) {IN_KeyUp(&in_down);}
void IN_LeftDown(void) {IN_KeyDown(&in_left);}
void IN_LeftUp(void) {IN_KeyUp(&in_left);}
void IN_RightDown(void) {IN_KeyDown(&in_right);}
void IN_RightUp(void) {IN_KeyUp(&in_right);}
void IN_ForwardDown(void) {IN_KeyDown(&in_forward);}
void IN_ForwardUp(void) {IN_KeyUp(&in_forward);}
void IN_BackDown(void) {IN_KeyDown(&in_back);}
void IN_BackUp(void) {IN_KeyUp(&in_back);}
void IN_LookupDown(void) {IN_KeyDown(&in_lookup);}
void IN_LookupUp(void) {IN_KeyUp(&in_lookup);}
void IN_LookdownDown(void) {IN_KeyDown(&in_lookdown);}
void IN_LookdownUp(void) {IN_KeyUp(&in_lookdown);}
void IN_MoveleftDown(void) {IN_KeyDown(&in_moveleft);}
void IN_MoveleftUp(void) {IN_KeyUp(&in_moveleft);}
void IN_MoverightDown(void) {IN_KeyDown(&in_moveright);}
void IN_MoverightUp(void) {IN_KeyUp(&in_moveright);}

void IN_SpeedDown(void) {IN_KeyDown(&in_speed);}
void IN_SpeedUp(void) {IN_KeyUp(&in_speed);}
void IN_StrafeDown(void) {IN_KeyDown(&in_strafe);}
void IN_StrafeUp(void) {IN_KeyUp(&in_strafe);}

void IN_Button0Down(void) {IN_KeyDown(&in_buttons[0]);}
void IN_Button0Up(void) {IN_KeyUp(&in_buttons[0]);}
void IN_Button1Down(void) {IN_KeyDown(&in_buttons[1]);}
void IN_Button1Up(void) {IN_KeyUp(&in_buttons[1]);}
void IN_Button2Down(void) {IN_KeyDown(&in_buttons[2]);}
void IN_Button2Up(void) {IN_KeyUp(&in_buttons[2]);}
void IN_Button3Down(void) {IN_KeyDown(&in_buttons[3]);}
void IN_Button3Up(void) {IN_KeyUp(&in_buttons[3]);}
void IN_Button4Down(void) {IN_KeyDown(&in_buttons[4]);}
void IN_Button4Up(void) {IN_KeyUp(&in_buttons[4]);}
void IN_Button5Down(void) {IN_KeyDown(&in_buttons[5]);}
void IN_Button5Up(void) {IN_KeyUp(&in_buttons[5]);}
void IN_Button6Down(void) {IN_KeyDown(&in_buttons[6]);}
void IN_Button6Up(void) {IN_KeyUp(&in_buttons[6]);}
void IN_Button7Down(void) {IN_KeyDown(&in_buttons[7]);}
void IN_Button7Up(void) {IN_KeyUp(&in_buttons[7]);}
void IN_Button8Down(void) {IN_KeyDown(&in_buttons[8]);}
void IN_Button8Up(void) {IN_KeyUp(&in_buttons[8]);}
void IN_Button9Down(void) {IN_KeyDown(&in_buttons[9]);}
void IN_Button9Up(void) {IN_KeyUp(&in_buttons[9]);}
void IN_Button10Down(void) {IN_KeyDown(&in_buttons[10]);}
void IN_Button10Up(void) {IN_KeyUp(&in_buttons[10]);}
void IN_Button11Down(void) {IN_KeyDown(&in_buttons[11]);}
void IN_Button11Up(void) {IN_KeyUp(&in_buttons[11]);}
void IN_Button12Down(void) {IN_KeyDown(&in_buttons[12]);}
void IN_Button12Up(void) {IN_KeyUp(&in_buttons[12]);}
void IN_Button13Down(void) {IN_KeyDown(&in_buttons[13]);}
void IN_Button13Up(void) {IN_KeyUp(&in_buttons[13]);}
void IN_Button14Down(void) {IN_KeyDown(&in_buttons[14]);}
void IN_Button14Up(void) {IN_KeyUp(&in_buttons[14]);}
void IN_Button15Down(void) {IN_KeyDown(&in_buttons[15]);}
void IN_Button15Up(void) {IN_KeyUp(&in_buttons[15]);}

void IN_ButtonDown (void) {
	IN_KeyDown(&in_buttons[1]);}
void IN_ButtonUp (void) {
	IN_KeyUp(&in_buttons[1]);}

void IN_CenterView (void) {
	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.snap.ps.delta_angles[PITCH]);
}


//==========================================================================

cvar_t	*cl_upspeed;
cvar_t	*cl_forwardspeed;
cvar_t	*cl_sidespeed;

cvar_t	*cl_yawspeed;
cvar_t	*cl_pitchspeed;

cvar_t	*cl_run;

cvar_t	*cl_anglespeedkey;


/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles( void ) {
	float	speed;

	if ( in_speed.active ) {
		speed = 0.001f * cls.frametime * cl_anglespeedkey->value;
	} else {
		speed = 0.001f * cls.frametime;
	}

	if ( !in_strafe.active ) {
		cl.viewangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&in_right);
		cl.viewangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&in_left);
	}

	cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&in_lookup);
	cl.viewangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&in_lookdown);
}

/*
================
CL_KeyMove

Sets the usercmd_t based on key states
================
*/
void CL_KeyMove( usercmd_t *cmd ) {
	int		movespeed;
	int		forward, side, up;
	float	s1, s2;

	//
	// adjust for speed key / running
	// the walking flag is to keep animations consistant
	// even during acceleration and develeration
	//
	if ( in_speed.active ^ cl_run->integer ) {
		movespeed = 127;
		cmd->buttons &= ~BUTTON_WALKING;
	} else {
		cmd->buttons |= BUTTON_WALKING;
		movespeed = 46;
	}

	forward = 0;
	side = 0;
	up = 0;

	if (in_strafe.active) {
		s1 = CL_KeyState(&in_right);
		s2 = CL_KeyState(&in_left);
		if (cl_idrive->integer && cl_idrive->integer != 2) {
			if (s1 && s2) {
				if (in_right.downtime > in_left.downtime)
					s2 = 0;
				if (in_right.downtime < in_left.downtime)
					s1 = 0;
			}
		}
		side += movespeed * s1;
		side -= movespeed * s2;
	}

	s1 = CL_KeyState(&in_moveright);
	s2 = CL_KeyState(&in_moveleft);
	if (cl_idrive->integer && cl_idrive->integer != 2) {
		if (s1 && s2) {
			if (in_moveright.downtime > in_moveleft.downtime)
				s2 = 0;
			if (in_moveright.downtime < in_moveleft.downtime)
				s1 = 0;
		}
	}
	side += movespeed * s1;
	side -= movespeed * s2;

	s1 = CL_KeyState (&in_up);
	s2 = CL_KeyState (&in_down);
	if (cl_idrive->integer || cl_idrive->integer == 2) {
		if (s1 && s2) {
			if (in_up.downtime > in_down.downtime)
				s2 = 0;
			if (in_up.downtime < in_down.downtime)
				s1 = 0;
		}
	}
	up += movespeed * s1;
	up -= movespeed * s2;

	s1 = CL_KeyState (&in_forward);
	s2 = CL_KeyState (&in_back);
	if (cl_idrive->integer && cl_idrive->integer != 2) {
		if (s1 && s2) {
			if (in_forward.downtime > in_back.downtime)
				s2 = 0;
			if (in_forward.downtime < in_back.downtime)
				s1 = 0;
		}
	}
	forward += movespeed * s1;
	forward -= movespeed * s2;	

	cmd->forwardmove = ClampChar( forward );
	cmd->rightmove = ClampChar( side );
	cmd->upmove = ClampChar( up );
}

/*
=================
CL_MouseEvent
=================
*/
void CL_MouseEvent( int dx, int dy, int time ) {
	if ( cls.keyCatchers & KEYCATCH_UI ) {
		VM_Call(uivm, UI_MOUSE_EVENT, dx, dy);
	} else if (cls.keyCatchers & KEYCATCH_CGAME) {
		VM_Call (cgvm, CG_MOUSE_EVENT, dx, dy);
	} else {
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
}

/*
=================
CL_JoystickEvent

Joystick values stay set until changed
=================
*/
void CL_JoystickEvent( int axis, int value, int time ) {
	if ( axis < 0 || axis >= MAX_JOYSTICK_AXIS ) {
		Com_Error( ERR_DROP, "CL_JoystickEvent: bad axis %i", axis );
	}
	cl.joystickAxis[axis] = value;
}

/*
=================
CL_JoystickMove
=================
*/
void CL_JoystickMove( usercmd_t *cmd ) {
	int		movespeed;
	float	anglespeed;

	if ( in_speed.active ^ cl_run->integer ) {
		movespeed = 2;
	} else {
		movespeed = 1;
		cmd->buttons |= BUTTON_WALKING;
	}

	if ( in_speed.active ) {
		anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	} else {
		anglespeed = 0.001 * cls.frametime;
	}

	if ( !in_strafe.active ) {
		cl.viewangles[YAW] += anglespeed * cl_yawspeed->value * cl.joystickAxis[AXIS_SIDE];
	} else {
		cmd->rightmove = ClampChar( cmd->rightmove + cl.joystickAxis[AXIS_SIDE] );
	}

	if ( in_mlooking ) {
		cl.viewangles[PITCH] += anglespeed * cl_pitchspeed->value * cl.joystickAxis[AXIS_FORWARD];
	} else {
		cmd->forwardmove = ClampChar( cmd->forwardmove + cl.joystickAxis[AXIS_FORWARD] );
	}

	cmd->upmove = ClampChar( cmd->upmove + cl.joystickAxis[AXIS_UP] );
}

/*
=================
CL_MouseMove
=================
*/
void CL_MouseMove( usercmd_t *cmd ) {
	float	mx, my;
	float	accelSensitivity;
	float	rate;

	// allow mouse smoothing
	if ( m_filter->integer ) {
		mx = ( cl.mouseDx[0] + cl.mouseDx[1] ) * 0.5;
		my = ( cl.mouseDy[0] + cl.mouseDy[1] ) * 0.5;
	} else {
		mx = cl.mouseDx[cl.mouseIndex];
		my = cl.mouseDy[cl.mouseIndex];
	}
	cl.mouseIndex ^= 1;
	cl.mouseDx[cl.mouseIndex] = 0;
	cl.mouseDy[cl.mouseIndex] = 0;

	rate = sqrtf( mx * mx + my * my ) / frame_msec;
	accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;

	// scale by FOV
	accelSensitivity *= cl.cgameSensitivity;

	if ( rate && cl_showMouseRate->integer ) {
		Com_Printf( "%f : %f\n", rate, accelSensitivity );
	}

	mx *= accelSensitivity;
	my *= accelSensitivity;

	if (!mx && !my) {
		return;
	}

	// add mouse X/Y movement to cmd
	if ( in_strafe.active ) {
		cmd->rightmove = ClampChar( cmd->rightmove + m_side->value * mx );
	} else {
		cl.viewangles[YAW] -= m_yaw->value * mx;
	}

	if ( (in_mlooking || cl_freelook->integer) && !in_strafe.active ) {
		cl.viewangles[PITCH] += m_pitch->value * my;
	} else {
		cmd->forwardmove = ClampChar( cmd->forwardmove - m_forward->value * my );
	}
}


/*
==============
CL_CmdButtons
==============
*/
void CL_CmdButtons( usercmd_t *cmd ) {
	int		i;

	//
	// figure button bits
	// send a button bit even if the key was pressed and released in
	// less than a frame
	//
	for (i = 0 ; i < 15 ; i++) {
		if ( in_buttons[i].active || in_buttons[i].wasPressed ) {
			cmd->buttons |= 1 << i;
		}
		in_buttons[i].wasPressed = qfalse;
	}

	if (cmd->buttons & BUTTON_FORCEPOWER)
	{
		//check for transferring a use force to a use inventory...
		if ((cmd->buttons & BUTTON_USE) || CL_NoUseableForce())
		{ //it's pushed, remap it!
			cmd->buttons &= ~BUTTON_FORCEPOWER;
			cmd->buttons |= BUTTON_USE_HOLDABLE;
		}
	}

	if ( cls.keyCatchers || com_unfocused->integer || com_minimized->integer ) {
		cmd->buttons |= BUTTON_TALK;
	}

	// allow the game to know if any key at all is
	// currently pressed, even if it isn't bound to anything
	if ( kg.anykeydown && !cls.keyCatchers ) {
		cmd->buttons |= BUTTON_ANY;
	}
}


/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove( usercmd_t *cmd ) {
	int		i;
	qboolean didForce = qfalse;

	// copy the state that the cgame is currently sending
	cmd->weapon = cl.cgameUserCmdValue;
	cmd->forcesel = cl.cgameForceSelection;
	cmd->invensel = cl.cgameInvenSelection;

	if (cl.gcmdSendValue)
	{
		cmd->generic_cmd = cl.gcmdValue;
		cl.gcmdSendValue = qfalse;
	}
	else
	{
		cmd->generic_cmd = 0;
	}

	// send the current server time so the amount of movement
	// can be determined without allowing cheating
	cmd->serverTime = cl.serverTime;

	if (cl.cgameViewAngleForceTime > cl.serverTime)
	{
		cl.cgameViewAngleForce[YAW] -= SHORT2ANGLE(cl.snap.ps.delta_angles[YAW]);

		cl.viewangles[YAW] = cl.cgameViewAngleForce[YAW];
		cl.cgameViewAngleForceTime = 0;

		didForce = qtrue;
	}

	if (cl.viewangles[YAW] < 0)
	{
		cl.viewangles[YAW] += 360;
		cl.lastViewYaw += 360;
	}
	if (cl.viewangles[YAW] > 360)
	{
		cl.viewangles[YAW] -= 360;
		cl.lastViewYaw -= 360;
	}

	if (!didForce && cl.cgameTurnExtentTime > cl.serverTime)
	{ //Do not allow crossing the extents in any direction
		qboolean forceBack = qfalse;
		float extentAdd = cl.cgameTurnExtentAdd;
		float extentSub = cl.cgameTurnExtentSub;

		extentAdd -= SHORT2ANGLE(cl.snap.ps.delta_angles[YAW]);
		extentSub -= SHORT2ANGLE(cl.snap.ps.delta_angles[YAW]);

		if (extentAdd < 0)
		{
			extentAdd += 360;
		}
		if (extentSub < 0)
		{
			extentSub += 360;
		}

		if (cl.viewangles[YAW] > extentAdd &&
			cl.lastViewYaw < extentAdd)
		{
			forceBack = qtrue;
		}
		else if (cl.viewangles[YAW] > extentSub &&
			cl.lastViewYaw < extentSub)
		{
			forceBack = qtrue;
		}
		else if (cl.viewangles[YAW] < extentAdd &&
			cl.lastViewYaw > extentAdd)
		{
			forceBack = qtrue;
		}
		else if (cl.viewangles[YAW] < extentSub &&
			cl.lastViewYaw > extentSub)
		{
			forceBack = qtrue;
		}

		if (forceBack)
		{
			cl.viewangles[YAW] = cl.lastViewYaw;
		}

		cl.cgameTurnExtentTime = 0;
	}

	cl.lastViewYaw = cl.viewangles[YAW];

	for (i=0 ; i<3 ; i++) {
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);
	}
}


/*
=================
CL_CreateCmd
=================
*/
usercmd_t CL_CreateCmd( void ) {
	usercmd_t	cmd;
	vec3_t		oldAngles;

	VectorCopy( cl.viewangles, oldAngles );

	// keyboard angle adjustment
	CL_AdjustAngles ();

	Com_Memset( &cmd, 0, sizeof( cmd ) );

	CL_CmdButtons( &cmd );

	// get basic movement from keyboard
	CL_KeyMove( &cmd );

	// get basic movement from mouse
	CL_MouseMove( &cmd );

	// get basic movement from joystick
	CL_JoystickMove( &cmd );

	// check to make sure the angles haven't wrapped
	if ( cl.viewangles[PITCH] - oldAngles[PITCH] > 90 ) {
		cl.viewangles[PITCH] = oldAngles[PITCH] + 90;
	} else if ( oldAngles[PITCH] - cl.viewangles[PITCH] > 90 ) {
		cl.viewangles[PITCH] = oldAngles[PITCH] - 90;
	}

	// store out the final values
	CL_FinishMove( &cmd );

	// draw debug graphs of turning for mouse testing
	if ( cl_debugMove->integer ) {
		if ( cl_debugMove->integer == 1 ) {
			SCR_DebugGraph( abs((int)(cl.viewangles[YAW] - oldAngles[YAW])), 0 );
		}
		if ( cl_debugMove->integer == 2 ) {
			SCR_DebugGraph( abs((int)(cl.viewangles[PITCH] - oldAngles[PITCH])), 0 );
		}
	}

	return cmd;
}


/*
=================
CL_CreateNewCommands

Create a new usercmd_t structure for this frame
=================
*/
void CL_CreateNewCommands( void ) {
	usercmd_t	*cmd;
	int			cmdNum;

	const int REAL_CMD_MASK = (cl_commandsize->integer >= 4 && cl_commandsize->integer <= 512) ? (cl_commandsize->integer - 1) : (CMD_MASK);//Loda - FPS UNLOCK ENGINE

	// no need to create usercmds until we have a gamestate
	if ( cls.state < CA_PRIMED ) {
		return;
	}

	frame_msec = com_frameTime - old_com_frameTime;

	// if running over 1000fps, act as if each frame is 1ms
	// prevents division by zero
	if ( frame_msec < 1 ) {
		frame_msec = 1;
	}

	// if running less than 5fps, truncate the extra time to prevent
	// unexpected moves after a hitch
	if ( frame_msec > 200 ) {
		frame_msec = 200;
	}
	old_com_frameTime = com_frameTime;


	// generate a command for this frame
	cl.cmdNumber++;
	cmdNum = cl.cmdNumber & REAL_CMD_MASK;//Loda - FPS UNLOCK ENGINE
	cl.cmds[cmdNum] = CL_CreateCmd ();
	cmd = &cl.cmds[cmdNum];
}

/*
=================
CL_ReadyToSendPacket

Returns qfalse if we are over the maxpackets limit
and should choke back the bandwidth a bit by not sending
a packet this frame.  All the commands will still get
delivered in the next packet, but saving a header and
getting more delta compression will reduce total bandwidth.
=================
*/
qboolean CL_ReadyToSendPacket( void ) {
	int		oldPacketNum;
	int		delta;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || cls.state == CA_CINEMATIC ) {
		return qfalse;
	}

	// If we are downloading, we send no less than 50ms between packets
	if ( *clc.downloadTempName &&
		cls.realtime - clc.lastPacketSentTime < 50 ) {
		return qfalse;
	}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if ( cls.state != CA_ACTIVE &&
		cls.state != CA_PRIMED &&
		!*clc.downloadTempName &&
		cls.realtime - clc.lastPacketSentTime < 1000 ) {
		return qfalse;
	}

	// send every frame for loopbacks
	if ( clc.netchan.remoteAddress.type == NA_LOOPBACK ) {
		return qtrue;
	}

	// send every frame for LAN
	if ( Sys_IsLANAddress( clc.netchan.remoteAddress ) ) {
		return qtrue;
	}

	// check for exceeding cl_maxpackets
	if ( cl_maxpackets->integer < 1 ) {
		Cvar_Set( "cl_maxpackets", "1" );
	} else if ( cl_maxpackets->integer > 2000 ) {
		Cvar_Set( "cl_maxpackets", "2000" );
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1) & PACKET_MASK;
	delta = cls.realtime -  cl.outPackets[ oldPacketNum ].p_realtime;
	if ( delta < 1000 / cl_maxpackets->integer ) {
		// the accumulated commands will go out in the next packet
		return qfalse;
	}

	return qtrue;
}

/*
===================
CL_WritePacket

Create and send the command packet to the server
Including both the reliable commands and the usercmds

During normal gameplay, a client packet will contain something like:

4	sequence number
2	qport
4	serverid
4	acknowledged sequence number
4	clc.serverCommandSequence
<optional reliable commands>
1	clc_move or clc_moveNoDelta
1	command count
<count * usercmds>

===================
*/
void CL_WritePacket( void ) {
	msg_t		buf;
	byte		data[MAX_MSGLEN];
	int			i, j;
	usercmd_t	*cmd, *oldcmd;
	usercmd_t	nullcmd;
	int			packetNum;
	int			oldPacketNum;
	int			count, key;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || cls.state == CA_CINEMATIC ) {
		return;
	}

	Com_Memset( &nullcmd, 0, sizeof(nullcmd) );
	oldcmd = &nullcmd;

	MSG_Init( &buf, data, sizeof(data) );

	MSG_Bitstream( &buf );
	// write the current serverId so the server
	// can tell if this is from the current gameState
	MSG_WriteLong( &buf, cl.serverId );

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	MSG_WriteLong( &buf, clc.serverMessageSequence );

	// write the last reliable message we received
	MSG_WriteLong( &buf, clc.serverCommandSequence );

	// write any unacknowledged clientCommands
	for ( i = clc.reliableAcknowledge + 1 ; i <= clc.reliableSequence ; i++ ) {
		MSG_WriteByte( &buf, clc_clientCommand );
		MSG_WriteLong( &buf, i );
		MSG_WriteString( &buf, clc.reliableCommands[ i & (MAX_RELIABLE_COMMANDS-1) ] );
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if ( cl_packetdup->integer < 0 ) {
		Cvar_Set( "cl_packetdup", "0" );
	} else if ( cl_packetdup->integer > 5 ) {
		Cvar_Set( "cl_packetdup", "5" );
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1 - cl_packetdup->integer) & PACKET_MASK;
	count = cl.cmdNumber - cl.outPackets[ oldPacketNum ].p_cmdNumber;
	if ( count > MAX_PACKET_USERCMDS ) {
		count = MAX_PACKET_USERCMDS;
		Com_Printf("MAX_PACKET_USERCMDS\n");
	}
	if ( count >= 1 ) {
		const int REAL_CMD_MASK = (cl_commandsize->integer >= 4 && cl_commandsize->integer <= 512) ? (cl_commandsize->integer - 1) : (CMD_MASK);//Loda - FPS UNLOCK ENGINE
		if ( cl_showSend->integer ) {
			Com_Printf( "(%i)", count );
		}

		// begin a client move command
		if ( cl_nodelta->integer || !cl.snap.valid || clc.demowaiting
			|| clc.serverMessageSequence != cl.snap.messageNum ) {
			MSG_WriteByte (&buf, clc_moveNoDelta);
		} else {
			MSG_WriteByte (&buf, clc_move);
		}

		// write the command count
		MSG_WriteByte( &buf, count );

		// use the checksum feed in the key
		key = clc.checksumFeed;
		// also use the message acknowledge
		key ^= clc.serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey(clc.serverCommands[ clc.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1) ], 32);

		// write all the commands, including the predicted command
		for ( i = 0 ; i < count ; i++ ) {
			j = (cl.cmdNumber - count + i + 1) & REAL_CMD_MASK;//Loda - FPS UNLOCK ENGINE
			cmd = &cl.cmds[j];
			MSG_WriteDeltaUsercmdKey (&buf, key, oldcmd, cmd);
			oldcmd = cmd;
		}
	}

	//
	// deliver the message
	//
	packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
	cl.outPackets[ packetNum ].p_realtime = cls.realtime;
	cl.outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
	cl.outPackets[ packetNum ].p_cmdNumber = cl.cmdNumber;
	clc.lastPacketSentTime = cls.realtime;

	if ( cl_showSend->integer ) {
		Com_Printf( "%i ", buf.cursize );
	}

	CL_Netchan_Transmit (&clc.netchan, &buf);

	// clients never really should have messages large enough
	// to fragment, but in case they do, fire them all off
	// at once
	while ( clc.netchan.unsentFragments ) {
		CL_Netchan_TransmitNextFragment( &clc.netchan );
	}
}

/*
=================
CL_SendCmd

Called every frame to builds and sends a command packet to the server.
=================
*/
void CL_SendCmd( void ) {
	// don't send any message if not connected
	if ( cls.state < CA_CONNECTED ) {
		return;
	}

	// don't send commands if paused
	if ( com_sv_running->integer && sv_paused->integer && cl_paused->integer ) {
		return;
	}

	// we create commands even if a demo is playing,
	CL_CreateNewCommands();

	// don't send a packet if the last packet was sent too recently
	if ( !CL_ReadyToSendPacket() ) {
		if ( cl_showSend->integer ) {
			Com_Printf( ". " );
		}
		return;
	}

	CL_WritePacket();
}

/*
============
CL_InitInput
============
*/
void CL_InitInput( void ) {
	Cmd_AddCommand ("centerview",IN_CenterView);

	Cmd_AddCommand ("+taunt", IN_Button3Down);//gesture
	Cmd_AddCommand ("-taunt", IN_Button3Up);
	Cmd_AddCommand ("+moveup",IN_UpDown);
	Cmd_AddCommand ("-moveup",IN_UpUp);
	Cmd_AddCommand ("+movedown",IN_DownDown);
	Cmd_AddCommand ("-movedown",IN_DownUp);
	Cmd_AddCommand ("+left",IN_LeftDown);
	Cmd_AddCommand ("-left",IN_LeftUp);
	Cmd_AddCommand ("+right",IN_RightDown);
	Cmd_AddCommand ("-right",IN_RightUp);
	Cmd_AddCommand ("+forward",IN_ForwardDown);
	Cmd_AddCommand ("-forward",IN_ForwardUp);
	Cmd_AddCommand ("+back",IN_BackDown);
	Cmd_AddCommand ("-back",IN_BackUp);
	Cmd_AddCommand ("+lookup", IN_LookupDown);
	Cmd_AddCommand ("-lookup", IN_LookupUp);
	Cmd_AddCommand ("+lookdown", IN_LookdownDown);
	Cmd_AddCommand ("-lookdown", IN_LookdownUp);
	Cmd_AddCommand ("+strafe", IN_StrafeDown);
	Cmd_AddCommand ("-strafe", IN_StrafeUp);
	Cmd_AddCommand ("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand ("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand ("+moveright", IN_MoverightDown);
	Cmd_AddCommand ("-moveright", IN_MoverightUp);
	Cmd_AddCommand ("+speed", IN_SpeedDown);
	Cmd_AddCommand ("-speed", IN_SpeedUp);
	Cmd_AddCommand ("+attack", IN_Button0Down);
	Cmd_AddCommand ("-attack", IN_Button0Up);
	//Cmd_AddCommand ("+force_jump", IN_Button1Down);//force jump
	//Cmd_AddCommand ("-force_jump", IN_Button1Up);
	Cmd_AddCommand ("+use", IN_Button5Down);
	Cmd_AddCommand ("-use", IN_Button5Up);
	Cmd_AddCommand ("+force_grip", IN_Button6Down);//force grip
	Cmd_AddCommand ("-force_grip", IN_Button6Up);
	Cmd_AddCommand ("+altattack", IN_Button7Down);//altattack
	Cmd_AddCommand ("-altattack", IN_Button7Up);
	Cmd_AddCommand ("+useforce", IN_Button9Down);//active force power
	Cmd_AddCommand ("-useforce", IN_Button9Up);
	Cmd_AddCommand ("+force_lightning", IN_Button10Down);//active force power
	Cmd_AddCommand ("-force_lightning", IN_Button10Up);
	Cmd_AddCommand ("+force_drain", IN_Button11Down);//active force power
	Cmd_AddCommand ("-force_drain", IN_Button11Up);
	//buttons
	Cmd_AddCommand ("+button0", IN_Button0Down);//attack
	Cmd_AddCommand ("-button0", IN_Button0Up);
	Cmd_AddCommand ("+button1", IN_Button1Down);//force jump
	Cmd_AddCommand ("-button1", IN_Button1Up);
	Cmd_AddCommand ("+button2", IN_Button2Down);//use holdable (not used - change to use jedi power?)
	Cmd_AddCommand ("-button2", IN_Button2Up);
	Cmd_AddCommand ("+button3", IN_Button3Down);//gesture
	Cmd_AddCommand ("-button3", IN_Button3Up);
	Cmd_AddCommand ("+button4", IN_Button4Down);//walking
	Cmd_AddCommand ("-button4", IN_Button4Up);
	Cmd_AddCommand ("+button5", IN_Button5Down);//use object
	Cmd_AddCommand ("-button5", IN_Button5Up);
	Cmd_AddCommand ("+button6", IN_Button6Down);//force grip
	Cmd_AddCommand ("-button6", IN_Button6Up);
	Cmd_AddCommand ("+button7", IN_Button7Down);//altattack
	Cmd_AddCommand ("-button7", IN_Button7Up);
	Cmd_AddCommand ("+button8", IN_Button8Down);
	Cmd_AddCommand ("-button8", IN_Button8Up);
	Cmd_AddCommand ("+button9", IN_Button9Down);//active force power
	Cmd_AddCommand ("-button9", IN_Button9Up);
	Cmd_AddCommand ("+button10", IN_Button10Down);//force lightning
	Cmd_AddCommand ("-button10", IN_Button10Up);
	Cmd_AddCommand ("+button11", IN_Button11Down);//force drain
	Cmd_AddCommand ("-button11", IN_Button11Up);
	Cmd_AddCommand ("+button12", IN_Button12Down);
	Cmd_AddCommand ("-button12", IN_Button12Up);
	Cmd_AddCommand ("+button13", IN_Button13Down);
	Cmd_AddCommand ("-button13", IN_Button13Up);
	Cmd_AddCommand ("+button14", IN_Button14Down);
	Cmd_AddCommand ("-button14", IN_Button14Up);
	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);

	Cmd_AddCommand ("sv_saberswitch", IN_GenCMD1);
	Cmd_AddCommand ("engage_duel", IN_GenCMD2);
	Cmd_AddCommand ("force_heal", IN_GenCMD3);
	Cmd_AddCommand ("force_speed", IN_GenCMD4);
	Cmd_AddCommand ("force_pull", IN_GenCMD5);
	Cmd_AddCommand ("force_distract", IN_GenCMD6);
	Cmd_AddCommand ("force_rage", IN_GenCMD7);
	Cmd_AddCommand ("force_protect", IN_GenCMD8);
	Cmd_AddCommand ("force_absorb", IN_GenCMD9);
	Cmd_AddCommand ("force_healother", IN_GenCMD10);
	Cmd_AddCommand ("force_forcepowerother", IN_GenCMD11);
	Cmd_AddCommand ("force_seeing", IN_GenCMD12);
	Cmd_AddCommand ("use_seeker", IN_GenCMD13);
	Cmd_AddCommand ("use_field", IN_GenCMD14);
	Cmd_AddCommand ("use_bacta", IN_GenCMD15);
	Cmd_AddCommand ("use_electrobinoculars", IN_GenCMD16);
	Cmd_AddCommand ("zoom", IN_GenCMD17);
	Cmd_AddCommand ("use_sentry", IN_GenCMD18);
	Cmd_AddCommand ("saberAttackCycle", IN_GenCMD19);
	Cmd_AddCommand ("force_throw", IN_GenCMD20);

	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
	cl_debugMove = Cvar_Get ("cl_debugMove", "0", 0);

	cl_idrive = Cvar_Get("cl_idrive", "0", CVAR_ARCHIVE);//JAPRO ENGINE
}
