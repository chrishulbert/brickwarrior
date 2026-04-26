// dxGame.cpp: implementation of the dxGame class.
//
//////////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdio.h>
#include <stdlib.h>	// random!!!
#include <time.h>

//#include "ddutil.h"
#include "dxGame.h"
#include "vkeys.h"
#include "settings.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dxGame	theGame; // here it is!!!!!!! YYEAAAAHHHHH!

dxGame::dxGame() {}	// shrink it!!!
dxGame::~dxGame() {} // I really should use these con/destructors...

void dxGame::DeInit()
	{
	sprites.UnLoad(); // free the sprite memory
	background.UnLoad();
	mousecursor.UnLoad();
	font8.UnLoad();
	font16.UnLoad();

	if (lpDDSGame != NULL) {
		lpDDSGame->Release();
		lpDDSGame=NULL;
		}

	SaveHighScores();
 	}

int	dxGame::HasQuit()
	{
	return quit; // true = quit / false = continue
	}

void dxGame::SetPause(int on)
	{
	if (on && !inmenu) {curmenu=0; curopt=0; menuopts=5; inmenu=1;}
	}

#define	LVLFILEID	0x1a4c564c	// LVL<eof>
#define	LV2FILEID	0x1a32564c	// LV2<eof>
int	dxGame::LoadLevelLvl(char *filename,int	level)
	{
	FILE	*fh;
	long	i,j,k,t,x,y,wid,ht,r,g,b,hits,active,moves,dir,time,fileid;
	short	movedir[MAXMOVES],movedist[MAXMOVES],movetime[MAXMOVES];
	short	xv,yv,dist;

	fh=fopen(filename,"rb");
	if (fh!=NULL) {
		fread(&fileid,4,1,fh); // ID -> LVL<eof>
		if (fileid!=LVLFILEID && fileid!=LV2FILEID)	{fclose(fh); return	false;}	// does it match?
		fread(&i,4,1,fh); // # of levels
		if (level>i	|| level<=0) {fclose(fh); return false;} // bad level #

		fseek(fh,(level-1)*4,SEEK_CUR);	// skip to the data for the offset for this level
		fread(&i,4,1,fh); // get the offset to the level data
		fseek(fh,i,SEEK_SET); // jump to the level

		// load level name
		fread(levelname,100,1,fh);

		// load balls
		j=0; fread(&j,2,1,fh);
		for	(i=0;i<j &&	i<MAXBALLS;i++)	{
			x=0; fread(&x,2,1,fh);
			y=0; fread(&y,2,1,fh);
			xv=0; fread(&xv,2,1,fh);
			yv=0; fread(&yv,2,1,fh);
			active=0; fread(&active,1,1,fh);
			ballstartx=x*100;
			ballstarty=y*100;
			ballstartxv=xv*100;
			ballstartyv=yv*100;
			ballstartactive=active;
			if (!active) {ballstartxv/=2; ballstartyv/=2;} // slow it down half
			}

		// load bricks
		j=0; fread(&j,2,1,fh); bricks=0;
		for	(i=0;i<j &&	i<MAXBRICKS;i++) {
			x=0; fread(&x,2,1,fh);
			y=0; fread(&y,2,1,fh);
			wid=0; fread(&wid,2,1,fh);
			ht=0; fread(&ht,2,1,fh);
			hits=0;	fread(&hits,1,1,fh);
			r=0; fread(&r,1,1,fh);
			g=0; fread(&g,1,1,fh);
			b=0; fread(&b,1,1,fh);
			moves=0; if	(fileid==LV2FILEID)	fread(&moves,1,1,fh);
			for	 (k=0;k<moves;k++) { // load movements
				dir=time=0; dist=0;
				fread(&dir,1,1,fh);	// 1=up 2=right 3=down 4=left
				fread(&dist,2,1,fh);
				fread(&time,2,1,fh);
				if (k<MAXMOVES)	{
					movedir[k]=(short)dir;
					movedist[k]=(short)dist;
					movetime[k]=(short)time;
					}
				}
			if (bricks<MAXBRICKS) {
				brick[bricks].x=x*100;
				brick[bricks].y=y*100;
				brick[bricks].wid=wid*100;
				brick[bricks].ht=ht*100;
				if (wid<=0)	wid=1; if (ht<=0) ht=1;	// to prevent div by 0 later on
				brick[bricks].hits=hits;
				brick[bricks].edge=(hits!=1); // it has an edge!
				brick[bricks].flashtime=0;
				brick[bricks].red=(unsigned	char)r;
				brick[bricks].green=(unsigned char)g;
				brick[bricks].blue=(unsigned char)b;
				brick[bricks].clr=MYRGB(r,g,b);
				brick[bricks].high=MYRGB(min(r+43,255),min(g+43,255),min(b+43,255));
				brick[bricks].low=MYRGB(max(r-43,0),max(g-43,0),max(b-43,0));
				brick[bricks].moves=(char)moves;
				brick[bricks].curmove=0;
				brick[bricks].curmovetime=0;
				for	(k=0;k<moves;k++) {
					brick[bricks].movedir[k]=(char)movedir[k];
					brick[bricks].movedist[k]=(short)movedist[k];
					brick[bricks].movetime[k]=(short)movetime[k]; }
				t=brick[bricks].movedir[0];
				if (t==1 ||	t==3) {	// up or down
					brick[bricks].curmovex=(short)x;
					if (t==1) // up
						brick[bricks].curmovey=y-brick[bricks].movedist[0];
					if (t==3) // down
						brick[bricks].curmovey=y+brick[bricks].movedist[0];
					}
				if (t==2 ||	t==4) {	// left or right
					brick[bricks].curmovey=(short)y;
					if (t==2) // right
						brick[bricks].curmovex=x+brick[bricks].movedist[0];
					if (t==4)
						brick[bricks].curmovex=x-brick[bricks].movedist[0];
					}
				bricks++;
				} // if bricks<maxbricks
			} // for i=0 to j
		fclose(fh);

		balls=1;
		ball[0].x=ballstartx; ball[0].y=ballstarty;
		ball[0].xv=ballstartxv;	ball[0].yv=ballstartyv;
		ball[0].active=ballstartactive;
		if (ball[0].type>2)	ball[0].type=2;	// cannot start w/more than a bubble ball

		return true; // success
		}
	return false; // conked out
	}

