// TODO refactor non-game-specific stuff eg drawing into their own helpers.
// TODO tidy up unused assets.

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#include "game.h"
#include "image.h"
#include "endian.h"
#include "helpers.h"
#include "sound.h"
#include "mixer.h"

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
	PWR_LAST_VALID, // The marker for the last valid ones.
	PWR_SPECIAL_PADDLE, // A special 'powerup' that's really simply a falling half-paddle.
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
	MENU_GAME, // Play/load/save.
	MENU_LOAD, // Load game.
	MENU_SAVE, // Save game.
} Menu;

static struct {
	// Sound:
	Sound* sndMenuOpen;
	Sound* sndMenuClose;
	Sound* sndMenuMove;
	Sound* sndTinkBlock;
	Sound* sndKillBlock;
	Sound* sndBounce; // Off edge of screen.
	Sound* sndMult100;
	Sound* sndMult500;
	Sound* sndMult1000;
	Sound* sndMult1500;
	Sound* sndMult2000;
	Sound* sndExtraBall;
	Sound* sndNewHighscore;
	Sound* sndIntro;
	Sound* sndLoseBall;
	// Powerups:
	Sound* sndBubble;
	Sound* sndCoolBall;
	Sound* sndFast;
	Sound* sndSlow;
	Sound* sndThin;
	Sound* sndWide;
	Sound* sndTripleBall;
	Sound* sndLotsBall;
	Sound* sndUpdown;
	Sound* sndGetKill;
	Sound* sndRecover;
	Sound* sndCatch;
	Sound* sndProtection;

	bool hasQuit;

	uint32_t *gameFramebuffer; // This has the things that shouldn't change each frame: background, bricks, level name.
	Image *sprites;
	Image *font8;
	Image *font16;
	Image *font16blue;
	Image *background;

	// Powerups:
	Powerup powerup[MAXPOWERUPS];
	int powerups;
	bool hasUpdown; // Have they got the updown?
	bool isKilled; // Is the paddle red? Then it won't bounce balls any more but a powerup can recover it.
	bool hasProtection; // Have protection bar?
	bool hasCatch;
	bool isCaught;
	int caughtBall; // Have they got a catch?

	bool needRedraw; // Need to redraw the level? (# of bricks changed).

	// Paddle stuff:
	int padx;
	int pady;
	int padwidth;

	Ball ball[MAXBALLS];
	int balls;

	Brick brick[MAXBRICKS];
	int bricks;

	int players; // One or two players. TODO remove.
	char levelname[100];

	// Episodes:
	Episode episode[20];
	int episodes, curEpisode, curLevel,
		curBalls, // Kindof like 'lives remaining'.
		ballStartX, ballStartY, ballStartXV, ballStartYV, ballStartActive;

	// Menu:
	int	menuopts, curopt, curmenu;
	bool inMenu;
	Screen whatScreen;

	// Scoring:
	int mult, score;

	int timesincelasthit;
	float runtime; // Seconds.

	// Highscores:
	Highscore highscore[HIGHSCORES];
	char curnametext[100];
	int curnameoff;
} state;

// Forward declarations:
void add_high_score(char *name, char *episodename, int score, int level);
void new_ball();
void no_powerups();
void new_powerup(int x, int y);
void clip_powerup(int index);
void drop_catch(bool force);
PowerupType	choose_powerup();
void redraw_brick(int i, uint32_t *framebuffer);
void redraw_rect(uint32_t *framebuffer, int left, int top, int wid, int ht, int bricktoskip);
void menu_press_enter();
void load_cur_level();
void save_high_scores();
void get_save_title(int slot, char* title);
void draw_rect(uint32_t *framebuffer, int x, int y, int width, int height, uint32_t colour);
void draw_text(uint32_t *framebuffer, char *text, Image* image, int x, int y);
void load_proper_back();
void load_episodes(char *fn);
void load_high_scores();
void draw_game_screen_if_needed(uint32_t *framebuffer);
void draw_subimage(uint32_t *framebuffer, Image* image, int dstLeft, int dstTop, int srcLeft, int srcTop, int width, int height);
void draw_paddle(uint32_t *framebuffer, int x, int y, int width);
void black_fade(uint32_t *framebuffer);

// Sokol game lifecycle:

void game_init() {
	state.gameFramebuffer = malloc(SCREENWIDTH * SCREENHEIGHT * 4);

	state.sprites = image_load("assets/img/sprites.png");
	state.font8 = image_load("assets/img/font8.png");
	state.font16 = image_load("assets/img/font16.png");
	state.font16blue = image_load("assets/img/font16blue.png");

	state.whatScreen=SCREEN_TITLE;
	load_proper_back();

	state.hasQuit=false;
	state.inMenu=false;
	state.curmenu=MENU_MAIN;
	state.curopt=0; // Play!
	state.menuopts=4;

	state.runtime = 0;
	srand(time(NULL));

	// Sounds:
	state.sndMenuOpen = sound_load("assets/sound/menuopen.wav");
	state.sndMenuClose = sound_load("assets/sound/menuclose.wav");
	state.sndMenuMove = sound_load("assets/sound/menumove.wav");
	state.sndKillBlock = sound_load("assets/sound/killblok.wav");
	state.sndTinkBlock = sound_load("assets/sound/tinkblok.wav");
	state.sndBounce = sound_load("assets/sound/bounce.wav");
	state.sndExtraBall = sound_load("assets/sound/extraball.wav");
	state.sndMult100 = sound_load("assets/sound/mult100.wav");
	state.sndMult500 = sound_load("assets/sound/mult500.wav");
	state.sndMult1000 = sound_load("assets/sound/mult1000.wav");
	state.sndMult1500 = sound_load("assets/sound/mult1500.wav");
	state.sndMult2000 = sound_load("assets/sound/mult2000.wav");
	state.sndNewHighscore = sound_load("assets/sound/newhiscr.wav");
	state.sndIntro = sound_load("assets/sound/intro.wav");
	state.sndLoseBall = sound_load("assets/sound/loseball.wav");
	state.sndBubble = sound_load("assets/sound/bubble.wav");
	state.sndCoolBall = sound_load("assets/sound/coolball.wav");
	state.sndFast = sound_load("assets/sound/fast.wav");
	state.sndSlow = sound_load("assets/sound/slow.wav");
	state.sndThin = sound_load("assets/sound/thin.wav");
	state.sndWide = sound_load("assets/sound/wide.wav");
	state.sndTripleBall = sound_load("assets/sound/triple.wav");
	state.sndLotsBall = sound_load("assets/sound/lotsball.wav");
	state.sndUpdown = sound_load("assets/sound/updown.wav");
	state.sndGetKill = sound_load("assets/sound/getkill.wav");
	state.sndRecover = sound_load("assets/sound/recover.wav");
	state.sndCatch = sound_load("assets/sound/catch.wav");
	state.sndProtection = sound_load("assets/sound/protect.wav");

	load_episodes("assets/levels/episodes.inf");
	load_high_scores();
}

