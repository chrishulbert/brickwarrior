char	*savenames[]={
	"Episode","Level","Updown","Kill",
	"Protection","HaveCatch","Caught","CaughtBall",
	"PadX","PadY","PadWidth","CurBalls","BallStartX",
	"BallStartY","BallStartXV","BallStartYV","BallStartActive",
	"Mult","Score",""};
void dxGame::SaveGame(int slot)
	{
	long	*savedata[]={
		&curepisode,&curlevel,&updown,&kill,
		&protection,&havecatch,&caught,&caughtball,
		&padx,&pady,&padwidth,&curballs,&ballstartx,
		&ballstarty,&ballstartxv,&ballstartyv,&ballstartactive,
		&mult,&score};
	int		i,j;
	char	text[100];
	HKEY	key; // registry

	// delete it first, to get rid of extra junk that isnt needed
	DeleteSave(slot);

	// open the slot
	sprintf	(text,"SOFTWARE\\Red Shift\\BrickWarrior\\Save Games\\Save #%d",slot);
	RegCreateKeyEx (HKEY_LOCAL_MACHINE,text,
				0,0,0,KEY_ALL_ACCESS,0,&key,0);

	// save balls
	RegSetValueEx(key,"Balls",0,REG_DWORD,(BYTE	*)&balls,4);
	for	(i=0;i<balls;i++) {
		sprintf(text,"Ball%d",i+1);
		RegSetValueEx(key,text,0,REG_BINARY,(BYTE *)&ball[i],sizeof(_ball));
		}

	// save bricks
	struct	_savebrick {
		long	x,y,wid,ht;
		short	flashtime;
		BYTE	hits,edge;
		long	clr; // RGB colour
		} savebrick;
	struct	_savemoves {
		BYTE	moves,curmove;
		short	curmovetime,curmovex,curmovey;
		BYTE	movedata[MAXMOVES*5];
		} savemoves;
	
	RegSetValueEx(key,"Bricks",0,REG_DWORD,(BYTE *)&bricks,4);
	for	(i=0;i<bricks;i++) {
		savebrick.x=brick[i].x;	savebrick.y=brick[i].y;
		savebrick.wid=brick[i].wid;	savebrick.ht=brick[i].ht;
		savebrick.flashtime=(short)brick[i].flashtime;
		savebrick.hits=(BYTE)brick[i].hits;
		savebrick.edge=(BYTE)brick[i].edge;
		savebrick.clr=((brick[i].red&0xff)<<16)	+
			((brick[i].green&0xff)<<8) +
			(brick[i].blue&0xff);

		sprintf(text,"Brick%d",i+1);
		RegSetValueEx(key,text,0,REG_BINARY,(BYTE *)&savebrick,sizeof(_savebrick));

		if (brick[i].moves)	{ // save the brick movement
			savemoves.moves=brick[i].moves;
			savemoves.curmove=brick[i].curmove;
			savemoves.curmovetime=brick[i].curmovetime;
			savemoves.curmovex=brick[i].curmovex;
			savemoves.curmovey=brick[i].curmovey;
			memset(savemoves.movedata,0,MAXMOVES*5);
			for	(j=0;j<brick[i].moves;j++) {
				savemoves.movedata[j*5]=brick[i].movedir[j];
				savemoves.movedata[j*5+1]=BYTE(brick[i].movedist[j]);
				savemoves.movedata[j*5+2]=BYTE(brick[i].movedist[j]>>8);
				savemoves.movedata[j*5+3]=BYTE(brick[i].movetime[j]);
				savemoves.movedata[j*5+4]=BYTE(brick[i].movetime[j]>>8);
				}
			sprintf(text,"Brick%dMovement",i+1);
			RegSetValueEx(key,text,0,REG_BINARY,(BYTE *)&savemoves,8+5*brick[i].moves);
			}
		}

	// save powerups
	RegSetValueEx(key,"Powerups",0,REG_DWORD,(BYTE *)&powerups,4);
	for	(i=0;i<powerups;i++) {
		sprintf(text,"Powerup%d",i+1);
		RegSetValueEx(key,text,0,REG_BINARY,(BYTE *)&powerup[i],sizeof(_powerup));
		}

	// level name
	RegSetValueEx(key,"LevelName",0,REG_SZ,(BYTE *)levelname,strlen(levelname)+1);

	// other stuff
	for	(i=0;i<200 && savenames[i][0];i++)
		RegSetValueEx(key,savenames[i],0,REG_DWORD,(BYTE *)savedata[i],4);

	// episode filename
	RegSetValueEx(key,"EpisodeFileName",0,REG_SZ,(BYTE *)episode[curepisode].basename,strlen(episode[curepisode].basename)+1);

	RegCloseKey	(key); // saved!
	}

