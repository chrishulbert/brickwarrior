// TODO refactor non-game-specific stuff eg drawing into their own helpers.

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game.h"
#include "image.h"
#include "endian.h"
#include "helpers.h"

#define	VERSIONNAME		"Version 20"
#define	HIGHSCORES		20
#define	SAVEGAMES		6
#define	MAXEPISODES		20
#define	MAXBRICKS		1000 // a lot!!!
#define	MAXPOWERUPS		10 // Powerups on screen.
#define	MAXBALLS		9
#define	MAXMOVES		10
#define	BALLWID			12
#define	BALLHT			12
#define	POWERUPCHANCE	5 // One in N bricks has a powerup.

typedef enum {
	PWR_BUBBLE=0,
	PWR_FAST,
	PWR_SLOW,
	PWR_THIN,
	PWR_WIDE,
	PWR_TRIPLE,
	PWR_UPDOWN,
	PWR_KILL,
	PWR_CATCH,
	PWR_PROTECTION,
	PWR_BLACKBUBBLE,
	PWR_RANDOM,
	PWR_LAST,
} PowerupType;

typedef struct {
	PowerupType	type;
	int x;
	int y;
	bool isCatchable;
} Powerup; // Falling powerups.

typedef struct {
	int x;
	int y;
	int xv;
	int yv;
	int active;
	int type; // 3=laser, 4=purple. TODO make this an enum?
} Ball;

typedef struct {
	int x, y, wid, ht, hits, flashtime;
	uint32_t colour;
	uint32_t high, higher, low; // Edge colours.
	bool hasEdge; // Chamfered edge style.
	char moves, curmove,
		movedir[MAXMOVES]; // 1=up 2=right 3=down 4=left
	int curmovetime, curmovex, curmovey, // destination x,y
		movedist[MAXMOVES],
		movetime[MAXMOVES];
} Brick;

typedef struct {
	char	name[100], basename[20], backname[30];
	int		levels;
} Episode;

typedef struct {
	char name[100], episodename[100];
	int score, level;
} Highscore;

typedef enum {
	SCREEN_TITLE=0,
	SCREEN_HIGHSCORE,
	SCREEN_GAME,
	SCREEN_TYPENAME,
	SCREEN_ABOUT,
} Screen;

typedef enum {
	MENU_MAIN=0,
	MENU_PLAYERS, // How many players. TODO remove
	MENU_EPISODE, // Choose an episode.
	MENU_MUSIC, // CD player - todo remove?
	MENU_GAME, // Play/load/save.
	MENU_LOAD, // Load game.
	MENU_SAVE, // Save game.
} Menu;

static struct {
	// // Sound:
	// CDXSound *Sound;
	// CDXSoundBuffer *sndKillBlock;
	// CDXSoundBuffer *sndTinkBlock;
	// CDXSoundBuffer *sndBounce; // Off edge of screen.
	// CDXSoundBuffer *sndMult100;
	// CDXSoundBuffer *sndMult500;
	// CDXSoundBuffer *sndMult1000;
	// CDXSoundBuffer *sndMult1500;
	// CDXSoundBuffer *sndMult2000;
	// CDXSoundBuffer *sndExtraBall;
	// CDXSoundBuffer *sndNewHighscore;
	// CDXSoundBuffer *sndIntro;
	// CDXSoundBuffer *sndLoseBall;
	// // Powerups:
	// CDXSoundBuffer *sndBubble;
	// CDXSoundBuffer *sndCoolBall;
	// CDXSoundBuffer *sndFast;
	// CDXSoundBuffer *sndSlow;
	// CDXSoundBuffer *sndThin;
	// CDXSoundBuffer *sndWide;
	// CDXSoundBuffer *sndTripleBall;
	// CDXSoundBuffer *sndLotsBall;
	// CDXSoundBuffer *sndUpdown;
	// CDXSoundBuffer *sndGetKill;
	// CDXSoundBuffer *sndRecover;
	// CDXSoundBuffer *sndCatch;
	// CDXSoundBuffer *sndProtection;

	bool hasQuit;

	Image *sprites;
	Image *font8;
	Image *font16;
	Image *font16blue;
	Image *background;

	// Powerups:
	Powerup powerup[MAXPOWERUPS];
	int powerups;
	bool hasUpdown; // Have they got the updown?
	bool isKilled; // Is the paddle red?
	bool hasProtection; // Have protection bar?
	bool hasCatch;
	bool isCaught;
	int caughtBall; // Have they got a catch?

	bool needRedraw; // Need to redraw the level? (# of bricks changed).

	// Paddle stuff:
	int padx;
	int pady;
	int padwidth;
	int pdeltax;
	int pdeltay; // How far the paddle moved this frame.

	Ball ball[MAXBALLS];
	int balls;

	Brick brick[MAXBRICKS];
	int bricks;

