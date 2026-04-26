#include <stdio.h>
#include <conio.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>

#define	COMMANDHELP	"LVLMAKER16: compile an entire episode of BrickWarrior\n"\
	"Version for brickwarrior 16: supports MOVING BRICKS\n"\
	"INF files into a single LVL file\n"\
	"and vice-versa\n"\
	"Usage: LVLMAKER <inf/lvl-file>\n"\
	"<inf-file> can be any of the levels from the episode, LVLMAKER will figure\n"\
	" out the rest of the levels\n"\
	"<lvl-file> is a .LVL file\n"\
	"Example: LVLMAKER chris5.inf\n"\
	"LVLMAKER will automatically go to chris1.inf and keep on going through\n"\
	"chris2, chris3, etc until, say, it cannot find chris21.inf\n"\
	"try going into win95 explorer and just dragging an INF/LVL file over the LVLMAKER\n"\
	"icon.\n"\
	"Example: LVLMAKER chris.lvl\n"\
	"will expand all the levels in chris.lvl to chris1.inf, chris2.inf etc\n"
#define	LVLFILEID   0x1a4c564c	// LVL<eof>
#define	LV2FILEID   0x1a32564c	// LV2<eof>

void	CompileLevels(char *fn);
void	DeCompileLevels(char *fn);
int	main(int argc,char **args)
	{
	if (argc<2)
		printf(COMMANDHELP);
	else {
		if (strrchr(args[1],'.') && !strnicmp(strrchr(args[1],'.')+1,"LVL",3))
			DeCompileLevels(args[1]);
		else
			CompileLevels(args[1]);
		}
	return 0;
	}

void	GetNameBit(char *fn,char *name)
	{
	strcpy(name,fn);
	if (strrchr(name,'.')) *strrchr(name,'.')=0;
	int	i;
	for (i=strlen(name)-1;i>0 && isdigit(name[i]);i--) name[i]=0;
	}

int	FileExists(char *fn) {
	FILE	*fh;
    fh=fopen(fn,"rb");
    if (fh==NULL) return false; // doesnt exist
    fclose(fh); return true; // does!
	}

void	CopyFile(char *src,char *dest)
	{
     FILE	*in,*out;
     if ((in=fopen(src,"rb"))==NULL) return;
     if ((out=fopen(dest,"wb"))==NULL) {fclose(in); return;}

       char	c;
       while (!feof(in)) {
         c=fgetc(in);
         if (feof(in)) break;
         fputc(c,out);
         }
       fclose(out);
       fclose(in);
     }

void	MakeBackup(char *fn) {
	char	backupname[1024],shortname[20];

	// dont bother if FN doesnt exist to back up
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

int	GetNumLevels(char *basename)
	{
	int	curlevel;
	char	textbuf[100];

	for (curlevel=1;curlevel<100;curlevel++)
		{
		sprintf(textbuf,"%s%d.inf",basename,curlevel);
		if (!FileExists(textbuf)) break;
		}
	return curlevel-1;
	}

#define	MAXBRICKS 1000
#define	MAXBALLS  10
#define MAXMOVES  10
struct	_level {
	struct	_brick {
    	long	x,y,wid,ht,r,g,b,hits,moves;
		char	m_dir[MAXMOVES];
		long	m_dist[MAXMOVES],
				m_time[MAXMOVES];
    	} brick[MAXBRICKS];
	struct	_ball {
    	long	x,y,xv,yv,active;
    	} ball[MAXBALLS];
	long	bricks,balls;
    char	name[1024];
	} level;

void	LoadLevel(char *fn) {
	enum	{uptobricks=1,uptoballs};
	FILE	*fh;
	char	line[1024],textbuf[200],textbuf2[200];
	int		upto,i,j;
	long	x,y,xv,yv,wid,ht,c,hits,active,moves; // **changed for LV2

	memset(level.name,0,100); // no name
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
				if (upto==uptobricks) { // brick data
					sscanf(line,"{%d,%d,%d,%d,%x,%d,%d,%d",&x,&y,&wid,&ht,&c,&hits,&moves); // **changed LV2
					if (wid>=2 && ht>=2 && level.bricks<MAXBRICKS) {
						level.brick[level.bricks].x=x;
						level.brick[level.bricks].y=y;
						level.brick[level.bricks].wid=wid;
						level.brick[level.bricks].ht=ht;
						level.brick[level.bricks].hits=hits;
						level.brick[level.bricks].r=(c>>16)&0xff;
						level.brick[level.bricks].g=(c>>8)&0xff;
						level.brick[level.bricks].b=(c   )&0xff;
						if (moves<0 || moves>MAXMOVES) moves=0;
						level.brick[level.bricks].moves=moves;
						if (moves) strcpy(textbuf,strchr(line,'[')+1);
						for (i=0;i<moves;i++) {
							j=0;
							sscanf(textbuf,"%c,%d,%d,%s",&j,
                                &level.brick[level.bricks].m_dist[i],
                                &level.brick[level.bricks].m_time[i],
								textbuf2);
							j=tolower(j);
							if (j=='u') j=1; if (j=='r') j=2;
							if (j=='d') j=3; if (j=='l') j=4;
                            level.brick[level.bricks].m_dir[i]=j;
							strcpy(textbuf,textbuf2);
							}
						level.bricks++;
						}
					} // brick data
				if (upto==uptoballs) {
					sscanf(line,"{%d,%d,%d,%d,%d",&x,&y,&xv,&yv,&active);
					if (level.balls<MAXBALLS) {
						level.ball[level.balls].x=x;
						level.ball[level.balls].y=y;
                        level.ball[level.balls].xv=xv;
                        level.ball[level.balls].yv=yv;
						level.ball[level.balls].active=active;
						level.balls++;
						}
					} // ball data
				} // line[0]=='{'
			} // while !feof
		fclose(fh);
		return; // success
		}
	return; // conked out - doesnt matter much though
	}

