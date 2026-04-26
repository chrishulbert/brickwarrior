#include "graph.h"
#include "mouse.h"

#include <sys\stat.h>
#include <time.h>
#include <math.h>
#include <bios.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <unistd.h>

#define	RADRATIO	57.29577951308
#define	RAD2DEG(x) (x*RADRATIO)
#define	DEG2RAD(x) (x/RADRATIO)
#define	CLIP360(x) (x<0?x%360+360:x%360)
#define	FTL(x) (x<0?long(x-0.5):long(x+0.5)) // float to long with 5/4 cutoff
#define	MAX(x,y) (x<y?y:x)
#define	MIN(x,y) (x>y?y:x)

#define	VERSIONDESC	"version 18"
#define	HELPMESSAGE	"LevelEdit for Brickwarrior version 18\n"\
	"(c) Chris Hulbert 1998\n"\
	"Press F1 again to turn this help off\n"\
	"To create bricks: right drag the mouse\n"\
	"To create balls: press insert\n"\
	"To move bricks/balls: drag with left mouse button\n"\
	"To accurately move bricks/balls: move with arrow keys\n"\
	"To move the bricks 5x faster, hold CTRL when using the arrow keys\n"\
	"To select bricks/balls: left click on them or press space\n"\
	"Do ctrl+right drag to select all the bricks which are inside a region\n"\
	"Bricks can touch the red level boundary lines but should not go over them\n"\
	">>>> Keys:\n"\
	"m     > movement editor (for moving bricks)\n"\
	"f2    > save level (although it is saved automatically when you quit)\n"\
	"+     > duplicate selection to the right (shift goes left)\n"\
	"-     > duplicate selection down (shift goes up)\n"\
	"*     > split into 4\n"\
	"/     > split into 2 vertically (shift goes horizontally)\n"\
	"a     > select all bricks\n"\
	"n     > select nothing\n"\
	"< & > > open last/next file in episode\n"\
	"q/esc > quit and save\n"\
	"g     > display grid, paddle boundaries, and level title\n"\
	"l     > edit level title\n"\
	"del   > delete all selected bricks/balls\n"\
	"c     > clip all bricks into the level boundaries\n"\
	"s     > select bricks with same colour & strength to the one selected\n"\
	"h     > center all bricks horizontally in the level\n"\
	"t     > select all tiny bricks\n"\
	"shift-down > move colour down from sliders to selected bricks\n"\
	"shift-up   > move colour up from selected brick to colour sliders\n"\
	"[     > darken colour sliders\n"\
	"]     > brighten colour sliders\n"\
	"d     > kill duplicate bricks\n"\
	"CRANK'EM OUT!!!"

#define	COMMANDHELP	"\rLVLEDIT: level editor for BrickWarrior!!!\n"\
	"Usage: LVLEDIT <levelfile>\n"\
	"Example: LVLEDIT level1.inf\n"


void	EditLevel(char *fn);
int	main(int argc,char **args)
	{
	char	levelname[100];
	if (argc<2)	{
	  printf(COMMANDHELP);
	  printf("\nLevel name (eg 'level1') or just press enter if you don't want to\ncreate a level:");
	  gets(levelname);
	  if (levelname[0])	{
		  if (!strrchr(levelname,'.')) strcat(levelname,".inf");
		  EditLevel(levelname);	}
	  }
	else EditLevel(args[1]);
	return 0;
	}

#define	MAXBRICKS 1000
#define	MAXBALLS  10
#define	MAXMOVES	10
#define	BRICKCOMPSIZE	36 // only compare the x,y,wid,ht,r,g,b,hits,m_count to check for identical bricks
// ***********************************************************************
// ***********************************************************************
struct	_level {
	struct	_brick {
		long	x,y,wid,ht,r,g,b,hits,
				m_count;
		char	m_dir[MAXMOVES];
		long	m_dist[MAXMOVES],
				m_time[MAXMOVES];
		int		sel;
		} brick[MAXBRICKS];
	struct	_ball {
		long	x,y,bearing,velocity,active;
		int		sel;
		} ball[MAXBALLS];
	long	bricks,balls;
	char	name[1024];
	} level;

int	grid,help, // grid and help on or off
	slidred,slidgreen,slidblue;

int	needunflash=false;
void	UnFlash() {
	if (needunflash) {
		SetSinglePal(0,0,0,0);
		needunflash=false;
		}
	}
void	Flash()	{ // flash palette entry 0 to white
	SetSinglePal(0,63,63,63);
	needunflash=true;
	}

void	DelBrick(int i)	{
	if (i>=level.bricks	|| i<0)	return;
	memcpy(&level.brick[i],&level.brick[i+1],
		   sizeof(level.brick[0])*(level.bricks-i-1));
	level.bricks--;
	}

int	BricksIdentical(int	i,int j)
	{
	 return	!memcmp(&level.brick[i],&level.brick[j],BRICKCOMPSIZE);
	 }
void	KillDuplicates() // kills bricks with identical properties
	// called when you press 'd', after loading, and before saving
	{
	 int	i,j;
	 for (i=0;i<level.bricks;i++)
		for	(j=i+1;j<level.bricks;j++)
			if (BricksIdentical(i,j)) {
				DelBrick(j); j--;
				}
		// for j=i+1 to level.bricks
	 // for i=0 to level.bricks
	 }