	int players; // One or two players.
	char levelname[100];
	int bricksLeft;

	// Episodes:
	Episode episode[20];
	int episodes, curEpisode, curLevel, curBalls,
		ballStartX, ballStartY, ballStartXV, ballStartYV, ballStartActive;

	// Menu:
	int	menuopts, curopt, curmenu;
	int inMenu; // 0 = not in menu, 1=?, 2=special value? TODO figure out meaning.
	int mouseposx, mouseposy, lastmouse;
	Screen whatScreen;

	// Scoring:
	int mult, score;

	// Framerate:
	int curframe, framerateinc, framerate, timesincelasthit;

	// Highscores:
	Highscore highscore[HIGHSCORES];
	char curnametext[100];
	int curnameoff;
} state;

// Forward declarations:
void save_high_scores();
void draw_rect(uint32_t *framebuffer, int x, int y, int width, int height, uint32_t colour);
void draw_text(uint32_t *framebuffer, char *text, Image* image, int x, int y);
void load_proper_back();
void load_episodes(char *fn);
void load_high_scores();

// Sokol game lifecycle:

void game_init() {
	state.sprites = image_load("assets/img/sprites.png");
	state.font8 = image_load("assets/img/font8.png");
	state.font16 = image_load("assets/img/font16.png");
	state.font16blue = image_load("assets/img/font16blue.png");

	state.whatScreen=SCREEN_TITLE;
	load_proper_back();

	state.hasQuit=false;
	state.inMenu=0;
	state.curmenu=MENU_MAIN;
	state.curopt=0; // Play!
	state.menuopts=4;

	// TODO what do we do re mouse?
	state.mouseposx=320;
	state.mouseposy=240;
	state.lastmouse=false;

	state.curframe=state.framerateinc=state.framerate=0;
	srand(time(NULL));

	// Sounds:
	// TODO use dr_wav header library.
	// sndKillBlock.Load(Sound,"sounds\\killblok.wav");
	// sndTinkBlock.Load(Sound,"sounds\\tinkblok.wav");
	// sndBounce.Load(Sound,"sounds\\bounce.wav");
	// sndExtraBall.Load(Sound,"sounds\\extrabal.wav");
	// sndMult100.Load(Sound,"sounds\\mult100.wav");
	// sndMult500.Load(Sound,"sounds\\mult500.wav");
	// sndMult1000.Load(Sound,"sounds\\mult1000.wav");
	// sndMult1500.Load(Sound,"sounds\\mult1500.wav");
	// sndMult2000.Load(Sound,"sounds\\mult2000.wav");
	// sndNewHighscore.Load(Sound,"sounds\\newhiscr.wav");
	// sndIntro.Load(Sound,"sounds\\intro.wav");
	// sndLoseBall.Load(Sound,"sounds\\loseball.wav");
	// // powerups below
	// sndBubble.Load(Sound,"sounds\\bubble.wav");
	// sndCoolBall.Load(Sound,"sounds\\coolball.wav");
	// sndFast.Load(Sound,"sounds\\fast.wav");
	// sndSlow.Load(Sound,"sounds\\slow.wav");
	// sndThin.Load(Sound,"sounds\\thin.wav");
	// sndWide.Load(Sound,"sounds\\wide.wav");
	// sndTripleBall.Load(Sound,"sounds\\triple.wav");
	// sndLotsBall.Load(Sound,"sounds\\lotsball.wav");
	// sndUpdown.Load(Sound,"sounds\\updown.wav");
	// sndGetKill.Load(Sound,"sounds\\getkill.wav");
	// sndRecover.Load(Sound,"sounds\\recover.wav");
	// sndCatch.Load(Sound,"sounds\\catch.wav");
	// sndProtection.Load(Sound,"sounds\\protect.wav");

	load_episodes("assets/levels/episodes.inf");
	load_high_scores();
}

void game_deinit() {
	image_free(state.sprites);
	image_free(state.font8);
	image_free(state.font16);
	image_free(state.font16blue);
	image_free(state.background);
	save_high_scores();
}

void game_update(float duration) {
}

void game_draw(uint32_t *framebuffer) {
	memcpy(framebuffer, state.background->data, SCREENWIDTH * SCREENHEIGHT * 4);
}

// Game-specific functions:

void set_pause(bool on) {
	if (on && !state.inMenu) {
		state.curmenu = MENU_MAIN;
		state.curopt = 0;
		state.menuopts = 5;
		state.inMenu = 1;
	}
}

// Converts 0-255 rgb into an 0xAABBGGRR colour.
// Clamps it for you.
uint32_t rgb(int r, int g, int b) {
	#define MAX(a, b) ((a) > (b) ? (a) : (b))
	#define MIN(a, b) ((a) < (b) ? (a) : (b))
	#define CLAMP(a) (MAX(MIN(a, 255), 0))
	return 0xff000000 +
		(((uint32_t)CLAMP(b)) << 16) +
		(((uint32_t)CLAMP(g)) << 8) +
		((uint32_t)CLAMP(r));
}