void dxGame::LoadGame(int slot)
	{
	long	*savedata[]={
		&curepisode,&curlevel,&updown,&kill,
		&protection,&havecatch,&caught,&caughtball,
		&padx,&pady,&padwidth,&curballs,&ballstartx,
		&ballstarty,&ballstartxv,&ballstartyv,&ballstartactive,
		&mult,&score};
	int		i,result,j;
	char	text[100];
	HKEY	key; // registry
	DWORD	amtdone;

	// open the slot
	sprintf	(text,"SOFTWARE\\Red Shift\\BrickWarrior\\Save Games\\Save #%d",slot);
	result=RegOpenKeyEx	(HKEY_LOCAL_MACHINE,text,0,KEY_QUERY_VALUE,&key);

	if (result)	return;	// couldn't open!!!

	// load balls
	amtdone=sizeof(long); RegQueryValueEx(key,"Balls",0,0,(BYTE	*)&balls,&amtdone);
	amtdone=sizeof(_ball);
	for	(i=0;i<balls;i++) {
		sprintf(text,"Ball%d",i+1);
		RegQueryValueEx(key,text,0,0,(BYTE *)&ball[i],&amtdone);
		if (ball[i].type > 4) ball[i].type=4;
		}

	// load bricks
	struct	_savebrick {
		long	x,y,wid,ht;
		short	flashtime;
		BYTE	hits,edge;
		long	clr; // RGB colour
		} savebrick;
	struct	_savemoves {
		BYTE	moves,curmove;
		short	curmovetime,curmovex,curmovey;
		BYTE	movedata[MAXMOVES*5];
		} savemoves;

	amtdone=sizeof(long); RegQueryValueEx(key,"Bricks",0,0,(BYTE *)&bricks,&amtdone);
	int	r,g,b;
	for	(i=0;i<bricks;i++) {
		sprintf(text,"Brick%d",i+1);
		amtdone=sizeof(_savebrick);
		RegQueryValueEx(key,text,0,0,(BYTE *)&savebrick,&amtdone);

		brick[i].x=savebrick.x;	brick[i].y=savebrick.y;
		brick[i].wid=savebrick.wid;	brick[i].ht=savebrick.ht;
		brick[i].flashtime=savebrick.flashtime;
		brick[i].hits=savebrick.hits;
		brick[i].edge=savebrick.edge;
		r=BYTE(savebrick.clr>>16);
		g=BYTE(savebrick.clr>>8);
		b=BYTE(savebrick.clr);
		brick[i].clr=MYRGB(r,g,b);
		brick[i].high=MYRGB(min(r+42,255),min(g+42,255),min(b+42,255));
		brick[i].low=MYRGB(max(r-42,0),max(g-42,0),max(b-42,0));
		brick[i].red=r;
		brick[i].green=g;
		brick[i].blue=b;

		sprintf(text,"Brick%dMovement",i+1);
		amtdone=sizeof(_savemoves);
		result=RegQueryValueEx(key,text,0,0,(BYTE *)&savemoves,&amtdone);

		if (!result) { // has movements!
			brick[i].moves=savemoves.moves;
			brick[i].curmove=savemoves.curmove;
			brick[i].curmovetime=savemoves.curmovetime;
			brick[i].curmovex=savemoves.curmovex;
			brick[i].curmovey=savemoves.curmovey;
			for	(j=0;j<brick[i].moves;j++) {
				brick[i].movedir[j]=savemoves.movedata[j*5];
				brick[i].movedist[j]=savemoves.movedata[j*5+1] +
					(savemoves.movedata[j*5+2]<<8);
				brick[i].movetime[j]=savemoves.movedata[j*5+3] +
					(savemoves.movedata[j*5+4]<<8);
				}
			}
		else
			brick[i].moves=0;
		}

	// load powerups
	amtdone=sizeof(long); RegQueryValueEx(key,"Powerups",0,0,(BYTE *)&powerups,&amtdone);
	amtdone=sizeof(_powerup);
	for	(i=0;i<powerups;i++) {
		sprintf(text,"Powerup%d",i+1);
		RegQueryValueEx(key,text,0,0,(BYTE *)&powerup[i],&amtdone);
		}

	// level name
	amtdone=100; RegQueryValueEx(key,"LevelName",0,0,(BYTE *)&levelname,&amtdone);

	// other stuff
	amtdone=sizeof(long);
	for	(i=0;i<200 && savenames[i][0];i++)
		RegQueryValueEx(key,savenames[i],0,0,(BYTE *)savedata[i],&amtdone);

	// episode filename
	char	episodefilename[100];
	amtdone=100; RegQueryValueEx(key,"EpisodeFileName",0,0,(BYTE *)&episodefilename,&amtdone);
	for (i=0;i<episodes;i++) {
		if (!strnicmp(episodefilename,episode[i].basename,strlen(episodefilename)+1)) curepisode=i;
		}

	RegCloseKey	(key); // voila!
	needredraw=true; // must redraw the game now
	timesincelasthit=0;
	}