bool game_should_quit() {
	return state.hasQuit;
}

void game_deinit() {
	free(state.gameFramebuffer);
	image_free(state.sprites);
	image_free(state.font8);
	image_free(state.font16);
	image_free(state.font16blue);
	image_free(state.background);
	save_high_scores();
}

void game_update(float duration, int* keys, int* chars, MouseEvent* mouseEvents) {
	state.runtime += duration; // Not affected by pause because we still want cursor to blink in menu.
	if (state.inMenu) { duration=0; } // Paused!
	if (!state.inMenu && state.whatScreen==SCREEN_GAME && !is_mouse_locked()) { duration=0; } // Don't move until the mouse locks.
	if (duration > 0.05) { duration=0.05; }; // Minimum 20 fps (if it goes over it is probably something odd).
	int deltams = duration * 1000; // For compatibility with the old ms code.

	// Keys:
	for	(int i=0; keys[i]; i++) {
		switch (keys[i]) {
		case KEYCODE_F1: // New game / select episode.
			state.inMenu=true; state.curmenu=MENU_EPISODE; state.curopt=0; state.menuopts=state.episodes;
			break;
		case KEYCODE_F2: // Save.
			if (state.whatScreen==SCREEN_GAME) {
				state.inMenu=true; state.curmenu=MENU_SAVE; state.curopt=0; state.menuopts=SAVEGAMES;
			}
			break;
		case KEYCODE_F3: // Load.
			state.inMenu=true; state.curmenu=MENU_LOAD; state.curopt=0; state.menuopts=SAVEGAMES;
			break;
		case KEYCODE_F4: // Skip level cheat.
			state.curLevel++;
			load_cur_level();
			break;
		case KEYCODE_F5: // Slow cheat.
		case KEYCODE_F6: // Speedup cheat.
			for	(int k=0; k<state.balls; k++) {
				float speed = sqrt(SQR(state.ball[k].xv/100) + SQR(state.ball[k].yv/100));
				float angle = atan2(state.ball[k].yv, state.ball[k].xv);
				if (angle==0) { angle += randf(-0.5, 0.5); } // So it doesn't go sideways once stopped.
				speed += keys[i]==KEYCODE_F5 ? -50 : 50;
				if (speed<1) { speed=1; }
				state.ball[k].xv = cos(angle)*speed*100;
				state.ball[k].yv = sin(angle)*speed*100;
			}
			break;
		case KEYCODE_ESCAPE:
			if (state.inMenu) { // Go up a menu level.
				switch (state.curmenu) {
					case MENU_MAIN:
						state.inMenu = false;
						break;
					case MENU_EPISODE:
						state.curmenu = MENU_GAME;
						state.menuopts = 3;
						state.curopt = 0;
						break;
					case MENU_GAME:
						state.curmenu = MENU_MAIN;
						state.menuopts = 4;
						state.curopt = 0;
						break;
					case MENU_LOAD:
						state.curmenu = MENU_GAME;
						state.menuopts = 3;
						state.curopt = 1;
						break;
					case MENU_SAVE:
						state.curmenu = MENU_GAME;
						state.menuopts = 3;
						state.curopt = 2;
						break;
				}
				mixer_play(state.sndMenuClose, 0);
			} else { // Enter the menu.
				state.curmenu=MENU_MAIN;
				state.curopt=0;
				state.menuopts=4;
				state.inMenu=true;
				lock_mouse(false);
				mixer_play(state.sndMenuOpen, 0);
			}
			break;
		case KEYCODE_ENTER:
		case KEYCODE_KP_ENTER:
			if (state.inMenu) {
				menu_press_enter();
			}
			break;
		case KEYCODE_UP:
			if (state.inMenu) {
				state.curopt--;
				if (state.curopt < 0) { state.curopt += state.menuopts; }
				mixer_play(state.sndMenuMove, 0);
			}
			break;
		case KEYCODE_DOWN:
			if (state.inMenu) {
				state.curopt++;
				if (state.curopt >= state.menuopts) { state.curopt = 0; }
				mixer_play(state.sndMenuMove, 0);
			}
			break;
		}
	}

	if (state.whatScreen==SCREEN_GAME) {
		int pdeltax=0; // How far the paddle moved this frame.
		int pdeltay=0;
		if (!state.inMenu) { // Only move paddle if not in the menu.
			// Summarise the mouse events:
			float dx=0;
			float dy=0;
			bool didClick=false;
			for (int i=0; mouseEvents[i].isActual; i++) {
				dx += mouseEvents[i].dx * 0.15;
				dy += mouseEvents[i].dy * 0.15;
				if (mouseEvents[i].isClick) {
					didClick = true;
				}
			}
			// Only move the paddle if the mouse is locked in:
			if (is_mouse_locked()) {
				int oldpadx = state.padx;
				int oldpady = state.pady;
				state.padx += dx*100;
				state.pady += dy*100;
				if (!state.hasUpdown) { state.pady=42000; }
				MYCLAMP(state.padx, 2000+state.padwidth/2, 62000-state.padwidth/2);
				MYCLAMP(state.pady, 32000, 48000);
				pdeltax = state.padx - oldpadx; // How far did it move?
				pdeltay = state.pady - oldpady;
				if (didClick && state.isCaught) state.isCaught=false; // Let it go.
				if (state.isCaught) state.timesincelasthit=0;
			} else {
				// Request a mouse lock when they click while in game:
				if (didClick) {
					lock_mouse(true);
				}
			}
		}

		// Moving bricks:
		if (duration>0) { // Don't move when paused.
			for (int i=0; i<state.bricks; i++) {
				if (state.brick[i].moves) {
					int j=state.brick[i].curmovetime;
					state.brick[i].curmovetime += deltams;
					MYCLAMP(state.brick[i].curmovetime, 0, state.brick[i].movetime[state.brick[i].curmove]);
					int k=state.brick[i].curmovetime-j; // Time passed.
					int l=state.brick[i].movetime[state.brick[i].curmove]; // Total time.
					// deltatime / time left * dist to go = how far to go
					// Save old pos:
					int ox=state.brick[i].x/100;
					int oy=state.brick[i].y/100;

					if (l-j>0) { // Avoid div0.
						int	dist;
						dist=state.brick[i].curmovex - state.brick[i].x/100;
						state.brick[i].x += 100 * k * dist / (l-j);
						dist=state.brick[i].curmovey - state.brick[i].y/100;
						state.brick[i].y += 100 * k * dist / (l-j);
					}
					if (state.brick[i].curmovetime>=state.brick[i].movetime[state.brick[i].curmove]) {
						// next move
						state.brick[i].x=state.brick[i].curmovex*100;
						state.brick[i].y=state.brick[i].curmovey*100;
						state.brick[i].curmove++;
						if (state.brick[i].curmove>=state.brick[i].moves) state.brick[i].curmove=0;
						state.brick[i].curmovetime=0;
						k=state.brick[i].movedir[state.brick[i].curmove];
						if (k==1) // up
							state.brick[i].curmovey-=state.brick[i].movedist[state.brick[i].curmove];
						if (k==2) // right
							state.brick[i].curmovex+=state.brick[i].movedist[state.brick[i].curmove];
						if (k==3) // down
							state.brick[i].curmovey+=state.brick[i].movedist[state.brick[i].curmove];
						if (k==4) // left
							state.brick[i].curmovex-=state.brick[i].movedist[state.brick[i].curmove];
					}
					if (ox!=state.brick[i].x/100 || oy!=state.brick[i].y/100) { // Only redraw it if it actually moved.
						redraw_rect(state.gameFramebuffer, ox-1, oy-1, state.brick[i].wid/100+2, state.brick[i].ht/100+2, i);
						redraw_brick(i, state.gameFramebuffer); // Draw it now it's moved.
					}
				}
			}
		}

		// Update flashing bricks and count # of bricks:
		int numbricks=0; // Number of destroyable bricks remaining.
		for	(int i=0; i<state.bricks; i++) { 
			if (state.brick[i].flashtime)	{
				state.brick[i].flashtime -= deltams;
				if (state.brick[i].flashtime <= 0) {
					state.brick[i].flashtime = 0;
					redraw_brick(i, state.gameFramebuffer);
				}
			}
			if (state.brick[i].hits) {
				numbricks++;
			}
		}

		// Finished the level?
		if (!numbricks)	{
			if (state.curLevel < state.episode[state.curEpisode].levels) {
				// Progress to the next level.
				if (state.balls>=3) {
					state.curBalls++; // If they have all 3 balls when they finish a level then they get a new ball
					mixer_play(state.sndExtraBall, 0);
				}
				state.curLevel++;
				load_cur_level();
			} else { // Finished the episode!
				state.curLevel = -1; // a win!
				state.score += 10*state.mult*state.mult;
				state.mult=0;
				if (state.score > state.highscore[HIGHSCORES-1].score) {
					// Made it into the high scores!
					mixer_play(state.sndNewHighscore, 0);
					state.whatScreen = SCREEN_TYPENAME;	
					state.curnametext[0]=0; state.curnameoff=0;	// Start the name off.
				} else { // No new high score.
					state.whatScreen = SCREEN_HIGHSCORE;
				}
				load_proper_back();
			}
		}
		
		// Powerups:
		for	(int i=0; i<state.powerups; i++) {
			// Slide down:
			state.powerup[i].y += deltams*20; // (deltams/1000*20000 -> 200 pixels per sec down).
			// Fell too far?
			bool shouldRemovePowerup = state.powerup[i].y > 48500;
			// Is it caught?
			if (state.powerup[i].isCatchable &&
				MYINSIDE(state.powerup[i].x, state.powerup[i].y, state.padx-state.padwidth/2-1500, state.pady-1000, state.padx+state.padwidth/2+1500, state.pady+1000)) {
				// Give it an actual non-kill type if it's a random:
				PowerupType	type = state.powerup[i].type;
				if (type==PWR_RANDOM) {
					while (type==PWR_RANDOM || type==PWR_KILL) {
						type = choose_powerup();
					}
				}
				bool powerupDidKill = false;
				switch (type) {
					case PWR_BUBBLE:
					case PWR_BLACKBUBBLE:
						drop_catch(false);
						for	(int k=0; k<state.balls; k++) {
							if (state.ball[k].type==0) {
								state.ball[k].type = 2; // Green -> bubble.
							} else { // Yellow, bubble or above.
								if (type==PWR_BUBBLE) {
									state.ball[k].type=3; // Laser.
								} else if (type==PWR_BLACKBUBBLE) {
									state.ball[k].type=4; // Purple.
								}
							}
						}
						if (!state.isKilled) mixer_play(state.sndCoolBall, 0);
						break;
					case PWR_PROTECTION:
						if (!state.isKilled) mixer_play(state.sndProtection, 0);
						drop_catch(false);
						state.hasProtection=true;
						break;
					case PWR_CATCH:
						if (!state.isKilled) mixer_play(state.sndCatch, 0);
						state.hasCatch=true;
						state.isCaught=false;
						break;
					case PWR_UPDOWN:
						if (!state.isKilled) mixer_play(state.sndUpdown, 0);
						drop_catch(false);
						state.hasUpdown=1;
						break;
					case PWR_FAST:
						if (!state.isKilled) mixer_play(state.sndFast, 0);
						drop_catch(false);
						for	(int k=0; k<state.balls; k++) {	// how bodgy can you get?
							float speed = sqrt(SQR(state.ball[k].xv/100) + SQR(state.ball[k].yv/100));
							float angle = atan2(state.ball[k].yv, state.ball[k].xv);
							speed+=100;	// increase it!
							if (speed>800) speed=800;
							state.ball[k].xv = cos(angle)*speed*100;
							state.ball[k].yv = sin(angle)*speed*100;
						}
						break;
					case PWR_SLOW:
						if (!state.isKilled) mixer_play(state.sndSlow, 0);
						drop_catch(false);
						for	(int k=0; k<state.balls; k++) {
							float speed = sqrt(SQR(state.ball[k].xv/100) + SQR(state.ball[k].yv/100));
							float angle = atan2(state.ball[k].yv, state.ball[k].xv);
							speed-=100;	// slow it!
							if (speed<200) speed=200;
							state.ball[k].xv = cos(angle)*speed*100;
							state.ball[k].yv = sin(angle)*speed*100;
						}
						break;
					case PWR_WIDE:
						if (!state.isKilled) mixer_play(state.sndWide, 0);
						drop_catch(false);
						state.padwidth+=2000;	// 10 pixels each side
						if (state.padwidth>=12000) { // Drop half the paddle if it gets too big.
							state.padwidth-=2000; // undo the expansion...
							state.padwidth/=2; // ...and drop half!
							state.padx -= state.padwidth/2;
							// Drop half the paddle as a faux-powerup.
							if (state.powerups<MAXPOWERUPS) {
								state.powerup[state.powerups].x = state.padx + state.padwidth;
								state.powerup[state.powerups].y = state.pady;
								state.powerup[state.powerups].type = PWR_SPECIAL_PADDLE;
								state.powerup[state.powerups].isCatchable = false;
								clip_powerup(state.powerups);
								state.powerups++;
							}
						}
						break;
					case PWR_THIN:
						if (!state.isKilled) mixer_play(state.sndThin, 0);
						drop_catch(false);
						if (state.padwidth>6000) {
							state.padwidth-=2000;
						}
						break;

					case PWR_TRIPLE:
						drop_catch(false);
						int ballCountBeforeSplitting = state.balls; // So it doesn't try splitting the new balls.
						for	(int k=0; k<ballCountBeforeSplitting; k++) {
							// Make a yellow or green split into 2 yellows and 1 green:
							int newballtypea=state.ball[k].type;
							int newballtypeb=state.ball[k].type;
							if (state.ball[k].type==0) newballtypea=newballtypeb=1;
							if (state.ball[k].type==1) newballtypea=0;
							// Add ball A:
							if (state.balls<MAXBALLS) {
								state.ball[state.balls].x=state.ball[k].x;
								state.ball[state.balls].y=state.ball[k].y;
								float speed = sqrt(SQR(state.ball[k].xv/100) + SQR(state.ball[k].yv/100));
								float angle = atan2(state.ball[k].yv, state.ball[k].xv) - 0.5;
								state.ball[state.balls].xv=cos(angle)*speed*100;
								state.ball[state.balls].yv=sin(angle)*speed*100;
								state.ball[state.balls].active=state.ball[k].active;
								state.ball[state.balls].type=newballtypea;
								state.balls++;
							}
							// Add ball B:
							if (state.balls<MAXBALLS)	{
								state.ball[state.balls].x=state.ball[k].x;
								state.ball[state.balls].y=state.ball[k].y;
								float speed = sqrt(SQR(state.ball[k].xv/100) + SQR(state.ball[k].yv/100));
								float angle = atan2(state.ball[k].yv, state.ball[k].xv) + 0.5;
								state.ball[state.balls].xv=cos(angle)*speed*100;
								state.ball[state.balls].yv=sin(angle)*speed*100;
								state.ball[state.balls].active=state.ball[k].active;
								state.ball[state.balls].type=newballtypeb;
								state.balls++;
							}
						} // for k...
						if (!state.isKilled) {
							if (state.balls<=3) {
								mixer_play(state.sndTripleBall, 0);
							} else {
								mixer_play(state.sndLotsBall, 0);
							}
						}
						break;
					case PWR_KILL:
						mixer_play(state.sndGetKill, 0);
						if (state.hasCatch) {
							drop_catch(true); // Only drop the catch! (phew) (true=force it to drop even if you have a bubble/laser)
						} else {
							powerupDidKill = true;
						}
						break;
					case PWR_RANDOM:
					case PWR_LAST_VALID:
					case PWR_SPECIAL_PADDLE:
						break; // These aren't normal powerups, and the type can't be random by this point.
				}
				//  Unless they caught a 'kill'.
				if (powerupDidKill) {
					state.isKilled = true;
				} else {
					if (state.isKilled) {
						// Recovered a dead/red paddle by catching a non-killing powerup.
						state.isKilled = false;
						mixer_play(state.sndRecover, 0);
						state.score += 500000;
					}
				}
				shouldRemovePowerup = true; // Caught the powerup, so remove it!
			}
			if (shouldRemovePowerup) {
				memmove(&state.powerup[i], &state.powerup[i+1], (state.powerups-i-1)*sizeof(Powerup));
				state.powerups--;
				i--;
			}
		}

		// Balls:
		for	(int i=0; i<state.balls; i++) {
			int ox = state.ball[i].x;
			int oy = state.ball[i].y;
			int delta = maxi((abs(state.ball[i].xv) + abs(state.ball[i].yv)) * deltams / 300000, 1);

			if (state.isCaught)	{
				if (state.caughtBall==i) {
					state.ball[i].x += pdeltax;
					state.ball[i].y += pdeltay;
					if (state.ball[i].x<2000+BALLWID*50) state.ball[i].x=2000+BALLWID*50;
					if (state.ball[i].x>62000-BALLWID*50) state.ball[i].x=62000-BALLWID*50;
				}
			} else {
				for (int l=1; l<=delta; l++) { // Iterate a few times so the ball won't go straight through the paddle due to low fps.
					state.ball[i].x=ox+state.ball[i].xv*deltams*l/delta/1000;
					state.ball[i].y=oy+state.ball[i].yv*deltams*l/delta/1000;

					// Hit paddle (must be going down for the paddle to affect it) and the paddle must NOT be red/killed.
					if (state.ball[i].yv>0 &&
						!state.isKilled &&
						MYINSIDE(state.ball[i].x, state.ball[i].y,
							state.padx-state.padwidth/2-BALLWID*50, state.pady-500-BALLHT*50,
							state.padx+state.padwidth/2+BALLWID*50, state.pady+500+BALLHT*50)) {
						state.timesincelasthit=0;
						state.score += 10 * state.mult * state.mult;
						state.mult=0;

						if (!state.ball[i].active) {
							state.ball[i].xv*=2;
							state.ball[i].yv*=2;
							state.ball[i].active=5; // it has now hit the paddle
							// the 5 means it was just caught this frame
						}
						// Use isosceles, so that the speed remains the same:
						float angle = 270 + 90*(state.ball[i].x - state.padx)/state.padwidth;
						// The / 100 gets the velocities into a range that can be squared without an overflow:
						float speed = sqrt(SQR(state.ball[i].xv/100) + SQR(state.ball[i].yv/100)) + .5;
						state.ball[i].xv = cos(angle/57.67)*speed*100;
						state.ball[i].yv = sin(angle/57.67)*speed*100;
						if (state.hasCatch) {state.isCaught=true; state.caughtBall=i;}
						l=delta; // Don't move the ball any more.
					}

					int	tx,ty,bx,by,k1,k2;
					int hits=0;
					int oxv=state.ball[i].xv;
					int oyv=state.ball[i].yv;
					int ox=state.ball[i].x;
					int oy=state.ball[i].y;
					if (state.ball[i].active==1) { // true means it is active, but not caught just this frame
						for	(int j=0; j<state.bricks; j++) {
							if (MYINSIDE(state.ball[i].x, state.ball[i].y,
									state.brick[j].x-BALLWID*50, state.brick[j].y-BALLHT*50,
									state.brick[j].x+state.brick[j].wid+BALLWID*50, state.brick[j].y+state.brick[j].ht+BALLHT*50)) {
								int k = which_side(state.ball[i].x, state.ball[i].y,
									state.brick[j].x, state.brick[j].y,
									state.brick[j].x + state.brick[j].wid, state.brick[j].y + state.brick[j].ht);
								if (hits==0) {
									tx=state.brick[j].x;
									ty=state.brick[j].y;
									bx=state.brick[j].x+state.brick[j].wid;
									by=state.brick[j].y+state.brick[j].ht;
									k1=k;
									hits++;
								} else {
									tx = mini(state.brick[j].x, tx);
									ty = mini(state.brick[j].y, ty);
									bx = maxi(state.brick[j].x+state.brick[j].wid, bx);
									by = maxi(state.brick[j].y+state.brick[j].ht, by);
									hits++;
									if (hits==2) k2=k;
								}

								if (state.ball[i].type!=3) { // Not a laser.
									// Bounce the ball off moving bricks:
									int movexv=0;
									int moveyv=0;
									l=-1;
									if (state.brick[j].moves) { // moving brick
										l=state.brick[j].curmove;
										// movexv/yv is in pixels per sec
										// ballxv is in 100 pixels per sec
										// movetime is in milliseconds
										if (state.brick[j].movetime[l]>0) { // don't do a div by 0
											if (state.brick[j].movedir[l]==1) // up
												moveyv=-state.brick[j].movedist[k] * 1000 / state.brick[j].movetime[l];
											if (state.brick[j].movedir[l]==2) // right
												movexv=state.brick[j].movedist[k] * 1000 / state.brick[j].movetime[l];
											if (state.brick[j].movedir[l]==3) // down
												moveyv=state.brick[j].movedist[k] * 1000 / state.brick[j].movetime[l];
											if (state.brick[j].movedir[l]==4) // left
												movexv=-state.brick[j].movedist[k] * 1000 / state.brick[j].movetime[l];
										} // if movetime>0
									} // if moving brick
									
									if (k==SIDE_RIGHT) {state.ball[i].xv=ABS(state.ball[i].xv) + 40*movexv; state.ball[i].x=state.brick[j].x+state.brick[j].wid+BALLWID*50 + 2*movexv;}
									if (k==SIDE_LEFT) {state.ball[i].xv=-ABS(state.ball[i].xv) + 40*movexv; state.ball[i].x=state.brick[j].x-BALLWID*50 + 2*movexv;}
									if (k==SIDE_BOTTOM) {state.ball[i].yv=ABS(state.ball[i].yv) + 40*moveyv; state.ball[i].y=state.brick[j].y+state.brick[j].ht+BALLHT*50 + 2*moveyv;}
									if (k==SIDE_TOP) {state.ball[i].yv=-ABS(state.ball[i].yv) + 40*moveyv; state.ball[i].y=state.brick[j].y-BALLHT*50 + 2*moveyv;}
									l=delta;
								} // not a laser

								if (!state.brick[j].flashtime	&& (state.brick[j].hits>0 || state.ball[i].type==4)) { // non-gold brick or you have a purple ball
									state.brick[j].hits--;
									if (state.brick[j].hits<=0 ||	state.ball[i].type>=3) { // kill brick (instant kill with superballs)
										// drop powerup!
										if (numbricks!=1 && !(rand()%POWERUPCHANCE)) new_powerup(state.brick[j].x+state.brick[j].wid/2,state.brick[j].y+state.brick[j].ht/2);
										if (state.mult<100  && state.mult+state.balls>=100)  mixer_play(state.sndMult100, 0);
										if (state.mult<500  && state.mult+state.balls>=500)  mixer_play(state.sndMult500, 0);
										if (state.mult<1000 && state.mult+state.balls>=1000) mixer_play(state.sndMult1000, 0);
										if (state.mult<1500 && state.mult+state.balls>=1500) mixer_play(state.sndMult1500, 0);
										if (state.mult<2000 && state.mult+state.balls>=2000) mixer_play(state.sndMult2000, 0);
										state.mult += state.balls; // bigger mult with more balls

										int	_left,_top,_wid,_ht;
										_left=state.brick[j].x/100; _top=state.brick[j].y/100; _wid=state.brick[j].wid/100; _ht=state.brick[j].ht/100;
										memmove(&state.brick[j], &state.brick[j+1], sizeof(Brick)*(state.bricks-j-1));
										state.bricks--; j--;
										redraw_rect(state.gameFramebuffer, _left, _top, _wid, _ht, -1);
										mixer_play(state.sndKillBlock, 0);
									} else {
										// Flash colour:
										state.brick[j].flashtime=150;
										redraw_brick(j, state.gameFramebuffer);
										mixer_play(state.sndTinkBlock, 0);
									}
									state.timesincelasthit=0;
								} else { // It must be a gold brick, or it is a laser
									if (state.ball[i].type != 3) { mixer_play(state.sndTinkBlock, 0); }
								}
							} // if myinside
						} // for (int j=0; j<state.bricks; j++)
					} // if (state.ball[i].active==1)

					// Check if the bounce is weird (unless laser):
					if (hits==2	&& state.ball[i].type!=3)	{
						Side side = which_side(state.ball[i].x, state.ball[i].y, tx, ty, bx, by);
						if (side==k1 || side==k2) { // Acceptable?
							if (side==SIDE_RIGHT)  {state.ball[i].xv= ABS(oxv); state.ball[i].yv=oyv;}
							if (side==SIDE_LEFT)   {state.ball[i].xv=-ABS(oxv); state.ball[i].yv=oyv;}
							if (side==SIDE_BOTTOM) {state.ball[i].yv= ABS(oyv); state.ball[i].xv=oxv;}
							if (side==SIDE_TOP)    {state.ball[i].yv=-ABS(oyv); state.ball[i].xv=oxv;}
						}
						l=delta;
					}

					if (state.ball[i].active==5) {
						state.ball[i].active=1; // it was just caught this frame
					}

					// Bounce off the screen border 19,44,620,480 (things must be inside, not on this line).
					if (state.ball[i].x<2000+BALLWID*50) {
						state.ball[i].x=2000+BALLWID*50; state.ball[i].xv=-state.ball[i].xv; l=delta;
						mixer_play(state.sndBounce, 0);
					}
					if (state.ball[i].x>62000-BALLWID*50)	{
						state.ball[i].x=62000-BALLWID*50; state.ball[i].xv=-state.ball[i].xv; l=delta;
						mixer_play(state.sndBounce, 0);
					}
					if (state.ball[i].y<4500+BALLHT*50) {
						state.ball[i].y=4500+BALLHT*50; state.ball[i].yv=-state.ball[i].yv; l=delta;
						mixer_play(state.sndBounce, 0);
					}

					// Saved by the protection:
					if (state.hasProtection && state.ball[i].y>48000-BALLHT*50	&& (!state.isCaught || state.caughtBall!=i)) {
						state.hasProtection = false;
						state.ball[i].yv=-ABS(state.ball[i].yv);
						state.ball[i].y=48000-BALLHT*50;
						state.timesincelasthit=0;
						mixer_play(state.sndBounce, 0);
					}

					// Lost a ball:
					if (state.ball[i].y>48000+BALLHT*50 && (!state.isCaught || state.caughtBall!=i)) {
						memmove(&state.ball[i], &state.ball[i+1], (state.balls-i-1)*sizeof(Ball));
						state.balls--;
						i--;
						break;
					}
				} // for (int l=1; l<=delta; l++)
			} // else after if (state.isCaught)
		} // for (int i=0; i<state.balls; i++)

		// If it hasnt touched the paddle for 30 seconds, make the balls go laser so they definitely cannot get stuck.
		int lasttimesincelasthit = state.timesincelasthit;
		state.timesincelasthit += deltams;
		if (state.timesincelasthit>30000) { // 30 secs
			for	(int i=0; i<state.balls; i++) {
				state.ball[i].type = 3; // Laser.
			}
			state.timesincelasthit = 0;
			mixer_play(state.sndCoolBall, 0);
		}

		// After 10 seconds and every 2 seconds, reaim the balls so they hopefully get unstuck.
		if (state.timesincelasthit>10000 && state.timesincelasthit%2000 < lasttimesincelasthit%2000) {
			for	(int k=0; k<state.balls; k++) {
				float speed = sqrt(SQR(state.ball[k].xv/100) + SQR(state.ball[k].yv/100));
				float angle = atan2(state.ball[k].yv, state.ball[k].xv);
				angle += randf(-0.5, 0.5);
				state.ball[k].xv=cos(angle)*speed*100;
				state.ball[k].yv=sin(angle)*speed*100;
			}
		} // if (timesincelasthit>10000 && timesincelasthit%2000 < j%2000)

		// Have they run out of balls?
		if (state.balls==0) {
			if (state.curBalls>0)	{
				no_powerups();
				new_ball();
				state.curBalls--; // Lost a ball/life.
				mixer_play(state.sndLoseBall, 0);
			} else {
				// Game over, go to high scores now!
				state.score += 10 * state.mult * state.mult;
				state.mult=0;
				if (state.score >= state.highscore[HIGHSCORES-1].score) {
					mixer_play(state.sndNewHighscore, 0);
					state.whatScreen = SCREEN_TYPENAME;	// Made it into the high scores!
					state.curnametext[0]=0; state.curnameoff=0;	// Start the name off.
				} else {
					state.whatScreen = SCREEN_HIGHSCORE;
				}
				load_proper_back();
			}
		} // if (balls==0)
		// end if (state.whatScreen==SCREEN_GAME)
	} else if (state.whatScreen==SCREEN_TITLE) {
		// Deliberately empty.
	} else if (state.whatScreen==SCREEN_HIGHSCORE) {
		// Deliberately empty.
	} else if (state.whatScreen==SCREEN_TYPENAME) {
		if (!state.inMenu) {
			// Typed characters:
			for (int i=i; chars[i]; i++) {
				if (isprint(chars[i]) && state.curnameoff<19)	{
					state.curnametext[state.curnameoff] = chars[i];
					state.curnameoff++;
					state.curnametext[state.curnameoff]=0;
				}
			} // chars
			// Special keys:
			for (int i=0; keys[i]; i++) {
				switch (keys[i]) {
					case KEYCODE_BACKSPACE:
					case KEYCODE_DELETE:
						if (state.curnameoff > 0) {
							state.curnameoff--;
							state.curnametext[state.curnameoff]=0;
						}
						break;
					case KEYCODE_ENTER:
					case KEYCODE_KP_ENTER:
						add_high_score(state.curnametext, state.episode[state.curEpisode].name, state.score, state.curLevel);
						save_high_scores();
						state.whatScreen=SCREEN_HIGHSCORE;
						load_proper_back();
						break;
					default:
						break;
				}
			} // keys
		} // if (!inMenu)
	} // whatScreen==SCREEN_typename
}