// Loads a level in LVL binary format.
// Returns true if successful.
bool load_level_lvl(char *filename, int level) {
	#define	LVLFILEID 0x1a4c564c // LVL<eof>
	#define	LV2FILEID 0x1a32564c // LV2<eof>

	int k, t, x, y, wid, ht, r, g, b, hits, active, moves, dir, time;
	int movedir[MAXMOVES], movedist[MAXMOVES], movetime[MAXMOVES];
	int xv, yv, dist;

	FILE* fh = fopen(filename, "rb");
	if (!fh) { return false; }
	int32_t fileId = fread_i32(fh);
	if (fileId!=LVLFILEID && fileId!=LV2FILEID)	{ fclose(fh); return false; }
	int32_t numberOfLevels = fread_i32(fh);
	if (level>numberOfLevels || level<=0) { fclose(fh); return false; } // Bad level #.

	fseek(fh, (level-1)*4, SEEK_CUR); // Skip to the data for the offset for this level.
	int32_t levelDataOffset = fread_i32(fh); // Get the offset to the level data.
	fseek(fh, levelDataOffset, SEEK_SET); // Jump to the level.

	// Load level name:
	fread(state.levelname, 100, 1, fh);

	// Load balls:
	int32_t ballCount = fread_i32(fh);
	if (ballCount > MAXBALLS) { fclose(fh); return false; }
	for	(int i=0; i<ballCount; i++) {
		x=0; fread(&x, 2, 1, fh);
		y=0; fread(&y, 2, 1, fh);
		xv=0; fread(&xv, 2, 1, fh);
		yv=0; fread(&yv, 2, 1, fh);
		active=0; fread(&active, 1, 1, fh);
		state.ballStartX = x*100;
		state.ballStartY = y*100;
		state.ballStartXV = xv*100;
		state.ballStartYV = yv*100;
		state.ballStartActive = active;
		if (!active) { state.ballStartXV/=2; state.ballStartYV/=2; } // Slow it down half.
	}

	// Load bricks:
	int32_t brickCount = fread_i32(fh);
	if (brickCount > MAXBRICKS) { fclose(fh); return false; }
	state.bricks=0;
	for	(int i=0; i<brickCount; i++) {
		int16_t x = fread_i16(fh);
		int16_t y = fread_i16(fh);
		int16_t wid = fread_i16(fh);
		int16_t ht = fread_i16(fh);
		unsigned char hits = fread_u8(fh);
		int r = fread_u8(fh);
		int g = fread_u8(fh);
		int b = fread_u8(fh);
		unsigned char moves = 0; if	(fileId==LV2FILEID)	{ moves = fread_u8(fh); }
		if (moves > MAXMOVES) { fclose(fh); return false; }
		for (int k=0; k<moves; k++) { // Load movements.
			movedir[k] = fread_u8(fh); // 1=up 2=right 3=down 4=left.
			movedist[k] = fread_i16(fh);
			movetime[k] = fread_i16(fh);
		}
		if (state.bricks < MAXBRICKS) {
			state.brick[state.bricks].x=x*100;
			state.brick[state.bricks].y=y*100;
			state.brick[state.bricks].wid=wid*100;
			state.brick[state.bricks].ht=ht*100;
			if (wid<=0)	{wid=1;} if (ht<=0) {ht=1;}	// To prevent div by 0 later on.
			state.brick[state.bricks].hits=hits;
			state.brick[state.bricks].hasEdge=hits!=1; // It has an edge!
			state.brick[state.bricks].flashtime=0;
			state.brick[state.bricks].colour=rgb(r, g, b);
			state.brick[state.bricks].high=rgb(r+43, g+43, b+43);
			state.brick[state.bricks].higher=rgb(r+86, g+86, b+86);
			state.brick[state.bricks].low=rgb(r-43, g-43, b-43);
			state.brick[state.bricks].moves=moves;
			state.brick[state.bricks].curmove=0;
			state.brick[state.bricks].curmovetime=0;
			for	(k=0; k<moves; k++) {
				state.brick[state.bricks].movedir[k]=movedir[k];
				state.brick[state.bricks].movedist[k]=movedist[k];
				state.brick[state.bricks].movetime[k]=movetime[k];
			}
			t=state.brick[state.bricks].movedir[0];
			if (t==1 ||	t==3) {	// up or down
				state.brick[state.bricks].curmovex=(short)x;
				if (t==1) // up
					state.brick[state.bricks].curmovey=y-state.brick[state.bricks].movedist[0];
				if (t==3) // down
					state.brick[state.bricks].curmovey=y+state.brick[state.bricks].movedist[0];
				}
			if (t==2 ||	t==4) {	// left or right
				state.brick[state.bricks].curmovey=(short)y;
				if (t==2) // right
					state.brick[state.bricks].curmovex=x+state.brick[state.bricks].movedist[0];
				if (t==4) // Left
					state.brick[state.bricks].curmovex=x-state.brick[state.bricks].movedist[0];
				}
			state.bricks++;
		} // if bricks<maxbricks
	} // for i=0 to brickCount
	fclose(fh);

	state.balls = 1;
	state.ball[0].x = state.ballStartX;
	state.ball[0].y = state.ballStartY;
	state.ball[0].xv = state.ballStartXV;
	state.ball[0].yv = state.ballStartYV;
	state.ball[0].active = state.ballStartActive;
	if (state.ball[0].type > 2) {
		state.ball[0].type = 2;	// Cannot start w/more than a bubble ball.
	}

	return true; // Success.
}