int	dxGame::LoadLevelInf(char *filename)
	{
	enum	{uptobricks=1,uptoballs};
	FILE	*fh;
	char	line[1024],textbuf[200],textbuf2[200];
	int		upto,i,j;
	long	x,y,xv,yv,wid,ht,c,r,g,b,hits,active,moves;

	fh=fopen(filename,"rb");
	if (fh!=NULL) {
		upto=0;
		while (!feof(fh)) {
			if (fgets(line,1024,fh)==NULL) line[0]=0;

			if (!strnicmp(line,"Name",4)) {
				if (strchr(line,'"')) {
					strcpy(levelname,strchr(line,'"')+1);
					if (strrchr(levelname,'"'))
						*strrchr(levelname,'"')=0;
					}
				}
			if (!strnicmp(line,"[Bricks]",8)) upto=uptobricks;
			if (!strnicmp(line,"[Balls]",7)) upto=uptoballs;
			if (line[0]=='{') {
				if (upto==uptobricks) {	// brick data
					sscanf(line,"{%d,%d,%d,%d,%x,%d,%d",&x,&y,&wid,&ht,&c,&hits,&moves);
					if (bricks<MAXBRICKS) {
						brick[bricks].x=x*100;
						brick[bricks].y=y*100;
						brick[bricks].wid=wid*100;
						brick[bricks].ht=ht*100;
						if (wid<=0)	wid=1; if (ht<=0) ht=1;	// to prevent div by 0 later on
						brick[bricks].hits=hits;
						brick[bricks].edge=(hits!=1); // it has an edge!
						brick[bricks].flashtime=0;
						r=(unsigned	char)(c>>16);
						g=(unsigned	char)(c>>8);
						b=(unsigned	char)(c);
						brick[bricks].red=(unsigned	char)r;
						brick[bricks].green=(unsigned char)g;
						brick[bricks].blue=(unsigned char)b;
						brick[bricks].clr=MYRGB(r,g,b);
						brick[bricks].high=MYRGB(min(r+43,255),min(g+43,255),min(b+43,255));
						brick[bricks].low=MYRGB(max(r-43,0),max(g-43,0),max(b-43,0));
						if (moves<0	|| moves>MAXMOVES) moves=0;
						brick[bricks].moves=(char)moves;
						brick[bricks].curmove=0;
						brick[bricks].curmovetime=0;
						if (moves) strcpy(textbuf,strchr(line,'[')+1);
						for	(i=0;i<moves;i++) {
							j=0;
							sscanf(textbuf,"%c,%d,%d,%s",&j,
								&brick[bricks].movedist[i],
								&brick[bricks].movetime[i],
								textbuf2);
							j=tolower(j);
							if (j=='u')	j=1; if	(j=='r') j=2;
							if (j=='d')	j=3; if	(j=='l') j=4;
							brick[bricks].movedir[i]=j;
							strcpy(textbuf,textbuf2);
							}
						brick[bricks].curmove=0;
						brick[bricks].curmovetime=0;
						j=brick[bricks].movedir[0];
						if (j==1 ||	j==3) {	// up or down
							brick[bricks].curmovex=(short)x;
							if (j==1)
								brick[bricks].curmovey=y-brick[bricks].movedist[0];
							if (j==3)
								brick[bricks].curmovey=y+brick[bricks].movedist[0];
							}
						if (j==2 ||	j==4) {	// left or right
							brick[bricks].curmovey=(short)y;
							if (j==2) // right
								brick[bricks].curmovex=x+brick[bricks].movedist[0];
							if (j==4) // left
								brick[bricks].curmovex=x-brick[bricks].movedist[0];
							}
						bricks++;
						}
					} // brick data
				if (upto==uptoballs) {
					sscanf(line,"{%d,%d,%d,%d,%d",&x,&y,&xv,&yv,&active);
					ballstartx=x*100;
					ballstarty=y*100;
					ballstartxv=xv*100;
					ballstartyv=yv*100;
					ballstartactive=active;
					if (!active) {ballstartxv/=2; ballstartyv/=2;} // slow it down half
					} // ball data
				} // line[0]=='{'
			} // while !feof
		fclose(fh);

		balls=1;
		ball[0].x=ballstartx; ball[0].y=ballstarty;
		ball[0].xv=ballstartxv;	ball[0].yv=ballstartyv;
		ball[0].active=ballstartactive;
		if (ball[0].type>2)	ball[0].type=2;	// cannot start w/more than a bubble ball
		return true; // success
		}
	return false; // conked out
	}