void game_draw(uint32_t* framebuffer) {
	char	text[200];

	if (state.whatScreen != SCREEN_GAME) { // Non-game screens need their background drawn for them.
		if (state.background && state.background->data) {
			memcpy(framebuffer, state.background->data, SCREENWIDTH * SCREENHEIGHT * 4);
		}
	}

	if (state.whatScreen == SCREEN_GAME) {
		// Draw the level only when necessary to the gameFramebuffer.
		// Then, each frame, quickly copy the not-often-changing parts of the screen, then overlay eg balls/paddle/etc.
		draw_game_screen_if_needed(state.gameFramebuffer);
		memcpy(framebuffer, state.gameFramebuffer, SCREENWIDTH * SCREENHEIGHT * 4);

		// Score:
		sprintf(text, "Balls:%-3dLevel:%-3dMult:%-5dScore:%d", state.curBalls, state.curLevel, state.mult, state.score);
		draw_text(framebuffer, text, state.font16, 21, 24);

		// Protection bar:
		if (state.hasProtection) {
			uint32_t colour = rgb(204, 153, 0);
			draw_rect(framebuffer, 20, 480-1, 600, 1, colour);
		}

		// Powerups:
		for	(int i=0; i<state.powerups; i++) {
			if (state.powerup[i].type != PWR_SPECIAL_PADDLE) {
				draw_subimage(
					framebuffer, state.sprites,
					state.powerup[i].x/100-15, state.powerup[i].y/100-5,
					state.powerup[i].type*30, 22,
					30,10);
			} else {
				draw_paddle(framebuffer, state.powerup[i].x/100, state.powerup[i].y/100, 50);
			}
		}

		// Balls:
		for	(int i=0; i<state.balls; i++) {
			draw_subimage(
				framebuffer, state.sprites,
				state.ball[i].x/100-6, state.ball[i].y/100-6,
				state.ball[i].type*12, 0,
				12, 12);
		}

		// Paddle:
		// Smallest=5000, shrinks at>=11000, has split at >=10000.
		if (state.padwidth >= 10000) { // Draw with a crack, hinting that if it grows more it'll split in half.
			int x=state.padx/100;
			int y=state.pady/100;
			int wid=state.padwidth/100;
			draw_paddle(framebuffer, x-wid/4-1, y, wid/2);
			draw_paddle(framebuffer, x+wid/4+1, y, wid/2+1);
		} else {
			draw_paddle(framebuffer, state.padx/100, state.pady/100, state.padwidth/100);
		}

		// Mouse click reminder:
		if (!state.inMenu && !is_mouse_locked()) {
			draw_text(framebuffer, "Click here!", state.font16, 320-11*10/2, 430);
		}
	} else if (state.whatScreen==SCREEN_HIGHSCORE) {
		for	(int i=0; i<HIGHSCORES; i++) {
			if (state.highscore[i].level != -1) {
				sprintf(text, "%2d %-20s%-15sLevel %-2d Score:%d", i+1, state.highscore[i].name, state.highscore[i].episodename, state.highscore[i].level, state.highscore[i].score);
			} else {
				sprintf(text, "%2d %-20s%-15sWon!     Score:%d", i+1, state.highscore[i].name, state.highscore[i].episodename, state.highscore[i].score);
			}
			draw_text(framebuffer, text, state.font16, 0, 140+i*16);
		}
	} else if (state.whatScreen==SCREEN_TYPENAME) {
		draw_text(framebuffer, "Congrats! You're in the high scores!\nType in your name:", state.font16, 320-29*4, 120);
		bool isBlink = (((int)(state.runtime * 2.0)) & 0x1) != 0;
		sprintf(text, "%s%c", state.curnametext, isBlink ? '_' : ' ');
		draw_text(framebuffer, text, state.font16, 100, 230);
	} else if (state.whatScreen==SCREEN_ABOUT) {
		draw_text(framebuffer, VERSIONNAME, state.font8, 0, 472);
	}

	// Menu:
	if (state.inMenu) {
		black_fade(framebuffer);

		draw_text(framebuffer, ">", state.font16blue, 260, 200+state.curopt*20);
		switch (state.curmenu) {
		case MENU_MAIN:
			draw_text(framebuffer, "BrickWarrior", state.font16blue, 240, 170);
			draw_text(framebuffer, "Game", state.font16blue, 280, 200);
			draw_text(framebuffer, "High scores", state.font16blue, 280, 220);
			draw_text(framebuffer, "About/Help", state.font16blue, 280, 240);
			draw_text(framebuffer, "Quit", state.font16blue, 280, 260);
			break;
		case MENU_PLAYERS:
			draw_text(framebuffer, "How many players?", state.font16blue, 240, 170);
			draw_text(framebuffer, "One player", state.font16blue, 280, 200);
			draw_text(framebuffer, "Two players", state.font16blue, 280, 220);
			break;
		case MENU_GAME:
			draw_text(framebuffer, "What do you want to do?", state.font16blue, 240, 170);
			draw_text(framebuffer, "Play a new game", state.font16blue, 280, 200);
			draw_text(framebuffer, "Load a game", state.font16blue, 280, 220);
			if (state.whatScreen==SCREEN_GAME) {
				draw_text(framebuffer, "Save this game", state.font16blue, 280, 240);
			} else {
				draw_text(framebuffer, "Cannot save", state.font16blue, 280, 240);
			}
			break;
		case MENU_LOAD:
			draw_text(framebuffer, "Load game", state.font16blue, 240, 170);
			for	(int i=0; i<SAVEGAMES; i++) {
				get_save_title(i+1, text);
				draw_text(framebuffer, text, state.font16blue, 280, 200+i*20);
			}
			break;
		case MENU_SAVE:
			draw_text(framebuffer, "Save game", state.font16blue, 240, 170);
			for	(int i=0; i<SAVEGAMES; i++) {
				get_save_title(i+1, text);
				draw_text(framebuffer, text, state.font16blue, 280, 200+i*20);
			}
			break;
		case MENU_EPISODE:
			draw_text(framebuffer, "Choose an episode:", state.font16blue, 240, 170);
			for	(int i=0; i<state.episodes; i++) {
				draw_text(framebuffer, state.episode[i].name, state.font16blue, 280, 200+i*20);
			}
			break;
		}
	} // if inMenu
}