// Returns true on success.
bool load_level_inf(char *filename) {
	enum	{uptobricks=1,uptoballs};
	FILE*	fh;
	char	line[1024],textbuf[200],textbuf2[200];
	int		upto,i,j;
	int		x,y,xv,yv,wid,ht,c,r,g,b,hits,active,moves;

	fh = fopen(filename,"rb");
	if (!fh) { return false; }
	upto=0;
	while (!feof(fh)) {
		if (fgets(line,1024,fh)==NULL) { line[0]=0; }

		if (!strncasecmp(line,"Name",4)) {
			if (strchr(line,'"')) {
				strcpy(state.levelname, strchr(line,'"')+1);
				if (strrchr(state.levelname,'"'))
					*strrchr(state.levelname,'"')=0;
				}
			}
		if (!strncasecmp(line,"[Bricks]",8)) upto=uptobricks;
		if (!strncasecmp(line,"[Balls]",7)) upto=uptoballs;
		if (line[0]=='{') {
			if (upto==uptobricks) {	// brick data
				sscanf(line,"{%d,%d,%d,%d,%x,%d,%d",&x,&y,&wid,&ht,&c,&hits,&moves);
				if (state.bricks < MAXBRICKS) {
					state.brick[state.bricks].x=x*100;
					state.brick[state.bricks].y=y*100;
					state.brick[state.bricks].wid=wid*100;
					state.brick[state.bricks].ht=ht*100;
					if (wid<=0)	wid=1; if (ht<=0) ht=1;	// to prevent div by 0 later on
					state.brick[state.bricks].hits=hits;
					state.brick[state.bricks].hasEdge=(hits!=1); // it has an edge!
					state.brick[state.bricks].flashtime=0;
					int r = ((c >> 16) & 0xff);
					int g = ((c >> 8) & 0xff);
					int b = (c & 0xff);
					state.brick[state.bricks].colour=rgb(r, g, b);
					state.brick[state.bricks].high=rgb(r+43, g+43, b+43);
					state.brick[state.bricks].low=rgb(r-43, g-43, b-43);
					if (moves<0	|| moves>MAXMOVES) { moves=0; }
					state.brick[state.bricks].moves=moves;
					state.brick[state.bricks].curmove=0;
					state.brick[state.bricks].curmovetime=0;
					if (moves) { strcpy(textbuf, strchr(line,'[')+1); }
					for	(i=0; i<moves; i++) {
						char dir;
						sscanf(textbuf,"%c,%d,%d,%s",
							&dir,
							&state.brick[state.bricks].movedist[i],
							&state.brick[state.bricks].movetime[i],
							textbuf2);
						dir=tolower(dir);
						int j=0;
						if (dir=='u') { j=1; }
						if (dir=='r') { j=2; }
						if (dir=='d') { j=3; }
						if (dir=='l') { j=4; }
						state.brick[state.bricks].movedir[i]=j;
						strcpy(textbuf, textbuf2);
					}
					state.brick[state.bricks].curmove=0;
					state.brick[state.bricks].curmovetime=0;
					j=state.brick[state.bricks].movedir[0];
					if (j==1 ||	j==3) {	// up or down
						state.brick[state.bricks].curmovex=(short)x;
						if (j==1)
							state.brick[state.bricks].curmovey=y-state.brick[state.bricks].movedist[0];
						if (j==3)
							state.brick[state.bricks].curmovey=y+state.brick[state.bricks].movedist[0];
					}
					if (j==2 ||	j==4) {	// left or right
						state.brick[state.bricks].curmovey=(short)y;
						if (j==2) // right
							state.brick[state.bricks].curmovex=x+state.brick[state.bricks].movedist[0];
						if (j==4) // left
							state.brick[state.bricks].curmovex=x-state.brick[state.bricks].movedist[0];
					}
					state.bricks++;
				}
			} // brick data
			if (upto==uptoballs) {
				sscanf(line,"{%d,%d,%d,%d,%d",&x,&y,&xv,&yv,&active);
				state.ballStartX=x*100;
				state.ballStartY=y*100;
				state.ballStartXV=xv*100;
				state.ballStartYV=yv*100;
				state.ballStartActive=active;
				if (!active) { state.ballStartXV /= 2; state.ballStartYV /= 2;} // Slow it down half.
			} // ball data
		} // line[0]=='{'
	} // while !feof
	fclose(fh);

	state.balls=1;
	state.ball[0].x=state.ballStartX;
	state.ball[0].y=state.ballStartY;
	state.ball[0].xv=state.ballStartXV;
	state.ball[0].yv=state.ballStartYV;
	state.ball[0].active=state.ballStartActive;
	if (state.ball[0].type>2) { state.ball[0].type=2; } // Cannot start w/more than a bubble ball.
	return true; // success
}