void dxGame::LoadEpisodes(char * fn)
	{
	FILE	*fh;
	char	line[1024];

	episodes=0;
	fh=fopen(fn,"rb");
	if (fh!=NULL) {
		while (!feof(fh)) {
			if (fgets(line,1024,fh)==NULL) line[0]=0;

			if (line[0]=='{') {
				sscanf(line,"{%[^,],%d,%[^,}],%[^}]}",episode[episodes].name,
					&episode[episodes].levels,
					episode[episodes].basename,
					episode[episodes].backname);
				episodes++;
				}
			} // while !feof
		fclose(fh);

		return;	// success
		}
	return;	// conked out
	episodes=0;
	}

void dxGame::ClipPowerup(int thep)
	{
	if (powerup[thep].y<5000) 
		powerup[thep].y=5000;
	if (powerup[thep].x<3500)
		powerup[thep].x=3500;
	if (powerup[thep].x>60500)
		powerup[thep].x=60500;
	}

int	dxGame::ChoosePowerup()
	{
	int	i,keepit,numbubbles,numtriples,numcatches;

	// count all the existing bubbles and triples
	numbubbles=numtriples=numcatches=0;
	for	(i=0;i<powerups;i++) {
		if (powerup[i].type==PWR_BUBBLE	||
			powerup[i].type==PWR_BLACKBUBBLE) numbubbles++;
		if (powerup[i].type==PWR_CATCH)	numcatches++;
		if (powerup[i].type==PWR_TRIPLE) numtriples++;
		}

	do	{ // loop until the powerup is ok
		i=rand()%PWR_LAST;
		keepit=true;
		if (ball[0].type<2)	{ // all powerups are cool if youve got a bubble or above
			if (i==PWR_TRIPLE && (numbubbles ||	numcatches)) keepit=false;
			if ((i==PWR_BUBBLE || i==PWR_BLACKBUBBLE ||
				i==PWR_CATCH) && (numtriples ||	balls>1)) keepit=false;
			}
		} while	(!keepit);
	return	i;
	}