void	WriteLevel(FILE *fn,char *inffile)
	{
	LoadLevel(inffile);

	// now write it to FN
	fwrite(level.name,100,1,fn);

	int	i,j;

	fwrite(&level.balls,2,1,fn);
	for (i=0;i<level.balls;i++) {
		fwrite(&level.ball[i].x,2,1,fn);
		fwrite(&level.ball[i].y,2,1,fn);
		fwrite(&level.ball[i].xv,2,1,fn);
		fwrite(&level.ball[i].yv,2,1,fn);
		fwrite(&level.ball[i].active,1,1,fn);
		}

	fwrite(&level.bricks,2,1,fn);
	for (i=0;i<level.bricks;i++) {
		fwrite(&level.brick[i].x,2,1,fn);
		fwrite(&level.brick[i].y,2,1,fn);
		fwrite(&level.brick[i].wid,2,1,fn);
		fwrite(&level.brick[i].ht,2,1,fn);
		fwrite(&level.brick[i].hits,1,1,fn);
		fwrite(&level.brick[i].r,1,1,fn);
		fwrite(&level.brick[i].g,1,1,fn);
		fwrite(&level.brick[i].b,1,1,fn);
		fwrite(&level.brick[i].moves,1,1,fn);
		for (j=0;j<level.brick[i].moves;j++) {
			fwrite(&level.brick[i].m_dir[j],1,1,fn);
			fwrite(&level.brick[i].m_dist[j],2,1,fn);
			fwrite(&level.brick[i].m_time[j],2,1,fn);
			}
		}
	// cool! done!
	}

void	CompileLevels(char *fn)
	{
	char	basename[100], // ie 'c:\games\brick\chris'
			textbuf[100];

	// check!
	if (!FileExists(fn)) {printf ("Error: %s missing!",fn); return;}

	GetNameBit(fn,basename);

	// backup the lvl file
	sprintf(textbuf,"%s.lvl",basename); MakeBackup(textbuf);

	// go 4 it!
	FILE	*fh;
	fh=fopen(textbuf,"wb");
	if (fh==NULL) {printf("Error opening %s!!",textbuf);return;}

	long	i,j,levels,curlevel,leveloffs[100];

	i=LV2FILEID; fwrite(&i,4,1,fh);	// ID **changed for LV2
	levels=GetNumLevels(basename);
	fwrite(&levels,4,1,fh); // # of levels
	for (i=0;i<levels;i++) {
		leveloffs[i]=0; // dont know yet, just setting aside room
		fwrite(&leveloffs[i],4,1,fh);
		}

	// any comments and stuff can go here in the file...
	fprintf(fh,"\r\nThis file stores an episodes worth of levels for BrickWarrior\r\n"\
		"This episode contains %d levels\r\n",levels);

	cprintf ("WRITING %s\r\n%d levels",textbuf,levels);
	// write all the levels' data
	for (curlevel=1;curlevel<=levels;curlevel++) {
		leveloffs[curlevel-1]=ftell(fh);
		sprintf(textbuf,"%s%d.inf",basename,curlevel);
		WriteLevel(fh,textbuf);
		}

	fseek(fh,8,SEEK_SET); // do the level offsets properly now
	for (i=0;i<levels;i++)
		fwrite(&leveloffs[i],4,1,fh);

	// DONE!
	fclose(fh);

	cprintf("\r\nDone!\r\n");

	printf ("Remember, to get the INF files back out again if you delete them,\n"\
		"drag the LVL file onto the LVLMAKER icon (this program is LVLMAKER)\n");
	}