void load_episodes(char *fn) {
	state.episodes=0;
	FILE* fh=fopen(fn,"rb");
	if (!fh) { return; }
	char line[1024];
	while (!feof(fh)) {
		if (fgets(line,1024,fh)==NULL) { line[0]=0; }
		if (line[0]!='{') { continue; }
		sscanf(line,"{%[^,],%d,%[^,}],%[^}]}",
			state.episode[state.episodes].name,
			&state.episode[state.episodes].levels,
			state.episode[state.episodes].basename,
			state.episode[state.episodes].backname);
		state.episodes++;
	} // while !feof
	fclose(fh);
}

void clip_powerup(int index) {
	if (state.powerup[index].y<5000) { state.powerup[index].y=5000; }
	if (state.powerup[index].x<3500) { state.powerup[index].x=3500; }
	if (state.powerup[index].x>60500) { state.powerup[index].x=60500; }
}

int	choose_powerup() {
	// Count all the existing bubbles and triples:
	int numbubbles = 0;
	int numtriples = 0;
	int numcatches = 0;
	for	(int i=0; i<state.powerups; i++) {
		if (state.powerup[i].type==PWR_BUBBLE || state.powerup[i].type==PWR_BLACKBUBBLE) numbubbles++;
		if (state.powerup[i].type==PWR_CATCH) numcatches++;
		if (state.powerup[i].type==PWR_TRIPLE) numtriples++;
	}

	while (true) { // Loop until the powerup is ok:
		int i = rand() % PWR_LAST;
		bool keepit = true;
		if (state.ball[0].type<2) { // All powerups are cool if you've got a bubble or above
			if (i==PWR_TRIPLE && (numbubbles ||	numcatches)) keepit=false;
			if ((i==PWR_BUBBLE || i==PWR_BLACKBUBBLE ||
				i==PWR_CATCH) && (numtriples ||	state.balls>1)) keepit=false;
		}
		if (keepit) {
			return i;
		}
	}
}

void new_powerup(int x, int y) {
	if (state.powerups<MAXPOWERUPS) {
		state.powerup[state.powerups].x=x;
		state.powerup[state.powerups].y=y;
		state.powerup[state.powerups].type=choose_powerup();
		state.powerup[state.powerups].isCatchable=true;
		clip_powerup(state.powerups);
		state.powerups++;
	}
}

// Get rid of all powerups (lost a ball or start a new episode).
void no_powerups()  {
	state.padwidth=7500;
	for (int i=0; i<MAXBALLS; i++) {
		state.ball[i].type=0; 
	}
	state.hasUpdown=false;
	state.isKilled=false;
	state.hasProtection=false;
	state.hasCatch=false;
	state.isCaught=false;
}

// A new ball (doesnt retain old powerups).
void new_ball()  {
	state.timesincelasthit=0;
	if (state.balls >= MAXBALLS) { return; }
	state.ball[state.balls].x=state.ballStartX;
	state.ball[state.balls].y=state.ballStartY;
	state.ball[state.balls].xv=state.ballStartXV;
	state.ball[state.balls].yv=state.ballStartYV;
	state.ball[state.balls].active=state.ballStartActive;
	state.ball[state.balls].type=0;
	state.balls++;
}

// Load the currently selected level.
void load_cur_level() {
	state.padx=32000;
	state.pady=42000; // Updown pady clipping (bottom 1/4 of the screen): 360->480
	state.bricks=0;
	state.powerups=0;
	state.isKilled=false;
	state.hasProtection=false;
	state.levelname[0] = 0;
	state.needRedraw=true;
	state.timesincelasthit=0;
	state.isCaught=false;

	// Try INF before try LVL:
	char	path[100];
	sprintf(path, "assets/levels/%s%d.inf", state.episode[state.curEpisode].basename, state.curLevel);
	if (load_level_inf(path)) { return; } // INF worked :)

	sprintf(path, "assets/levels/%s.lvl", state.episode[state.curEpisode].basename);
	load_level_lvl(path, state.curLevel);
}