void dxGame::NewPowerup(int	x, int y)
	{
	if (powerups<MAXPOWERUPS) {
		powerup[powerups].x=x;
		powerup[powerups].y=y;
		powerup[powerups].type=ChoosePowerup();
		powerup[powerups].catchable=true;
		ClipPowerup(powerups);
		powerups++;
		}
	}

void dxGame::DegradeBalls()
	{ // when you get a powerup, this returns all the balls to their original colour
	/*  Not needed anymore
	int	k;
	for	(k=0;k<balls;k++) {
		if (ball[k].type<=2) ball[k].type=0; // go back to solid green
		else if	(ball[k].type<=5) ball[k].type=3; // go back to solid yellow
		else if	(ball[k].type==7) ball[k].type=6; // go back to first bubble
		}
	*/
	}

void dxGame::NoPowerups() // get rid of all powerups (lost a ball or
	// start a new episode)
	{
	padwidth=7500; ball[0].type=0; 
	updown=havecatch=protection=kill=0;	// powerups in possession
	}

void dxGame::NewBall() // a new ball (doesnt retain old powerups)
	{
	if (balls<MAXBALLS)	{
		ball[balls].x=ballstartx;
		ball[balls].y=ballstarty;
		ball[balls].xv=ballstartxv;
		ball[balls].yv=ballstartyv;
		ball[balls].active=ballstartactive;
		ball[balls].type=0;
		balls++;
		}
	timesincelasthit=0;
	}

void dxGame::LoadCurLevel()
	{
	// new level, so do some stuff
	padx=32000;	pady=42000;	// updown pady clipping (bottom 1/4 of the screen): 360->480
	bricks=powerups=kill=protection=0; levelname[0]=0;
	needredraw=true;
	timesincelasthit=0;
	caught=0;

	// try INF before try LVL
	char	__abc__[100];
	sprintf(__abc__,"levels\\%s%d.inf",episode[curepisode].basename,curlevel);
	if (LoadLevelInf(__abc__)) return; // loaded cool!

	sprintf(__abc__,"levels\\%s.lvl",episode[curepisode].basename);
	LoadLevelLvl(__abc__,curlevel);
	}

#define	NAMES	18
char	*name[NAMES]={"Nice","Smokey","Reaper",
	"Jazz","Ben","Psycho",
	"Jumper","Master","Freak",
	"Insane","The Man","One",
	"FatMan","Bubble","BrickWarrior",
	"Dinosaur","PaddleMeister","Ripper"};