void	LoadLevel(char *fn)	{
	enum	{uptobricks=1,uptoballs};
	FILE	*fh;
	char	line[1024],textbuf[200],textbuf2[200];
	int		upto,i;
	long	x,y,xv,yv,wid,ht,c,hits,active,movecount;

	level.bricks=0;
	level.balls=0;
	fh=fopen(fn,"rb");
	if (fh!=NULL) {
		upto=0;
		while (!feof(fh)) {
			if (fgets(line,1024,fh)==NULL) line[0]=0;

			if (!strnicmp(line,"Name",4)) {
				if (strchr(line,'"')) {
					strcpy(level.name,strchr(line,'"')+1);
					if (strrchr(level.name,'"'))
						*strrchr(level.name,'"')=0;
					}
				}
			if (!strnicmp(line,"[Bricks]",8)) upto=uptobricks;
			if (!strnicmp(line,"[Balls]",7)) upto=uptoballs;
			if (line[0]=='{') {
				if (upto==uptobricks) {	// brick data
					sscanf(line,"{%d,%d,%d,%d,%x,%d,%d",&x,&y,&wid,&ht,&c,&hits,&movecount);
					if (wid>=2 && ht>=2	&& level.bricks<MAXBRICKS) {
						level.brick[level.bricks].x=x;
						level.brick[level.bricks].y=y;
						level.brick[level.bricks].wid=wid;
						level.brick[level.bricks].ht=ht;
						level.brick[level.bricks].hits=hits;
						level.brick[level.bricks].r=(c>>16)&0xff;
						level.brick[level.bricks].g=(c>>8)&0xff;
						level.brick[level.bricks].b=(c	 )&0xff;
						level.brick[level.bricks].sel=0;
						if (movecount<0	|| movecount>MAXMOVES) movecount=0;
						level.brick[level.bricks].m_count=movecount;
						if (movecount) strcpy(textbuf,strchr(line,'[')+1);
						for	(i=0;i<movecount;i++) {
							sscanf(textbuf,"%c,%d,%d,%s",&level.brick[level.bricks].m_dir[i],
								&level.brick[level.bricks].m_dist[i],
								&level.brick[level.bricks].m_time[i],
								textbuf2);
							strcpy(textbuf,textbuf2);
							}
						level.bricks++;
						}
					} // brick data
				if (upto==uptoballs) {
					sscanf(line,"{%d,%d,%d,%d,%d",&x,&y,&xv,&yv,&active);
					if (level.balls<MAXBALLS) {
					  	if (x>640) {x/=100;	y/=100;	xv/=100; yv/=100;}
						level.ball[level.balls].x=x;
						level.ball[level.balls].y=y;
						level.ball[level.balls].bearing=
						 CLIP360(90-FTL(RAD2DEG(atan2(-yv,xv))));
						level.ball[level.balls].velocity=FTL(sqrt(xv*xv+yv*yv));
						level.ball[level.balls].active=active;
						level.ball[level.balls].sel=0;
						level.balls++;
						}
					} // ball data
				} // line[0]=='{'
			} // while !feof
		fclose(fh);

		KillDuplicates(); // kill all duplicate bricks
		return;	// success
		}
	else { // new level
	  strcpy(level.name,"Press L to change this");
		if (level.balls<MAXBALLS) {
			level.ball[level.balls].x=320;
			level.ball[level.balls].y=60;
			level.ball[level.balls].bearing=180;
			level.ball[level.balls].velocity=300;
			level.ball[level.balls].active=0;
			level.ball[level.balls].sel=0;
			level.balls++; }
	  }
	return;	// conked out
	}
int	FileExists(char	*fn) {
	FILE	*fh;
	fh=fopen(fn,"rb");
	if (fh==NULL) return false;	// doesnt exist
	fclose(fh);	return true; // does!
	}
void	CopyFile(char *src,char	*dest)
	{
	 FILE	*in,*out;
	 if	((in=fopen(src,"rb"))==NULL) return;
	 if	((out=fopen(dest,"wb"))==NULL) {fclose(in);	return;}

	   char	c;
	   while (!feof(in)) {
		 c=fgetc(in);
		 if	(feof(in)) break;
		 fputc(c,out);
		 }
	   fclose(out);
	   fclose(in);
	 }

void	MakeBackup(char	*fn) {
	char	backupname[1024],shortname[20];

	// quit if there is nothing to back up
	if (!FileExists(fn)) return;

	// get the shortname
	if (strrchr(fn,'\\')) strcpy(shortname,strrchr(fn,'\\')+1);
	else strcpy(shortname,fn);

	// get the directory name and add a BACKUPS bit to it
	strcpy(backupname,fn);
	if (strrchr(backupname,'\\')) strcpy(strrchr(backupname,'\\')+1,"Backups");
	else strcpy(backupname,"Backups");

	// make the directory
	mkdir(backupname,0); // take out the ,0 -> its for DJGPP

	// add the shortname file name to the directory name
	strcat(backupname,"\\");
	strcat(backupname,shortname);

	// go 4 it!
	CopyFile(fn,backupname);
	}

void	SaveLevel(char *fn)	{
	KillDuplicates();
	FILE	*fh;
	fh=fopen(fn,"wb"); if (fh==NULL) return;
	int	i,j;

	fprintf(fh,"; BrickWarrior Level file\r\n"\
			"; coords go from 0,0 to 640,480\r\n"\
			"; balls {x,y,xvel,yvel,active}\r\n"\
			"; bricks {x,y,wid,ht,RRGGBB,strength(0=indestructable),\r\n"\
			";  moves}[movedir1,movedist1,movetime1,...]\r\n\r\n"\
			"Name = \x22%s\x22\r\n\r\n",level.name);

	long	xv,yv;
	fprintf(fh,"[Balls]\r\n");
	for	(i=0;i<level.balls;i++)	{
		xv=	FTL(cos(DEG2RAD(CLIP360(90-level.ball[i].bearing)))*level.ball[i].velocity);
		yv=-FTL(sin(DEG2RAD(CLIP360(90-level.ball[i].bearing)))*level.ball[i].velocity);
		fprintf	(fh,"{%d,%d,%d,%d,%d}\r\n",level.ball[i].x,level.ball[i].y,xv,yv,level.ball[i].active);
		}
	fprintf(fh,"\r\n[Bricks]\r\n");
	for	(i=0;i<level.bricks;i++) {
		if (level.brick[i].wid>=2 && level.brick[i].ht>=2) // dont write tiny bricks
			fprintf	(fh,"{%d,%d,%d,%d,%02x%02x%02x,%d,%d}",level.brick[i].x,level.brick[i].y,level.brick[i].wid,
					 level.brick[i].ht,level.brick[i].r,level.brick[i].g,level.brick[i].b,level.brick[i].hits,level.brick[i].m_count);
		// put in the moves
		if (level.brick[i].m_count)	{
			fprintf	(fh,"[");
			for	(j=0;j<level.brick[i].m_count;j++) {
				if (j) fprintf(fh,",");
				fprintf	(fh,"%c,%d,%d",tolower(level.brick[i].m_dir[j]),level.brick[i].m_dist[j],
					level.brick[i].m_time[j]);
				}
			fprintf	(fh,"]");
			}
		fprintf(fh,"\r\n");
		}

	fclose(fh);
	}
void	DrawBrick(int x,int	y,int wid,int ht,int br,int	bg,int bb,int edge)
	{
	 int	r,g,b,h,l;

	if (br==0 && bg==0 && bb==0) { // invisible (big X thru it)
		Rectangle(x,y,wid,ht,RGB(br,bg,bb));
		Line(x,y,x+wid-1,y+ht-1,MYWHITE);
		Line(x+wid-1,y,x,y+ht-1,MYWHITE);
		} // if invisible
	else	{
		Rectangle(x,y,wid,ht,RGB(br,bg,bb));
		if (edge) {	// has an edge
			r=br+43; if	(r>255)	r=255;
			g=bg+43; if	(g>255)	g=255;
			b=bb+43; if	(b>255)	b=255;
			h=RGB(r,g,b);
			r=br-43; if	(r<0) r=0;
			g=bg-43; if	(g<0) g=0;
			b=bb-43; if	(b<0) b=0;
			l=RGB(r,g,b);
		  // top then left
		  Rectangle(x,y,wid-1,2,h);
		  Rectangle(x,y,2,ht-1,h);

		  // bottom then right
		  Rectangle(x+1,y+ht-2,wid-1,2,l);
		  Rectangle(x+wid-2,y+1,2,ht-1,l);

		  // corners (bottom left then top right)
		  PutPixel(x,y+ht-1,l);
		  PutPixel(x+wid-1,y,l);
		  }
		} // else not invisible
	 }