void dxGame::GetSaveTitle(int slot,	char * title)
	{
	char	text[100];
	int		result;
	HKEY	key; // registry
	DWORD	amtdone;

	// open the slot
	sprintf	(text,"SOFTWARE\\Red Shift\\BrickWarrior\\Save Games\\Save #%d",slot);
	result=RegOpenKeyEx	(HKEY_LOCAL_MACHINE,text,0,KEY_QUERY_VALUE,&key);

	if (result)	{
		strcpy(title,"Unused savegame");
		return;	// couldn't open!!!
		}

	char	_lname[100];
	long	_episode,_level;
	amtdone=100; RegQueryValueEx(key,"LevelName",0,0,(BYTE *)&_lname,&amtdone);
	amtdone=sizeof(long);
	RegQueryValueEx(key,"Episode",0,0,(BYTE	*)&_episode,&amtdone);
	RegQueryValueEx(key,"Level",0,0,(BYTE *)&_level,&amtdone);

	sprintf	(title,"E%dL%d: %s",_episode+1,_level,_lname);

	RegCloseKey	(key); // voila!
	}

void dxGame::DeleteSave(int	slot)
	{
	char	text[100];
	int		result;
	HKEY	key; // registry

	// open the brickwarrior thing
	result=RegOpenKeyEx	(HKEY_LOCAL_MACHINE,"SOFTWARE\\Red Shift\\BrickWarrior\\Save Games",0,KEY_QUERY_VALUE,&key);

	if (result)	return;	// couldn't open!!! (it doesnt exist)

	sprintf	(text,"Save #%d",slot);
	RegDeleteKey(key,text);	// delete the savegame slot

	RegCloseKey	(key);
	}

int	dxGame::DoesSaveExist(int slot)
	{
	char	text[100];
	int		result;
	HKEY	key; // registry

	// open the slot
	sprintf	(text,"SOFTWARE\\Red Shift\\BrickWarrior\\Save Games\\Save #%d",slot);
	result=RegOpenKeyEx	(HKEY_LOCAL_MACHINE,text,0,KEY_QUERY_VALUE,&key);

	if (result)	return false; // couldn't open!!! (it doesnt exist)

	RegCloseKey	(key);
	return true; // save exists!
	}