void	dxGame::LoadHighScores()
	{
	int	i,result;
	DWORD	amtdone;
	char	text[100];
	HKEY	key; // registry

	result=RegOpenKeyEx	(HKEY_LOCAL_MACHINE,"SOFTWARE\\Red Shift\\BrickWarrior\\High Scores",0,KEY_QUERY_VALUE,&key);
	for	(i=0;i<HIGHSCORES;i++) {
		// default names
		strcpy(highscore[i].name,name[rand()%NAMES]);
		strcpy(highscore[i].episodename,episode[rand()%episodes].name);
		highscore[i].score=1000-10*i;
		highscore[i].level=1;

		// try to load the name
		if (!result) {
			sprintf(text,"Name%d",i+1);
			amtdone=100;
			RegQueryValueEx(key,text,NULL,NULL,(LPBYTE)&highscore[i].name,&amtdone);
			highscore[i].name[19]=0; // max length 19 chars

			amtdone=100;
			sprintf(text,"Episode%d",i+1);
			RegQueryValueEx(key,text,0,0,(LPBYTE)&highscore[i].episodename,&amtdone);

			sprintf(text,"Score%d",i+1);
			amtdone=sizeof(int);
			RegQueryValueEx(key,text,0,0,(BYTE *)&highscore[i].score,&amtdone);

			amtdone=sizeof(int);
			sprintf(text,"Level%d",i+1);
			RegQueryValueEx(key,text,0,0,(BYTE *)&highscore[i].level,&amtdone);
			}
		}
	if (!result) RegCloseKey(key);
	}

void	dxGame::AddHighScore(char *name,char *episodename,int score,int	level)
	{
	int	i,fitswhere;

	fitswhere=-1;
	for	(i=HIGHSCORES-1;i>=0;i--) if (score>=highscore[i].score) fitswhere=i;
	if (fitswhere==-1) return; // score didnt get in!

	// shuffle scores along
	i=fitswhere; // less typing :)
	memmove(&highscore[i+1],&highscore[i],sizeof(_highscore)*(HIGHSCORES-1-i));
	// save the scores
	strcpy(highscore[i].name,name);
	strcpy(highscore[i].episodename,episodename);
	highscore[i].score=score;
	highscore[i].level=level;
	}

void	dxGame::SaveHighScores()
	{
	int		i;
	char	text[100];
	HKEY	key; // registry

	RegCreateKeyEx (HKEY_LOCAL_MACHINE,
				"SOFTWARE\\Red Shift\\BrickWarrior\\High Scores",
				0,0,0,KEY_ALL_ACCESS,0,&key,0);

	for	(i=0;i<HIGHSCORES;i++) {
		sprintf(text,"Name%d",i+1);
		RegSetValueEx(key,text,0,REG_SZ,(unsigned char *)highscore[i].name,strlen(highscore[i].name)+1);
		sprintf(text,"Episode%d",i+1);
		RegSetValueEx(key,text,0,REG_SZ,(unsigned char *)highscore[i].episodename,strlen(highscore[i].episodename)+1);
		sprintf(text,"Score%d",i+1);
		RegSetValueEx(key,text,0,REG_DWORD,(unsigned char *)&highscore[i].score,4);
		sprintf(text,"Level%d",i+1);
		RegSetValueEx(key,text,0,REG_DWORD,(unsigned char *)&highscore[i].level,4);
		}
	RegCloseKey	(key);
	}

void dxGame::DropCatch(int force)
	{
	if (havecatch && (ball[0].type<2 || force))	{ // only drop the catch if you have less than a bubble ball
		havecatch=false;
		caught=false;
		if (powerups<MAXPOWERUPS) {
			powerup[powerups].x=padx;
			powerup[powerups].y=pady;
			powerup[powerups].type=PWR_CATCH;
			powerup[powerups].catchable=false;
		
			ClipPowerup(powerups);
			powerups++;
			}
		}
	}

int	dxGame::shiftup(int	c)
	{
	// gets a character and returns what it 
	// should be if the user held shift down
	char	 *intable="`1234567890-=[]\\;',./",
		*outtable="~!@#$%^&*()_+{}|:\"<>?";
	int	i,in,len;

	// easy one!		
	if (isalpha(c))	return toupper(c);

	in=toupper(c);
	for	(i=0,len=strlen(intable) ; i<=len ;	i++)
		if (in==intable[i])	return outtable[i];

	return in;
	}