void	DisplayColourSliders()
	{
	 Box(191,0,258,10,MYWHITE);
	 Rectangle(192,1,256,8,RGB(slidred,0,0));
	 Line(192+slidred,1,192+slidred,9,MYWHITE);
	 if	(slidred>128) GTextGoto(192,1);
	 else GTextGoto(424,1);
	 GText("%d",slidred);

	 Box(191,11,258,10,MYWHITE);
	 Rectangle(192,12,256,8,RGB(0,slidgreen,0));
	 Line(192+slidgreen,12,192+slidgreen,20,MYWHITE);
	 if	(slidgreen>128)	GTextGoto(192,12);
	 else GTextGoto(424,12);
	 GText("%d",slidgreen);

	 Box(191,22,258,10,MYWHITE);
	 Rectangle(192,23,256,8,RGB(0,0,slidblue));
	 Line(192+slidblue,23,192+slidblue,31,MYWHITE);
	 if	(slidblue>128) GTextGoto(192,23);
	 else GTextGoto(424,23);
	 GText("%d",slidblue);

	 DrawBrick(450,0,32,32,slidred,slidgreen,slidblue,1);
	 }

void	DisplayLevel()
	{
	 int	i;

	 // draw box for the level name
	  if (level.name[0]) {
		  GTextGoto(320-strlen(level.name)*4,53);
		  Rectangle(320-strlen(level.name)*4-6,52,strlen(level.name)*8+12,10,RGB(128,0,255));
		  GText(level.name);
		  }
	// draw borders for macbrickout sized screen
	Line(19,44,19,480,RGB(255,0,128));
	Line(19,44,620,44,RGB(255,0,128));
	Line(620,44,620,480,RGB(255,0,128));
	if (grid) {
	  for (i=0;i<640;i+=20)	Line(i,0,i,480,MYWHITE);
	  for (i=0;i<480;i+=20)	Line(0,i,640,i,MYWHITE);

		Line(20,415,619,415,RGB(255,255,0)); // paddle with no updown
		Line(20,425,619,425,RGB(255,255,0));
		Line(20,315,619,315,RGB(255,255,0)); // paddle updown boundaries
	  }
	for	(i=0;i<level.bricks;i++) {
		DrawBrick(level.brick[i].x,level.brick[i].y,level.brick[i].wid,
				  level.brick[i].ht,level.brick[i].r,level.brick[i].g,
				  level.brick[i].b,(level.brick[i].hits!=1));
		if (level.brick[i].sel)	Box(level.brick[i].x,level.brick[i].y,
			level.brick[i].wid,level.brick[i].ht,MYWHITE);
		}
	for	(i=0;i<level.balls;i++)	{
	   	Rectangle(level.ball[i].x-6,level.ball[i].y-6,12,12,level.ball[i].sel?MYWHITE:RGB(0,0,255));
	   	Rectangle(level.ball[i].x-5,level.ball[i].y-5,10,10,RGB(255,0,128));
		}
	if (grid) {	// draw the border over the top if the grids on
		Line(19,44,19,480,RGB(255,0,128));
		Line(19,44,620,44,RGB(255,0,128));
		Line(620,44,620,480,RGB(255,0,128));
	  }
	}

#define	RSHIFT 1
#define	LSHIFT 2
#define	SHIFT 3
#define	CTRL 4
#define	ALT	8
int	GetShift() { return	bioskey(2);}
void	AddToSel(int x,int y,int z=0)
	{
	 int	i;
	 for (i=0;i<level.bricks;i++)
		 if	(x>=level.brick[i].x &&	x<=(level.brick[i].x+level.brick[i].wid) &&
			 y>=level.brick[i].y &&	y<=(level.brick[i].y+level.brick[i].ht))
		   		{if	(GetShift()&CTRL &&	!z)	level.brick[i].sel^=1;
				else level.brick[i].sel=1;}
	 for (i=0;i<level.balls;i++)
		 if	(x>=level.ball[i].x-6 && x<=level.ball[i].x+6 &&
			 y>=level.ball[i].y-6 && y<=level.ball[i].y+6 )
		   		{if	(GetShift()&CTRL &&	!z)	level.ball[i].sel^=1;
				else level.ball[i].sel=1;}
	 }
void	NoSel()
	{
	 int	i;
	 for (i=0;i<level.bricks;i++) level.brick[i].sel=0;
	 for (i=0;i<level.balls;i++) level.ball[i].sel=0;
	 }

#define	MYCLAMP(a,b,c) {if (a<b) a=b; if (a>c) a=c;}

void	ClipAll()
	{
	int	i,killit;
	for	(i=0;i<level.balls;i++)	{
		MYCLAMP(level.ball[i].x,20,619);
		MYCLAMP(level.ball[i].y,45,479);
		}

	if (GetShift()&SHIFT) {	// move bricks which are outside, don't cut them
	  for (i=0;i<level.bricks;i++) {
		MYCLAMP(level.brick[i].x,20,620-level.brick[i].wid);
		MYCLAMP(level.brick[i].y,45,480-level.brick[i].ht);
		}
	  }
	else // cut bricks which are outside
	for	(i=0;i<level.bricks;i++) {
		killit=0;
		if (level.brick[i].x<20) {
		  level.brick[i].wid-=20-level.brick[i].x;
		  level.brick[i].x=20;
		  }
		if (level.brick[i].y<45) {
		  level.brick[i].ht-=45-level.brick[i].y;
		  level.brick[i].y=45;
		  }
		if (level.brick[i].x+level.brick[i].wid>620) {
		  level.brick[i].wid=620-level.brick[i].x;
		  }
		if (level.brick[i].y+level.brick[i].ht>480)	{
		  level.brick[i].ht=480-level.brick[i].y;
		  }
		if (level.brick[i].wid<=0 || level.brick[i].ht<=0) {
		  	DelBrick(i); i--;
			}
		}
	}

void	MoveStuff(int dx,int dy,int	clamp=0)
	{
	 int	i;
		for	(i=0;i<level.balls;i++)
			  if (level.ball[i].sel) {
				level.ball[i].x+=dx;
   				level.ball[i].y+=dy;
				if (clamp) {
				MYCLAMP(level.ball[i].x,0,640);
				MYCLAMP(level.ball[i].y,0,480);	}
				   }
	   	for	(i=0;i<level.bricks;i++)
			  if (level.brick[i].sel) {
				level.brick[i].x+=dx;
   				level.brick[i].y+=dy;
				if (clamp) {
				MYCLAMP(level.brick[i].x,0,640-level.brick[i].wid);
				MYCLAMP(level.brick[i].y,0,480-level.brick[i].ht);
				}
				   }
		}