void load_high_scores() {
	#define	NAMES 18
	char* name[NAMES] = {
		"Nice", "Smokey", "Reaper",
		"Jazz", "Ben", "Psycho",
		"Jumper", "Master", "Freak",
		"Insane", "The Man", "One",
		"FatMan", "Bubble", "BrickWarrior",
		"Dinosaur", "PaddleMeister", "Ripper",
	};

	// int result=RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Red Shift\\BrickWarrior\\High Scores",0,KEY_QUERY_VALUE,&key);
	for	(int i=0; i<HIGHSCORES; i++) {
		// Fill in some default names:
		strcpy(state.highscore[i].name, name[rand() % NAMES]);
		strcpy(state.highscore[i].episodename, state.episode[rand() % state.episodes].name);
		state.highscore[i].score = 100-i;
		state.highscore[i].level = 1;

		// // try to load the name
		// if (!result) {
		// 	sprintf(text, "Name%d",i+1);
		// 	amtdone=100;
		// 	RegQueryValueEx(key,text,NULL,NULL,(LPBYTE)&highscore[i].name,&amtdone);
		// 	highscore[i].name[19]=0; // max length 19 chars

		// 	amtdone=100;
		// 	sprintf(text,"Episode%d",i+1);
		// 	RegQueryValueEx(key,text,0,0,(LPBYTE)&highscore[i].episodename,&amtdone);

		// 	sprintf(text,"Score%d",i+1);
		// 	amtdone=sizeof(int);
		// 	RegQueryValueEx(key,text,0,0,(BYTE *)&highscore[i].score,&amtdone);

		// 	amtdone=sizeof(int);
		// 	sprintf(text,"Level%d",i+1);
		// 	RegQueryValueEx(key,text,0,0,(BYTE *)&highscore[i].level,&amtdone);
		// 	}
	}
	// if (!result) RegCloseKey(key);
}

void save_high_scores() {
	// TODO
	// int i;
	// char text[100];
	// HKEY key; // registry

	// RegCreateKeyEx (HKEY_LOCAL_MACHINE,
	// 			"SOFTWARE\\Red Shift\\BrickWarrior\\High Scores",
	// 			0,0,0,KEY_ALL_ACCESS,0,&key,0);

	// for	(i=0;i<HIGHSCORES;i++) {
	// 	sprintf(text,"Name%d",i+1);
	// 	RegSetValueEx(key,text,0,REG_SZ,(unsigned char *)highscore[i].name,strlen(highscore[i].name)+1);
	// 	sprintf(text,"Episode%d",i+1);
	// 	RegSetValueEx(key,text,0,REG_SZ,(unsigned char *)highscore[i].episodename,strlen(highscore[i].episodename)+1);
	// 	sprintf(text,"Score%d",i+1);
	// 	RegSetValueEx(key,text,0,REG_DWORD,(unsigned char *)&highscore[i].score,4);
	// 	sprintf(text,"Level%d",i+1);
	// 	RegSetValueEx(key,text,0,REG_DWORD,(unsigned char *)&highscore[i].level,4);
	// 	}
	// RegCloseKey	(key);
}

void add_high_score(char *name, char *episodename, int score, int level) {
	int fitswhere = -1;
	int i;
	for	(i=HIGHSCORES-1; i>=0; i--) {
		if (score >= state.highscore[i].score) {
			fitswhere=i;
		}
	}
	if (fitswhere == -1) { return; } // Score didnt get in!

	// Shuffle scores along:
	i = fitswhere; // Less typing :)
	memmove(&state.highscore[i+1], &state.highscore[i], sizeof(Highscore)*(HIGHSCORES-1-i));
	// Insert the new score:
	strcpy(state.highscore[i].name, name);
	strcpy(state.highscore[i].episodename, episodename);
	state.highscore[i].score=score;
	state.highscore[i].level=level;
}

void drop_catch(bool force) {
	if (state.hasCatch && (state.ball[0].type<2 || force)) { // Only drop the catch if you have less than a bubble ball.
		state.hasCatch = false;
		state.isCaught = false;
		if (state.powerups >= MAXPOWERUPS) { return; }
		state.powerup[state.powerups].x=state.padx;
		state.powerup[state.powerups].y=state.pady;
		state.powerup[state.powerups].type=PWR_CATCH;
		state.powerup[state.powerups].isCatchable=false;
		clip_powerup(state.powerups);
		state.powerups++;
	}
}

// Gets a character and returns what it should be if the user held shift down.
char shiftup(char c) {
	if (isalpha(c)) { return toupper(c); } // Easy one!

	char *intable="`1234567890-=[]\\;',./",
		*outtable="~!@#$%^&*()_+{}|:\"<>?";
	int len=strlen(intable);
	for	(int i=0; i<=len; i++) {
		if (c==intable[i]) {
			return outtable[i];
		}
	}
	return c;
}