void dxGame::RedrawBrick(int i)	// i is the brick #
	{
	int		temp;
	DDBLTFX	ddbltfx;
	RECT	rbrick;

	if (brick[i].red==0 &&
		brick[i].green==0 &&
		brick[i].blue==0) return; // invisible brick!

	ddbltfx.dwSize=sizeof(ddbltfx);
	ddbltfx.dwFillColor	= brick[i].clr;
	if (brick[i].flashtime)	ddbltfx.dwFillColor=brick[i].high;
	rbrick.left=brick[i].x/100;
	rbrick.top=brick[i].y/100;
	rbrick.right=(brick[i].x+brick[i].wid)/100;
	rbrick.bottom=(brick[i].y+brick[i].ht)/100;
	lpDDSGame->Blt (&rbrick, NULL,
		NULL, DDBLT_COLORFILL, &ddbltfx);
	if (brick[i].edge) { // draw the edges
		ddbltfx.dwFillColor	= brick[i].high; // topleft colours
		if (brick[i].flashtime)	{
			//if (BITS==8) ddbltfx.dwFillColor=215;
			ddbltfx.dwFillColor=MYRGB(255,255,255);
			}
		temp=rbrick.right; rbrick.right=rbrick.left+2;
		lpDDSGame->Blt(&rbrick,NULL,NULL,DDBLT_COLORFILL,&ddbltfx);	// left
		rbrick.right=temp; temp=rbrick.bottom; rbrick.bottom=rbrick.top+2;
		lpDDSGame->Blt(&rbrick,NULL,NULL,DDBLT_COLORFILL,&ddbltfx);	// top
		rbrick.bottom=temp;
		ddbltfx.dwFillColor	= brick[i].low;	// bottomright colours
		if (brick[i].flashtime)	ddbltfx.dwFillColor=brick[i].clr;
		rbrick.top++; rbrick.left++;
		temp=rbrick.left; rbrick.left=rbrick.right-2; rbrick.right--;
		lpDDSGame->Blt(&rbrick,NULL,NULL,DDBLT_COLORFILL,&ddbltfx);	// right
		rbrick.left++; rbrick.top--; rbrick.right++;
		lpDDSGame->Blt(&rbrick,NULL,NULL,DDBLT_COLORFILL,&ddbltfx);	// right
		rbrick.left=temp; rbrick.top=rbrick.bottom-2; rbrick.bottom--;
		lpDDSGame->Blt(&rbrick,NULL,NULL,DDBLT_COLORFILL,&ddbltfx);	// bottom
		rbrick.top++; rbrick.left--; rbrick.bottom++;
		lpDDSGame->Blt(&rbrick,NULL,NULL,DDBLT_COLORFILL,&ddbltfx);	// bottom
		}
	}

void dxGame::DrawGameScreen()
	{// draw the game's: 1)background 2)bricks 3)level name into lpDDSGame
	if (lpDDSGame->IsLost()==DDERR_SURFACELOST)	{
		lpDDSGame->Restore(); // get back the memory...
		needredraw=true; // and redraw it
		}

	if (needredraw)	{
		background.Display(0,0,0,0,640,480,lpDDSGame); // back
		RedrawLevelname(); // level name
		for	(int i=0;i<bricks;i++) RedrawBrick(i); // bricks
		needredraw=false; // its been redrawn now
		}
	}

void dxGame::RedrawLevelname()
	{
	RECT	rbrick;
	DDBLTFX	ddbltfx;
	ddbltfx.dwSize=sizeof(ddbltfx);

	if (levelname[0]) {	// make sure the name isnt blank
		//if (BITS==8) ddbltfx.dwFillColor=86; // the backing 4 the level name	
		ddbltfx.dwFillColor=MYRGB(102,102,102);
		rbrick.left=320-strlen(levelname)*4;
		rbrick.top=52;
		rbrick.right=320+strlen(levelname)*4;
		rbrick.bottom=62;
		lpDDSGame->Blt (&rbrick, NULL,
			NULL, DDBLT_COLORFILL, &ddbltfx);
		sprites.Display(314-strlen(levelname)*4,52,388,0,6,10,lpDDSGame);
		sprites.Display(320+strlen(levelname)*4,52,394,0,6,10,lpDDSGame);
		font8.Text(levelname,320-strlen(levelname)*4,53,0,lpDDSGame);
		}
	}