void	DelBall(int	i) {
	if (i>=level.balls || i<0) return;
	memcpy(&level.ball[i],&level.ball[i+1],
		   sizeof(level.ball[0])*(level.balls-i-1));
	level.balls--;
	}
void	DeleteSel()
	{
	 int	i;
		for	(i=0;i<level.balls;i++)
			  if (level.ball[i].sel) {
					DelBall(i);	i--;
				   }
	   	for	(i=0;i<level.bricks;i++)
			  if (level.brick[i].sel) {
					DelBrick(i); i--;
				   }
	 }

int	NewBrickwh(int x,int y,int wid,int ht,int hits=1,int r=255,int g=255,int b=255,int sel=1)
	{
	if (level.bricks<MAXBRICKS)	{
		level.brick[level.bricks].x=x;
		level.brick[level.bricks].y=y;
		level.brick[level.bricks].wid=wid;
		level.brick[level.bricks].ht=ht;
		level.brick[level.bricks].hits=hits;
		level.brick[level.bricks].r=r;
		level.brick[level.bricks].g=g;
		level.brick[level.bricks].b=b;
		level.brick[level.bricks].sel=sel;
		level.brick[level.bricks].m_count=0;
		level.bricks++;
		return true; // worked
		}
	return false;
	 }
void	NewBrick(int x1,int	y1,int x2,int y2,int hits=1,
				 int r=-1,int g=-1,int b=-1,int	sel=1)
	{
	int	x,y,wid,ht;

	if (x1<x2) {x=x1;wid=x2-x;}
	else {x=x2;wid=x1-x;}
	if (y1<y2) {y=y1;ht=y2-y;}
	else {y=y2;ht=y1-y;}

	if (wid<2 || ht<2) return; // tiny brick (1x1) dont bother

	if (level.bricks<MAXBRICKS)	{
		level.brick[level.bricks].x=x;
		level.brick[level.bricks].y=y;
		level.brick[level.bricks].wid=wid;
		level.brick[level.bricks].ht=ht;
		level.brick[level.bricks].hits=hits;
		if (r==-1) level.brick[level.bricks].r=slidred;
		else level.brick[level.bricks].r=r;
		if (g==-1) level.brick[level.bricks].g=slidgreen;
		else level.brick[level.bricks].g=g;
		if (b==-1) level.brick[level.bricks].b=slidblue;
		else level.brick[level.bricks].b=b;
		level.brick[level.bricks].sel=sel;
		level.brick[level.bricks].m_count=0;
		level.bricks++;
		}
	 }

void	SplitHalf()
	{
	 int	i,x,y,wid,ht,r,g,b,hits;
	   	for	(i=0;i<level.bricks;i++)
			  if (level.brick[i].sel) {
					x=level.brick[i].x;
					y=level.brick[i].y;
					wid=level.brick[i].wid;
					ht=level.brick[i].ht;
					r=level.brick[i].r;
					g=level.brick[i].g;
					b=level.brick[i].b;
					hits=level.brick[i].hits;
					DelBrick(i); i--;
					if (GetShift()&SHIFT) {
						NewBrickwh(x,y,		  wid,ht/2-1,hits,r,g,b,0);
						NewBrickwh(x,y+ht/2+1,wid,ht/2-1,hits,r,g,b,0);
						}
					else	{
						NewBrickwh(x,		 y,wid/2-1,ht,hits,r,g,b,0);
						NewBrickwh(x+wid/2+1,y,wid/2-1,ht,hits,r,g,b,0);
					  }
				   }
		}

void	SplitQuad()
	{
	 int	i,x,y,wid,ht,r,g,b,hits;
	   	for	(i=0;i<level.bricks;i++)
			  if (level.brick[i].sel) {
					x=level.brick[i].x;
					y=level.brick[i].y;
					wid=level.brick[i].wid;
					ht=level.brick[i].ht;
					r=level.brick[i].r;
					g=level.brick[i].g;
					b=level.brick[i].b;
					hits=level.brick[i].hits;
					DelBrick(i); i--;
					NewBrickwh(x,		   y,		  wid/2-1,ht/2-1,hits,r,g,b,0);
					NewBrickwh(x+wid/2+2,y,			wid/2-1,ht/2-1,hits,r,g,b,0);
					NewBrickwh(x,		   y+ht/2+2,wid/2-1,ht/2-1,hits,r,g,b,0);
					NewBrickwh(x+wid/2+2,y+ht/2+2,wid/2-1,ht/2-1,hits,r,g,b,0);
				   }
		}

void	DrawMyBox(int x1,int y1,int	x2,int y2,int col)
	{
	int	x,y,wid,ht;

	if (x1<x2) {x=x1;wid=x2-x;}
	else {x=x2;wid=x1-x;}
	if (y1<y2) {y=y1;ht=y2-y;}
	else {y=y2;ht=y1-y;}
	Box(x,y,wid,ht,col);
	 }

void	AddBall(int	x,int y)
	{
	if (level.balls<MAXBALLS) {
		level.ball[level.balls].x=x;
		level.ball[level.balls].y=y;
		level.ball[level.balls].bearing=180;//xv;
		level.ball[level.balls].velocity=300;//yv;
		level.ball[level.balls].active=0;//active;
		level.ball[level.balls].sel=1;//0;
		level.balls++; }
	 }

void	GetString(char *line)
	{
	 int	curpos,done,x,y,c;
	 curpos=done=line[0]=0;
	GTextGetpos(&x,&y);
	while (!done) {
	  if (kbhit()) {
		  c=getch();
		  if (c==27) {curpos=0;	line[0]=0; done=true;}
		  if (c==13) done=true;	// enter or escape
		  if (c>=32	&& c<128 &&	curpos<99) {
			line[curpos]=c;	curpos++;
			line[curpos]=0;
			}
		  if (c==8 && curpos>0)	{
			curpos--;
			line[curpos]=0;	}
		}
	  GTextGoto(x,y);
	  GText("%s%c ",line,(clock()/20%2&&!done)?'_':' ');
	  B_Show();
	  }
	 }

void	ChangeLevelName()
	{
	 B_Clear();	// clear buffer
	 DisplayLevel();
	 GTextGoto(0,0);
	 GText("Old level name:'%s'\n",level.name);

	 char	newname[100];
	 GText("Type new name:");
	 B_Show();
	 GetString(newname);
	 GText("\nKeep new name [y/n]?");
	 B_Show();
	 if	(tolower(getch())=='y')	strcpy(level.name,newname);
	 }

long	GetLong() {
	char	line[100];
	long	i;

	GetString(line);

	i=12345678;
	sscanf(line,"%d",&i);
	return i;
	}