// i is the brick index.
void redraw_brick(int i, uint32_t *framebuffer) {
	if (state.brick[i].colour == 0xff000000) { return; } // Invisible brick!

	// Figure out which colours to use.
	bool isFlashing = state.brick[i].flashtime > 0;
	uint32_t high = isFlashing ? state.brick[i].higher : state.brick[i].high;
	uint32_t colour = isFlashing ? state.brick[i].high : state.brick[i].colour;
	uint32_t low = isFlashing ? state.brick[i].colour : state.brick[i].low;

	// Fill the rect.
	int left = state.brick[i].x/100; // Inclusive.
	int top = state.brick[i].y/100;
	int width = state.brick[i].wid/100;
	int height = state.brick[i].ht/100;
	draw_rect(framebuffer, left, top, width, height, colour);

	if (state.brick[i].hasEdge) { // Draw the chamfered edges.
		draw_rect(framebuffer, left, top, width, 2, high); // Top.
		draw_rect(framebuffer, left, top, 2, height, high); // Left.
		draw_rect(framebuffer, left+2, top+height-2, width-2, 2, low); // Bottom.
		draw_rect(framebuffer, left+width-2, top+2, 2, height-2, low); // Right.
		framebuffer[(top+(height-1))*SCREENWIDTH + left+1] = low; // Corners.
		framebuffer[(top+1)*SCREENWIDTH + left+width-1] = low;
	} 
}

void redraw_level_name(uint32_t *framebuffer) {
	if (!state.levelname[0]) { return; } // Make sure the name isn't blank.
	uint32_t colour = rgb(102, 102, 102);
	int len = strlen(state.levelname) + 1; // Extra width instead of the rounded ends.
	int left = 320 - len * 4;
	int top = 52;
	int width = len * 8;
	draw_rect(framebuffer, left, top, width, 10, colour);
	// TODO rounded ends:
	// sprites.Display(314-strlen(levelname)*4,52,388,0,6,10,lpDDSGame);
	// sprites.Display(320+strlen(levelname)*4,52,394,0,6,10,lpDDSGame);
	draw_text(framebuffer, state.levelname, state.font8, left, 53);
}

// Draw the game's background, bricks, level name.
void draw_game_screen(uint32_t *framebuffer) {
	if (state.needRedraw) {
		if (state.background && state.background->data) {
			memcpy(framebuffer, state.background->data, SCREENWIDTH * SCREENHEIGHT * 4);
		}
		redraw_level_name(framebuffer);
		for	(int i=0; i<state.bricks; i++) {
			redraw_brick(i, framebuffer);
		}
		state.needRedraw = false; // It's been redrawn now.
	}
}

// TODO move these draw_* helpers to their own place.
void draw_rect(uint32_t *framebuffer, int left, int top, int width, int height, uint32_t colour) {
	for (int y0=0; y0<height; y0++) {
		int y = y0 + top;
		for (int x0=0; x0<width; x0++) {
			int x = x0 + left;
			if (0 <= x && x < SCREENWIDTH && 0 <= y && y < SCREENHEIGHT) {
				framebuffer[y*SCREENWIDTH + x] = colour;
			}
		}
	}
}

// TODO instead of the character bias, split the colour variants into different images.
void draw_text(uint32_t *framebuffer, char *text, Image* image, int x, int y) {
	// TODO.
}

// Draws an area of an image to the buffer.
void draw_subimage(uint32_t *framebuffer, Image* image, int dstLeft, int dstTop, int srcLeft, int srcTop, int width, int height) {
	for (int y0=0; y0<height; y0++) {
		int srcY = y0 + srcTop;
		int dstY = y0 + dstTop;
		if (0 <= dstY && dstY < SCREENHEIGHT && 0 <= srcY && srcY < image->height) {
			for (int x0=0; x0<width; x0++) {
				int srcX = x0 + srcLeft;
				int dstX = x0 + dstLeft;
				if (0 <= dstX && dstX < SCREENWIDTH && 0 <= srcX && srcX < image->width) {
					uint32_t c = image->data[srcY*image->width + srcX];
					if ((c >> 24) > 0x80) { // Skip transparent pixels.
						framebuffer[dstY*SCREENWIDTH + dstX] = c;
					}
				}
			}
		}
	}
}