void dxGame::RedrawRect(int	left, int top, int wid,	int	ht,int bricktoskip)
	{
	// if its BIG then don't bother, there may be bricks underneath
	//if (wid>80 && ht>80) {needredraw=true; return;}

	background.Display(left,top,left,top,wid,ht,lpDDSGame);	// back

	int	i;
	if (top<61)	{ // redraw the level name if needed
		RedrawLevelname(); // level name
		for	(i=0;i<bricks;i++)
			if (i!=bricktoskip && brick[i].y<6100) RedrawBrick(i);
		} // if top

	// redraw any bricks underneath
	int	boxleft,boxright,boxtop,boxbottom;
	boxleft=left*100; boxright=(left+wid)*100;
	boxtop=top*100; boxbottom=(top+ht)*100;
	for (i=0;i<bricks;i++) {
		// if brickright >= boxleft && brickleft <= boxright
		if (i!=bricktoskip &&
			brick[i].x+brick[i].wid >= boxleft && brick[i].x <= boxright &&
            brick[i].y+brick[i].ht >= boxtop && brick[i].y <= boxbottom)
				RedrawBrick(i);
		}
	} // redrawrect

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

// this does a black fade (sort of darkens everything)
void dxGame::BlackFade(LPDIRECTDRAWSURFACE surf)
	{
	int	rval;

	// make sure its 8-bits
	DDPIXELFORMAT	pixelformat;
	pixelformat.dwSize=sizeof(DDPIXELFORMAT);
	rval=surf->GetPixelFormat(&pixelformat);
	if (rval!=DD_OK	|| !(pixelformat.dwFlags & DDPF_PALETTEINDEXED8)) return;

	// lock the data
	DDSURFACEDESC	surfacedetails;
	surfacedetails.dwSize=sizeof(DDSURFACEDESC);
	rval=surf->Lock(NULL,&surfacedetails,DDLOCK_WAIT,NULL);
	if (rval!=DD_OK) return; // bummer

	// access the data
		DWORD	x,y,ptr,lsx;
		//BYTE    c,r,g,b;
	lsx=0;
	for	(y=0,ptr=0;y<surfacedetails.dwHeight;y++,ptr+=surfacedetails.lPitch) {
				/*for (x=0;x<surfacedetails.dwWidth;x++) {
			c=*((LPBYTE(surfacedetails.lpSurface))+ptr+x);
			if (c>=43) c-=43; else c=0;
			*((LPBYTE(surfacedetails.lpSurface))+ptr+x)=c;
			}*/
		if (!(y%2))	// solid
					for	(x=0;x<surfacedetails.dwWidth;x++)
							*((LPBYTE(surfacedetails.lpSurface))+ptr+x)	= 0;
		else	{// dotted
					for	(x=lsx;x<surfacedetails.dwWidth;x+=2)
							*((LPBYTE(surfacedetails.lpSurface))+ptr+x)	= 0;
			lsx^=1;	// invert it
			}
		}
   
	// done!
	surf->Unlock(NULL);	
	}

void	dxGame::DrawPaddle(int x,int y,int wid)
	{
	#define	PADEND	5
	sprites.Display((x-wid/2),y-5,kill*200,12,PADEND,10); // left end
	sprites.Display((x-wid/2)+PADEND,y-5,kill*200+100+PADEND-wid/2,12,wid-2*PADEND,10);	// middle filler
	sprites.Display((x+wid/2)-PADEND,y-5,kill*200+200-PADEND,12,PADEND,10);	// right end
	}

int	dxGame::Cheat()
	{ // true if cheats enabled, false if not
	FILE	*fh;
	char	line[100];

	fh=fopen("CHEAT.TXT","rb");
	if (!fh) return	false; // no cheats
	fgets(line,100,fh);
	if (strnicmp(line,"<Cheat>",7))	{fclose(fh); return	false;}	// no cheats!
	fclose(fh);
	return true; // cheats on!
	}