void	EditBricks()
	{
	 int	i,j,numbricks,c,input;
	 numbricks=0; for (i=0;i<level.bricks;i++) if (level.brick[i].sel) numbricks++;
	 while (1) {
	  B_Clear(); // clear buffer
	  DisplayLevel();
		GTextGoto(0,0);
		GText("%d bricks selected / %d bricks total\n",numbricks,level.bricks);
		for	(i=j=0;i<level.bricks && j<10;i++)
		   if (level.brick[i].sel) { j++;
			 GText("%d): X(%d) Y(%d) WID(%d) HT(%d) STRENGTH(%d) R(%d) G(%d) B(%d) MOVES(%d)\n",i+1,level.brick[i].x,level.brick[i].y,level.brick[i].wid,level.brick[i].ht,level.brick[i].hits,level.brick[i].r,level.brick[i].g,level.brick[i].b,level.brick[i].m_count); }
		GText("\nX:x Y:y WID:w HT:h STRENGTH:s COLOUR:r/g/b GOLD:z GREY:a INVIS:i ?:");
		B_Show();
		c=tolower(getch());	while (kbhit())	getch();
		if (c==27 || c=='q'	|| c==13) break;

		if (c=='x')	GText("\nXPOS (0=left/640=right) ?:");
		if (c=='y')	GText("\nYPOS (0=top/480=bottom) ?:");
		if (c=='w')	GText("\nWIDTH (in pixels, minimum 2) ?:");
		if (c=='h')	GText("\nHEIGHT (in pixels, minimum 2) ?:");
		if (c=='s')	GText("\nSTRENGTH - HITS to take it out (0 = invincible) ?:");
		if (c=='r')	GText("\nRED (0-255) ?:");
		if (c=='g')	GText("\nGREEN (0-255) ?:");
		if (c=='b')	GText("\nBLUE (0-255) ?:");
		B_Show();

		input=0; if	(c!='z'	&& c!='a' && c!='i') input=GetLong();
		if (input!=12345678)
		for	(i=0;i<level.bricks;i++)
		   if (level.brick[i].sel) {
				if (c=='z')	{level.brick[i].r=207; level.brick[i].g=173; level.brick[i].b=48; level.brick[i].hits=0;}
				if (c=='a')	{level.brick[i].r=level.brick[i].g=level.brick[i].b=148; level.brick[i].hits=3;}
				if (c=='i')	{level.brick[i].r=level.brick[i].g=level.brick[i].b=0;}
				if (c=='x')	level.brick[i].x=input;
				if (c=='y')	level.brick[i].y=input;
				if (c=='w')	{MYCLAMP(input,2,1000);	level.brick[i].wid=input;}
				if (c=='h')	{MYCLAMP(input,2,1000);	level.brick[i].ht=input;}
				if (c=='s')	level.brick[i].hits=input;
				if (c=='r')	{MYCLAMP(input,0,255); level.brick[i].r=input;}
				if (c=='g')	{MYCLAMP(input,0,255); level.brick[i].g=input;}
				if (c=='b')	{MYCLAMP(input,0,255); level.brick[i].b=input;}
				}
		}
	 }
void	EditBalls()
	{
	 int	i,j,numballs,c,input;
	 numballs=0; for (i=0;i<level.balls;i++) if	(level.ball[i].sel)	numballs++;
	 while (1) {
	  B_Clear(); // clear buffer
	  DisplayLevel();
		GTextGoto(0,0);
		GText("%d balls selected / %d balls total\n",numballs,level.balls);
		for	(i=j=0;i<level.balls &&	j<10;i++)
		   if (level.ball[i].sel) {	j++;
			 GText("%d): X(%d) Y(%d) BEARING(%d) VELOCITY(%d) ACTIVE(%d)\n",i+1,level.ball[i].x,level.ball[i].y,level.ball[i].bearing,level.ball[i].velocity,level.ball[i].active);	}
		GText("\nX:x Y:y BEARING:b VELOCITY:v ACTIVE:a DONE:esc ?:");
		B_Show();
		c=tolower(getch());	while (kbhit())	getch();
		if (c==27 || c=='q'	|| c==13) break;

		if (c=='x')	GText("\nXPOS (0=left/640=right) ?:");
		if (c=='y')	GText("\nYPOS (0=top/480=bottom) ?:");
		if (c=='b')	GText("\nBEARING (degrees 0-360 clockwise from north) ?:");
		if (c=='v')	GText("\nVELOCITY (100 = 100 pixels per second) ?:");
		if (c=='a')	GText("\nACTIVE (0=starts off going through bricks) ?:");
		B_Show();
		input=GetLong();
		if (input!=12345678)
		for	(i=0;i<level.balls;i++)
		   if (level.ball[i].sel) {
				if (c=='x')	level.ball[i].x=input;
				if (c=='y')	level.ball[i].y=input;
				if (c=='b')	level.ball[i].bearing=input;
				if (c=='v')	level.ball[i].velocity=input;
				if (c=='a')	level.ball[i].active=input;
				}
		}
	 }

void	EditStuff()
	{
	 int	c,i,numbricks,numballs;

	 numbricks=0; for (i=0;i<level.bricks;i++) if (level.brick[i].sel) numbricks++;
	 numballs=0; for (i=0;i<level.balls;i++) if	(level.ball[i].sel)	numballs++;

	 if	(!numballs && !numbricks) return; // have to select something!
	 if	(!numballs)	EditBricks();
	 else if (!numbricks) EditBalls();
	 else	{
		B_Clear();
		DisplayLevel();
	   	GTextGoto(0,0);
		GText("(B)ricks | Ba(l)ls ?");
		B_Show();
		c=tolower(getch());	while (kbhit())	getch();
		if (c=='b')	EditBricks();
		if (c=='l')	EditBalls();
		}
	 }