void	DeCompileLevel(FILE *in,char *filename,int islv2)
	{
	FILE	*out;
	char	levelname[100];
	int		balls,bricks,i,j,x,y,wid,ht,active,r,g,b,strength,moves,dir,dist,time;
	short	xv,yv;

	MakeBackup(filename);
	out=fopen(filename,"wb"); if (out==NULL) return;

	fread(levelname,100,1,in);

    fprintf(out,"; BrickWarrior Level file\r\n"\
			"; coords go from 0,0 to 640,480\r\n"\
            "; balls {x,y,xvel,yvel,active}\r\n"\
            "; bricks {x,y,wid,ht,RRGGBB,strength(0=indestructable),\r\n"\
			";  moves}[movedir1,movedist1,movetime1,...]\r\n\r\n"\
            "Name = \x22%s\x22\r\n\r\n",levelname);

	balls=0; fread(&balls,2,1,in);
	fprintf(out,"[Balls]\r\n");
	for (i=0;i<balls;i++) {
		x=y=xv=yv=active=0;
		fread(&x,2,1,in); fread(&y,2,1,in);
		fread(&xv,2,1,in); fread(&yv,2,1,in);
		fread(&active,1,1,in);
		fprintf (out,"{%d,%d,%d,%d,%d}\r\n",x,y,xv,yv,active);
        }

	bricks=0; fread(&bricks,2,1,in);
	fprintf(out,"\r\n[Bricks]\r\n");
	for (i=0;i<bricks;i++) {
		x=y=wid=ht=r=g=b=strength=moves=0;
		fread(&x,2,1,in); fread(&y,2,1,in);
        fread(&wid,2,1,in); fread(&ht,2,1,in);
		fread(&strength,1,1,in);
        fread(&r,1,1,in); fread(&g,1,1,in); fread(&b,1,1,in);
        if (islv2) fread(&moves,1,1,in); // **new in LV2 file format
		fprintf (out,"{%d,%d,%d,%d,%02x%02x%02x,%d,%d}",x,y,wid,ht,r,g,b,strength,moves);
		if (moves) {
			fprintf (out,"[");
			for (j=0;j<moves;j++) {
				x=dir=dist=time=0;
				fread (&x,1,1,in); // direction
				if (x==1) dir='u'; if (x==2) dir='r';
				if (x==3) dir='d'; if (x==4) dir='l';
	    		fread (&dist,2,1,in); fread(&time,2,1,in);
				if (j) fprintf(out,",");
				fprintf(out,"%c,%d,%d",dir,dist,time);
				}
			fprintf (out,"]");
			}
		fprintf (out,"\r\n");
        }

	fclose(out);
	}

void	DeCompileLevels(char *fn)
	{
	char	basename[100], // ie 'c:\games\brick\chris'
			textbuf[100];

	// check!
	if (!FileExists(fn)) {printf ("Error: %s missing!",fn); return;}

	GetNameBit(fn,basename);

	// go 4 it!
	FILE	*fh;
	fh=fopen(fn,"rb");
	if (fh==NULL) {printf("Error opening %s!!",fn);return;}

	long	levels,leveloffs[100],i,id;

	fread(&id,4,1,fh);	// ID i should equal LVLFILEID or LV2FILEID
	fread(&levels,4,1,fh); // # of levels
	for (i=0;i<levels;i++) // their offsets
		fread(&leveloffs[i],4,1,fh);

	cprintf("DeCompiling %s (%d levels)",fn,levels);

	// get all the levels' data
	for (i=0;i<levels;i++) {
		fseek(fh,leveloffs[i],SEEK_SET);
		sprintf(textbuf,"%s%d.inf",basename,i+1);
		DeCompileLevel(fh,textbuf,id==LV2FILEID);
		}

	// DONE!
	fclose(fh);

	cprintf("\r\nDone!\r\n");

	printf ("Remember, to compact your files back into a LVL file,\n"\
		"run LVLMAKER (this program) with any of the INF files\n");
	}