void redraw_rect(uint32_t *framebuffer, int left, int top, int wid, int ht, int bricktoskip) {
	// Background:
	draw_subimage(framebuffer, state.background, left, top, left, top, wid, ht);

	// Redraw the level name if needed:
	if (top<61)	{
		redraw_level_name(framebuffer);
		for	(int i=0; i<state.bricks; i++) {
			if (i!=bricktoskip && state.brick[i].y<6100) {
				redraw_brick(i, framebuffer);
			}
		}
	}

	// Redraw any bricks underneath:
	int boxleft = left * 100;
	int boxright = (left + wid) * 100;
	int boxtop = top * 100;
	int boxbottom = (top + ht) * 100;
	for (int i=0; i<state.bricks; i++) {
		if (i!=bricktoskip &&
			state.brick[i].x+state.brick[i].wid >= boxleft && state.brick[i].x <= boxright &&
            state.brick[i].y+state.brick[i].ht >= boxtop && state.brick[i].y <= boxbottom) {
			redraw_brick(i, framebuffer);
		}
	}
}

// This does a black fade (sort of darkens everything).
void black_fade(uint32_t *framebuffer) {
	uint32_t black = 0xff000000;
	for (int y=0; y<SCREENHEIGHT; y++) {
		for (int x=(y&1); x<SCREENWIDTH; x+=2) {
			framebuffer[y*SCREENWIDTH + x] = black;
		}
	}
}

void draw_paddle(uint32_t *framebuffer, int x, int y, int width) {
	#define	PADEND 5
	int killedSrcXOffset = state.isKilled ? 200 : 0;
	int dstTop = y - 5;
	draw_subimage(
		framebuffer,
		state.sprites,
		(x-width/2),
		dstTop,
		killedSrcXOffset,
		12,
		width - PADEND,
		10); // Left and middle.
	draw_subimage(
		framebuffer,
		state.sprites,
		(x+width/2) - PADEND,
		dstTop,
		killedSrcXOffset + 200 - PADEND,
		12,
		PADEND,
		10); // Right end.
}

void load_proper_back() {
	image_free(state.background);
	char path[100];
	switch (state.whatScreen) {
		case SCREEN_TITLE:
			state.background = image_load("assets/img/title.png");
			break;
		case SCREEN_HIGHSCORE:
		case SCREEN_TYPENAME:
			state.background = image_load("assets/img/hiscore.png");
			break;
		case SCREEN_GAME:
			sprintf(path, "assets/img/%s.png", state.episode[state.curEpisode].backname);
			state.background = image_load(path);
			break;
		case SCREEN_ABOUT:
			state.background = image_load("assets/img/about.png");
			break;
	}
}

void menu_press_enter() {
	if (state.curmenu==MENU_MAIN)	{ // changed to ignore # of players
		if (state.curopt==0) {
			state.curmenu=MENU_GAME;
			state.menuopts=3;
			state.curopt=0;
		}
		if (state.curopt==1) {
			state.curmenu=MENU_MUSIC; state.curopt=0; state.menuopts=4;
		}
		if (state.curopt==2) {state.inMenu=0; state.whatScreen=SCREEN_HIGHSCORE; load_proper_back();} // high score screen
		if (state.curopt==3) {
			state.inMenu=0; 
			// sndIntro.Stop(); sndIntro.Play(); TODO sound
			state.whatScreen=SCREEN_ABOUT; load_proper_back();
		}
		if (state.curopt==4) state.hasQuit=true;
	} else if (state.curmenu==MENU_PLAYERS) { // how many players (not actually used).
		if (state.curopt==0) {state.players=1; }
		if (state.curopt==1) {state.players=2; }
		state.menuopts=state.episodes;
		state.curmenu=MENU_EPISODE;
		state.curopt=0;
	} else if (state.curmenu==MENU_GAME) { // game
		if (state.curopt==0) {state.curmenu=MENU_EPISODE; state.curopt=0; state.menuopts=state.episodes;} // play
		if (state.curopt==1) {state.curmenu=MENU_LOAD; state.curopt=0; state.menuopts=SAVEGAMES;}; // load
		if (state.curopt==2 && state.whatScreen==SCREEN_GAME)
			{state.curmenu=MENU_SAVE; state.curopt=0; state.menuopts=SAVEGAMES;};	// save
	} else if (state.curmenu==MENU_EPISODE) { // episode chooser
		state.curEpisode=state.curopt; state.curBalls=3; state.curLevel=1; state.mult=0; state.score=0;
		no_powerups();
		state.ball[0].type=0;
		load_cur_level();
		state.whatScreen=SCREEN_GAME;
		load_proper_back();
		state.inMenu=0; // play!
	} else if (state.curmenu==MENU_LOAD) { // load game
		// if (DoesSaveExist(curopt+1)) { // TODO loading.
		// 	LoadGame(curopt+1);
		// 	state.whatScreen=SCREEN_GAME;
		// 	load_proper_back();
		// 	state.inMenu=0; // play it!
		// }
	} else if (state.curmenu==MENU_SAVE) { // save game
		if (state.whatScreen==SCREEN_GAME) {
			// SaveGame(curopt+1); TODO save
			state.inMenu=0;
		}
	}
} // menu_press_enter