void set_pause(bool on) {
	if (on && !state.inMenu) {
		state.curmenu = MENU_MAIN;
		state.curopt = 0;
		state.menuopts = 4;
		state.inMenu = 1;
	}
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
	if (fileId!=LVLFILEID && fileId!=LV2FILEID)	{ fclose(fh); printf("LVL: Invalid file id (%x).\n", fileId); return false; }
	int32_t numberOfLevels = fread_i32(fh);
	if (level>numberOfLevels || level<=0) { fclose(fh); return false; } // Bad level #.

	fseek(fh, (level-1)*4, SEEK_CUR); // Skip to the data for the offset for this level.
	int32_t levelDataOffset = fread_i32(fh); // Get the offset to the level data.
	fseek(fh, levelDataOffset, SEEK_SET); // Jump to the level.

	// Load level name:
	fread(state.levelname, 100, 1, fh);

	// Load balls:
	int ballCount = fread_i16(fh);
	if (ballCount > MAXBALLS) { fclose(fh); printf("LVL: Too many balls (%d).\n", ballCount); return false; }
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
	int brickCount = fread_i16(fh);
	if (brickCount > MAXBRICKS) { fclose(fh); printf("LVL: Too many bricks (%d).\n", brickCount); return false; }
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
		if (moves > MAXMOVES) { fclose(fh); printf("LVL: Too many moves (%d).\n", moves); return false; }
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
					state.brick[state.bricks].higher=rgb(r+86, g+86, b+86);
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

PowerupType	choose_powerup() {
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
		PowerupType t = rand() % PWR_LAST_VALID;
		bool keepit = true;
		if (state.ball[0].type<2) { // All powerups are cool if you've got a bubble or above
			if (t==PWR_TRIPLE && (numbubbles ||	numcatches)) keepit=false;
			if ((t==PWR_BUBBLE || t==PWR_BLACKBUBBLE || t==PWR_CATCH) && (numtriples ||	state.balls>1)) keepit=false;
		}
		if (keepit) {
			return t;
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
void no_powerups() {
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
void new_ball() {
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
	char infPath[100];
	sprintf(infPath, "assets/levels/%s%d.inf", state.episode[state.curEpisode].basename, state.curLevel);
	if (load_level_inf(infPath)) { return; } // INF worked :)

	char lvlPath[100];
	sprintf(lvlPath, "assets/levels/%s.lvl", state.episode[state.curEpisode].basename);
	bool lvlSucceeded = load_level_lvl(lvlPath, state.curLevel);

	if (!lvlSucceeded) {
		printf("Could not load either INF (%s) or LVL (%s).\n", infPath, lvlPath);
	}
}

void load_high_scores() {
	// Fill in some default names:
	#define	NAMES 18
	char* name[NAMES] = {
		"Nice", "Smokey", "Reaper",
		"Jazz", "Ben", "Psycho",
		"Jumper", "Master", "Freak",
		"Insane", "The Man", "One",
		"FatMan", "Bubble", "BrickWarrior",
		"Dinosaur", "PaddleMeister", "Ripper",
	};
	for	(int i=0; i<HIGHSCORES; i++) {
		strcpy(state.highscore[i].name, name[rand() % NAMES]);
		strcpy(state.highscore[i].episodename, state.episode[rand() % state.episodes].name);
		state.highscore[i].score = 100-i;
		state.highscore[i].level = 1;
	}

	auto home = getenv("HOME");
    if (!home) { return; }

	char path[512];
	snprintf(path, sizeof(path), "%s/.config/brickwarrior/highscores", home);

    auto file = fopen(path, "rb");
    if (!file) { return; }

	void *buf = malloc(sizeof(state.highscore));
	auto count = fread(buf, sizeof(state.highscore), 1, file);
	if (count==1) { // Read successfully.
		memcpy(&state.highscore, buf, sizeof(state.highscore));
	}
	
	fclose(file);
}

void save_high_scores() {
	auto home = getenv("HOME");
    if (!home) { return; }

	char path[512];
	snprintf(path, sizeof(path), "%s/.config", home);
	mkdir(path, 0700);

    snprintf(path, sizeof(path), "%s/.config/brickwarrior", home);
	mkdir(path, 0700);

	snprintf(path, sizeof(path), "%s/.config/brickwarrior/highscores", home);
    auto file = fopen(path, "wb");
    if (!file) { return; }

	fwrite(state.highscore, sizeof(state.highscore), 1, file);

	fclose(file);
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

// Only drop the catch if you have less than a bubble ball.
void drop_catch(bool force) {
	if (state.hasCatch && (state.ball[0].type<2 || force)) {
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

// i is the brick index.
void redraw_brick(int i, uint32_t *framebuffer) {
	if (state.brick[i].colour == 0xff000000) { return; } // Invisible brick!

	// Figure out which colours to use.
	bool isFlashing = state.brick[i].flashtime > 0;
	uint32_t high = isFlashing ? state.brick[i].higher : state.brick[i].high;
	uint32_t colour = isFlashing ? state.brick[i].high : state.brick[i].colour;
	uint32_t low = isFlashing ? state.brick[i].colour : state.brick[i].low;

	// Fill the rect.
	int left = state.brick[i].x/100;
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
	int len = strlen(state.levelname); // Extra width instead of the rounded ends.
	int left = 320 - len * 4;
	int top = 52;
	int width = len * 8;
	draw_rect(framebuffer, left, top, width, 10, colour);
	// Rounded ends:
	draw_subimage(framebuffer, state.sprites, left-6,     52, 388, 0, 6, 10);
	draw_subimage(framebuffer, state.sprites, left+width, 52, 394, 0, 6, 10);
	draw_text(framebuffer, state.levelname, state.font8, left, 53);
}

// Draw the game's background, bricks, level name.
void draw_game_screen_if_needed(uint32_t *framebuffer) {
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

void draw_text(uint32_t *framebuffer, char *text, Image* image, int x, int y) {
	if (!image) { return; }
	int charWidth = image->width / 16;
	int charHeight = image->height / 6;
	int curX = x;
	int curY = y;
	int len = strlen(text);
	for (int i=0; i<len; i++) {
		char c = text[i];
		if (c == '\n') {
			curY += charHeight;
			curX = x;
		} else {
			if (32 <= c && c <= 127) { // Only printable chars.
				int index = c - 32;
				draw_subimage(
					framebuffer, image,
					curX, curY,
					(index % 16) * charWidth,
					(index / 16) * charHeight,
					charWidth, charHeight);
			}
			curX += charWidth;
		}
	}
}

// Redraws the background, level name, and bricks - intended for use for the gameFramebuffer.
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
		if (y%3) {
			memset(framebuffer+y*SCREENWIDTH, 0, SCREENWIDTH * 4); // All black.
		} else {
			for (int x=0; x<SCREENWIDTH; x++) {
				if (x%3) {
					framebuffer[y*SCREENWIDTH + x] = black;
				}
			}
		}
	}
}

// x/y is the center of the paddle.
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
	if (state.curmenu==MENU_MAIN) { // changed to ignore # of players
		if (state.curopt==0) {
			state.curmenu=MENU_GAME;
			state.menuopts=3;
			state.curopt=0;
		} else if (state.curopt==1) {
			state.inMenu=false; state.whatScreen=SCREEN_HIGHSCORE; load_proper_back();
		} else if (state.curopt==2) {
			state.inMenu=false; 
			mixer_play(state.sndIntro, 0);
			state.whatScreen=SCREEN_ABOUT; load_proper_back();
		} else if (state.curopt==3) {
			state.hasQuit=true;
		}
	} else if (state.curmenu==MENU_PLAYERS) { // how many players (not actually used). TODO remove
		if (state.curopt==0) { state.players=1; }
		if (state.curopt==1) { state.players=2; }
		state.menuopts=state.episodes;
		state.curmenu=MENU_EPISODE;
		state.curopt=0;
	} else if (state.curmenu==MENU_GAME) { // game
		if (state.curopt==0) { // Play.
			state.curmenu=MENU_EPISODE; state.curopt=0; state.menuopts=state.episodes;
		} else if (state.curopt==1) { // Load.
			state.curmenu=MENU_LOAD; state.curopt=0; state.menuopts=SAVEGAMES;
		} else if (state.curopt==2 && state.whatScreen==SCREEN_GAME) { // Save.
			state.curmenu=MENU_SAVE; state.curopt=0; state.menuopts=SAVEGAMES;
		}
	} else if (state.curmenu==MENU_EPISODE) { // episode chooser
		state.curEpisode=state.curopt; state.curBalls=3; state.curLevel=1; state.mult=0; state.score=0;
		no_powerups();
		state.ball[0].type=0;
		load_cur_level();
		state.whatScreen=SCREEN_GAME;
		load_proper_back();
		state.inMenu=false; // play!
	} else if (state.curmenu==MENU_LOAD) { // load game
		// if (DoesSaveExist(curopt+1)) { // TODO loading.
		// 	LoadGame(curopt+1);
		// 	state.whatScreen=SCREEN_GAME;
		// 	load_proper_back();
		// 	state.inMenu=false; // play it!
		// }
	} else if (state.curmenu==MENU_SAVE) {
		if (state.whatScreen==SCREEN_GAME) {
			// SaveGame(curopt+1); TODO save
			state.inMenu=false;
		}
	}
	mixer_play(state.sndMenuOpen, 0);
} // menu_press_enter

void get_save_title(int slot, char* title) {
	// TODO
	title[0]='T';
	title[1]='O';
	title[2]='D';
	title[3]='O';
	title[4]=0;
}