void	EditBrickMovement()
	{
	int	bricksselected,i,j,k;
	int	curitem, // 0=direction 1=distance 2=time
		curnumber; // which movement is selected

	for	(bricksselected=i=0;i<level.bricks;i++)
		if (level.brick[i].sel)	bricksselected++;

	if (!bricksselected) return; // no bricks selected

	int	done=false,c,e;
	char	text[100];
	curitem=0; curnumber=0; // both are zero based
	do	{
		// show details
		B_Clear(); // clear buffer
		DisplayLevel();
		GTextGoto(0,0);
		GText("%d bricks selected / %d bricks total\n",bricksselected,level.bricks);
		for	(i=j=0;i<level.bricks && j<8;i++)
			if (level.brick[i].sel)	{
				GText("%d): MOVES(%d)",i+1,level.brick[i].m_count);
				for	(k=0;k<level.brick[i].m_count && k<MAXMOVES;k++) {
					if (k==curnumber) {
						#define	_back	RGB(128,128,128)
						#define	_fore	RGB(255,255,255)
						#define	_selb   _fore
						#define	_self   _back

						GTextColour(_fore,_back); // background grey
                        GText("\n      %2d:",k+1);
						if (curitem==0) GTextColour(_self,_selb);
						GText(" %c ",level.brick[i].m_dir[k]);
						GTextColour(_fore,_back); // background grey
						GText("DIST:");
						if (curitem==1) GTextColour(_self,_selb);
						GText("%-3d",level.brick[i].m_dist[k]);
						GTextColour(_fore,_back); // background grey
					 	GText(" TIME:");
						if (curitem==2) GTextColour(_self,_selb);
						GText("%-5d",level.brick[i].m_time[k]);
						GTextColour(-1,-1); // set it back to normal
						}
					else
						GText("\n      %2d: %c DIST:%-3d TIME:%-5d",k+1,
							level.brick[i].m_dir[k],level.brick[i].m_dist[k],level.brick[i].m_time[k]);
					}
				GText("\n");
				j++;
				}

		GText("\nEdit:Enter, Add:A, Delete one:D, Delete all:L, Done:Escape ?:");
		B_Show();

		c=tolower(getch());	e=0; if (!c) e=getch(); while (kbhit()) getch();
		if (c==27 || c=='q') done=true;

		// arrowing through
		if (GetShift() & SHIFT) {
			if (e==0x48 && curnumber>0) { // up
                for (i=0;i<level.bricks;i++)
					if (level.brick[i].sel) {
						int	_abc;
						#define SWAP(a,b) {_abc=b; b=a; a=_abc;}
						SWAP(level.brick[i].m_dir[curnumber-1],level.brick[i].m_dir[curnumber]);
						SWAP(level.brick[i].m_dist[curnumber-1],level.brick[i].m_dist[curnumber]);
						SWAP(level.brick[i].m_time[curnumber-1],level.brick[i].m_time[curnumber]);
						}
				} // up
			if (e==0x50 && curnumber<MAXMOVES-1) {// down
                for (i=0;i<level.bricks;i++)
					if (level.brick[i].sel) {
						int	_abc;
						#define SWAP(a,b) {_abc=b; b=a; a=_abc;}
						SWAP(level.brick[i].m_dir[curnumber+1],level.brick[i].m_dir[curnumber]);
						SWAP(level.brick[i].m_dist[curnumber+1],level.brick[i].m_dist[curnumber]);
						SWAP(level.brick[i].m_time[curnumber+1],level.brick[i].m_time[curnumber]);
						}
				} // down
			}
		if (e==0x4b) {curitem--; if (curitem<0) curitem=2;} // left
		if (e==0x4d) {curitem++; if (curitem>2) curitem=0;} // right arrow
		if (e==0x48) {curnumber--; if (curnumber<0) curnumber=MAXMOVES-1;} // up
		if (e==0x50) {curnumber++; if (curnumber>=MAXMOVES) curnumber=0;} // down

		if (c=='a')	{ // add movement to end of list
			for	(i=0;i<level.bricks;i++)
				if (level.brick[i].sel)	{
					j=level.brick[i].m_count; // j is quicker to type than level.rarara
					level.brick[i].m_dir[j]='u';
					level.brick[i].m_dist[j]=0;
					level.brick[i].m_time[j]=1000;
					level.brick[i].m_count++;
					}
			}
		if (c=='l')	{ // delete all movements
			GText("\nAre you sure you want to delete all movements? [y/n]");
			B_Show();
			i=tolower(getch());
			if (i=='y')
				for	(i=0;i<level.bricks;i++)
					if (level.brick[i].sel)	level.brick[i].m_count=0;
			}
		if (c=='d')	{
			GText("\nAre you sure you want to delete the selected movement? [y/n]");
			B_Show();
			i=tolower(getch());
			j=curnumber+1;
			if (i=='y') {
				if (j>0	&& j<=MAXMOVES)	// go 4 it!
					for	(i=0;i<level.bricks;i++)
						if (level.brick[i].sel && j<=level.brick[i].m_count) {
							memcpy(&level.brick[i].m_dir[j-1],&level.brick[i].m_dir[j],
							   sizeof(level.brick[0].m_dir[0])*(level.brick[i].m_count-j));
							memcpy(&level.brick[i].m_dist[j-1],&level.brick[i].m_dist[j],
						   sizeof(level.brick[0].m_dist[0])*(level.brick[i].m_count-j));
							memcpy(&level.brick[i].m_time[j-1],&level.brick[i].m_time[j],						   sizeof(level.brick[0].m_time[0])*(level.brick[i].m_count-j));
							level.brick[i].m_count--;
							} // if brick.sel && brick.count
				}
			}
		if (c==13)	{
			if (curitem==0) { // direction
                GText("\nDirection as Up/Down/Left/Right:");
				B_Show();
				text[0]=tolower(getch());
				if ('a'<=text[0] && text[0]<='z')
	                for (i=0;i<level.bricks;i++)
						if (level.brick[i].sel)
							level.brick[i].m_dir[curnumber]=text[0];
				}
			if (curitem==1) { // distance
                GText("\nDistance in pixels:");
				GetString(text);
				if (text[0])
	                for (i=0;i<level.bricks;i++)
						if (level.brick[i].sel)
							level.brick[i].m_dist[curnumber]=atoi(text);
				}
			if (curitem==2) { // time
                GText("\nTake how many milliseconds to move (1000=1 sec)?:");
				GetString(text);
				if (text[0])
	                for (i=0;i<level.bricks;i++)
						if (level.brick[i].sel)
							level.brick[i].m_time[curnumber]=atoi(text);
				}
			} // if c==3
	} while	(!done);
	}

void	CopyMoves(int dest,int src)
	{
	if (dest<0 || dest>=level.bricks || src<0 || src>=level.bricks) return; // out of range
	memcpy(level.brick[dest].m_dir,level.brick[src].m_dir,sizeof(level.brick[dest].m_dir));
	memcpy(level.brick[dest].m_dist,level.brick[src].m_dist,sizeof(level.brick[dest].m_dist));
	memcpy(level.brick[dest].m_time,level.brick[src].m_time,sizeof(level.brick[dest].m_time));
	level.brick[dest].m_count=level.brick[src].m_count;
	}

void	DuplicateR()
	{
	int	i,j;
	for (i=0;i<level.bricks;i++)
		if (level.brick[i].sel)	{
			j=NewBrickwh(level.brick[i].x,level.brick[i].y,level.brick[i].wid,level.brick[i].ht,level.brick[i].hits,level.brick[i].r,level.brick[i].g,level.brick[i].b,0);
			if (GetShift()&SHIFT)
				level.brick[i].x-=(level.brick[i].wid+1);
			else
				level.brick[i].x+=level.brick[i].wid+1;
			if (j) CopyMoves(level.bricks-1,i);
			}
	}

void	DuplicateD()
	{
	int	i,j;
	for (i=0;i<level.bricks;i++)
		if (level.brick[i].sel)	{
			j=NewBrickwh(level.brick[i].x,level.brick[i].y,level.brick[i].wid,level.brick[i].ht,level.brick[i].hits,level.brick[i].r,level.brick[i].g,level.brick[i].b,0);
			if (GetShift()&SHIFT)
				level.brick[i].y-=(level.brick[i].ht+1);
			else
				level.brick[i].y+=level.brick[i].ht+1;
			if (j) CopyMoves(level.bricks-1,i);
			}
	}

void	DrawMouse(int mx,int my)
	{
	Line(mx+1,my,mx+11,my+10,0);
	Line(mx+1,my,mx+1,my+15,0);	// |
	Line(mx+11,my+10,mx+6,my+10,0);
	Line(mx+6,my+10,mx+1,my+15,0);

	Line(mx,my,mx+10,my+10,MYWHITE);
	Line(mx,my,mx,my+15,MYWHITE); // |
	Line(mx+10,my+10,mx+5,my+10,MYWHITE);
	Line(mx+5,my+10,mx,my+15,MYWHITE);
	}

void	SelectTiny() // t
	{
	 int	i;
	 for (i=0;i<level.bricks;i++) {
		if (level.brick[i].wid*level.brick[i].ht<10) level.brick[i].sel=true;
		else level.brick[i].sel=false;
		}
	 for (i=0;i<level.balls;i++) level.ball[i].sel=0;
	}

void	SetColourToBricks()	// alt up
	{
	 int	i;
	 for (i=0;i<level.bricks;i++)
		if (level.brick[i].sel)	{
		  slidred=level.brick[i].r;
		  slidgreen=level.brick[i].g;
		  slidblue=level.brick[i].b;
		  return;
		  }
	 }
void	SetBricksToColour()	// alt down
	{
	 int	i;
	 for (i=0;i<level.bricks;i++)
		if (level.brick[i].sel)	{
		  level.brick[i].r=slidred;
		  level.brick[i].g=slidgreen;
		  level.brick[i].b=slidblue;
		  }
	 }

void	ShowHelp()
	{
	GTextGoto(0,0);
	GText(HELPMESSAGE);
	}

void	SelSameBricks()
	{
	int	r,g,b,wid,ht,strength,i;

	r=-1;
	for	(i=0;i<level.bricks;i++)
		if (level.brick[i].sel)	{
			r=level.brick[i].r;
			g=level.brick[i].g;
			b=level.brick[i].b;
			wid=level.brick[i].wid;
			ht=level.brick[i].ht;
			strength=level.brick[i].hits;
			}

	if (r==-1) return; // no brick selected

	if (GetShift()&SHIFT) // check size only
		for	(i=0;i<level.bricks;i++) {
			level.brick[i].sel=
				(level.brick[i].wid==wid &&
				level.brick[i].ht==ht);
			}
	else	// check colour and strength
		for	(i=0;i<level.bricks;i++) {
			level.brick[i].sel=
				(level.brick[i].r==r &&
				level.brick[i].g==g	&&
				level.brick[i].b==b	&&
				level.brick[i].hits==strength);
		   }
	}

#define	INSIDE(x,l,r) (x>=l	&& x<=r)
void	SelectRegion(int x1,int	y1,int x2,int y2)
	{
	int	left,top,right,bottom,ins,sh;

	if (x1<x2) {left=x1; right=x2;}
	else {left=x2;right=x1;}
	if (y1<y2) {top=y1;bottom=y2;}
	else {top=y2;bottom=y1;}

	sh=GetShift()&SHIFT;
	int	i;
	for	(i=0;i<level.balls;i++)	{
		ins=(INSIDE(level.ball[i].x,left,right)	&&
		INSIDE(level.ball[i].y,top,bottom));
		if (!sh) level.ball[i].sel=ins;
		else level.ball[i].sel^=ins;
		}
	for	(i=0;i<level.bricks;i++) {
		ins=(INSIDE(level.brick[i].x,left,right) &&
		INSIDE(level.brick[i].x+level.brick[i].wid,left,right) &&
	   	INSIDE(level.brick[i].y,top,bottom)	&&
		INSIDE(level.brick[i].y+level.brick[i].ht,top,bottom));
		if (!sh) level.brick[i].sel=ins;
		else level.brick[i].sel^=ins; // invert it
	   }
	}

void	GetNameBit(char	*fn,char *name)
	{
	 strcpy(name,fn);
	 if	(strrchr(name,'.'))	*strrchr(name,'.')=0;
	 int	i;
	 for (i=strlen(name)-1;i>0 && isdigit(name[i]);i--)	name[i]=0;
	 }

void	GetExtBit(char *fn,char	*name)
	{
	 name[0]=0;
	 if	(strrchr(fn,'.')) strcpy(name,strrchr(fn,'.')+1);
	 }

int	GetCurNumber(char *fn)
	{
	 char	name[100];
	 strcpy(name,fn);
	 if	(strrchr(name,'.'))	*strrchr(name,'.')=0;
	 int	i;
	 for (i=strlen(name)-1;i>0 && isdigit(name[i]);i--);
	return atoi(name+i+1);
	 }

void	CenterHorizontally()
	{
	 int	summids,i,avgmid,delta;
	 for (i=summids=0;i<level.bricks;i++) {
		summids+=(level.brick[i].x+level.brick[i].wid/2);
		}
	avgmid=summids/level.bricks;
	// the average middle should be 320
	delta=320-avgmid; // what needs to be added to the bricks
	for	(i=0;i<level.bricks;i++) {
	   level.brick[i].x+=delta;
	   }
	}

void	OpenLast(char *fn)
	{
	char	namebuf[200],extbuf[10],text[100];

	SaveLevel(fn);
	if (GetCurNumber(fn)<2)	return;	// can't go below level 1
	GetNameBit(fn,namebuf);	GetExtBit(fn,extbuf);
	sprintf(text,"%s%d.%s",namebuf,GetCurNumber(fn)-1,extbuf);

	if (!FileExists(text) && !(GetShift()&SHIFT)) Flash(); // have to hold down shift to create a new file
	else	{
		strcpy(fn,text);
		LoadLevel(fn); MakeBackup(fn);
		}
	}

void	OpenNext(char *fn)
	{
	char	namebuf[200],extbuf[10],text[100];

	SaveLevel(fn);
	GetNameBit(fn,namebuf);	GetExtBit(fn,extbuf);
	sprintf(text,"%s%d.%s",namebuf,GetCurNumber(fn)+1,extbuf);
	if (!FileExists(text) && !(GetShift()&SHIFT)) Flash(); // have to hold down shift to create a new file
	else {
		strcpy(fn,text);
		LoadLevel(fn); MakeBackup(fn);
		}
	}

void	EditLevel(char *filename)
	{
	int	c,k,quit,mx,my,ombl,ombr,mbl,mbr,i,j,
		dx,dy,sdx,sdy,sldx,sldy,drag; // delta mouse move
	char	shortname[100],textbuf[100],fn[1000];
	long	lasttime; // save it to a temp file every 20 seconds, which gets
	// left behind in the case of a lockup or something bad

	strcpy(fn,filename);

	if (strrchr(fn,'\\')) strcpy(shortname,strrchr(fn,'\\')+1);
	else strcpy(shortname,fn);

	if (Init())	return;
	mreset();
	msetborders(0,0,639,479);
	msetpos(320,240);

	LoadLevel(fn); MakeBackup(fn);
	//SaveLevel("temp.inf");
	lasttime=clock();
	quit=0;	mbl=mbr=0; drag=0; grid=0; help=0;
	slidred=slidblue=slidgreen=128;
	do {
	  i=mx;	j=my; mx=mgetx(); my=mgety(); dx=mx-i; dy=my-j;
	  ombl=mbl;	ombr=mbr;
	  mbl=mbutton(1); mbr=mbutton(2);

	  // colour sliders!
	  if (mbl && mx>=182 &&	mx<=457	&& my<=32) {
		i=mx-192; if (i<0) i=0;	if (i>255) i=255;
		if (my<=11)	slidred=i;
		else if	(my<=22) slidgreen=i;
		else slidblue=i;
		}

	  if (!ombr	&& mbr)	{sdx=mx; sdy=my;} //start right drag
	  if (ombr && !mbr)	{
		if (GetShift()&CTRL)
		  	SelectRegion(sdx,sdy,mx,my);
		else
			NewBrick(sdx,sdy,mx,my);
		} // end right drag

	  if (!ombl	&& mbl && !(mx>=182	&& mx<=457 && my<=32)) { // cant be in sliders area
		sldx=mx; sldy=my;
		if (!(GetShift()&CTRL))	{NoSel(); AddToSel(mx,my,1);}
		} // left just pressed (undo here)
	  if (ombl && mbl && !(mx>=182 && mx<=457 && my<=32)) {	// drag
			if (abs(sldx-mx)>5 || abs(sldy-my)>5) drag = 1;
			if (drag) MoveStuff(dx,dy);}
	  if (ombl && !mbl)	{
		if(!drag) {if (GetShift()&CTRL)	AddToSel(mx,my);}
		drag=0;} //MoveStuff(0,0,1);

	  while	(kbhit()) {
			k=0;
			c=tolower(getch());	if (!c)	k=getch();

			// with control, the arrows move the bricks 5x faster
			if (k==0x8d) MoveStuff(0,-5); // up
			if (k==0x91) MoveStuff(0,5); // down
			if (k==0x73) MoveStuff(-5,0); // left
   			if (k==0x74) MoveStuff(5,0); // right

			if (!(GetShift()&SHIFT)) {
				if (k==72) MoveStuff(0,-1);	// up
				if (k==80) MoveStuff(0,1); // down
				if (k==75) MoveStuff(-1,0);	// left
				if (k==77) MoveStuff(1,0); // right
				}
			else {
			  	if (k==72) SetColourToBricks();// up
				if (k==80) SetBricksToColour();// down
			  	}
			if (k==83) DeleteSel();	// delete all selected things
			if (k==82) AddBall(mx,my); // insert
			if (k==0x44) ScreenShot(); // screenshot -> f10
			if (c==13) EditStuff();	// enter
			if (c=='a')	for	(i=0;i<level.bricks;i++) level.brick[i].sel=1;
			if (c=='+')	DuplicateR();
			if (c=='-')	DuplicateD();
			if (c==' ')	{if	(!(GetShift()&CTRL)) NoSel(); AddToSel(mx,my);}
			if (c=='q' || c==27) quit=true;	// q or escape
			if (c=='/')	SplitHalf();
			if (c=='*')	SplitQuad();
			if (c=='n')	NoSel();
			if (c=='s')	SelSameBricks();
			if (c=='c')	ClipAll();
			if (c=='h')	CenterHorizontally();
			if (c=='m')	EditBrickMovement();
			if (k==0x3c) SaveLevel(fn);	// f2
			if (c==',' || c=='<') {
			  	OpenLast(fn);
				if (strrchr(fn,'\\')) strcpy(shortname,strrchr(fn,'\\')+1);
				else strcpy(shortname,fn);
			  }
			if (c=='.' || c=='>') {
			  OpenNext(fn);
				if (strrchr(fn,'\\')) strcpy(shortname,strrchr(fn,'\\')+1);
				else strcpy(shortname,fn);
			  }
			if (c=='l')	ChangeLevelName();
			if (c=='t')	SelectTiny();
			if (c=='d')	KillDuplicates();
			if (k==59) help=!help; // f1
			if (c=='g')	grid=!grid;
			if (c=='[')	{ // darker
			  	slidred=MAX(0,slidred-42);
			  	slidgreen=MAX(0,slidgreen-42);
			  	slidblue=MAX(0,slidblue-42);
			  	}
			if (c==']')	{ // brighter
			  	slidred=MIN(255,slidred+42);
			  	slidgreen=MIN(255,slidgreen+42);
			  	slidblue=MIN(255,slidblue+42);
			  	}
			}
	  if (clock()>lasttime+20*CLOCKS_PER_SEC) {
		lasttime=clock();
		SaveLevel(fn);
		}

	  B_Clear(); // clear buffer
	  GTextGoto(0,0); GText("LevelEdit %s\nPress F1 for help",VERSIONDESC);
	  GTextGoto(640-strlen(shortname)*8,0);	GText("%s",shortname);
	  GTextGoto(640-strlen(level.name)*8,8); GText("%s",level.name);

	  sprintf(textbuf,"%d Brick%s, %d Ball%s",level.bricks,level.bricks==1?"":"s",level.balls,level.balls==1?"":"s");
	  GTextGoto(640-strlen(textbuf)*8,16); GText(textbuf);

	  j=0; for (i=0;i<level.bricks;i++)	if (level.brick[i].sel)	j++;
	  sprintf(textbuf,"Selected %d Brick%s",j,j==1?"":"s");
	  GTextGoto(640-strlen(textbuf)*8,24); GText(textbuf);

	  DisplayLevel();
	  DisplayColourSliders();
	  if (help)	ShowHelp();
	  if (ombr && mbr) DrawMyBox(mx,my,sdx,sdy,MYWHITE);
	  DrawMouse(mx,my);
	  B_Show();
	  UnFlash(); // set palette entry 0 back to black
	  }	while(!quit);

	while (kbhit())	getch();

	Deinit();
	SaveLevel(fn);
	//unlink("temp.inf");
	}
