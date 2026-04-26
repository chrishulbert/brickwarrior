// dxGame.h: interface for the dxGame class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DXGAME_H__2EA8B581_06C8_11D2_95DD_0000E8D1D170__INCLUDED_)
#define AFX_DXGAME_H__2EA8B581_06C8_11D2_95DD_0000E8D1D170__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <ddraw.h>
#include "dxSurface.h"
#include "CDXMusicCd.h"
#include "CDXSound.h"
#include "CDXSoundBuffer.h"

#define	HIGHSCORES	20
#define	SAVEGAMES	6
#define	MAXEPISODES	20 // fair few
#define	MAXBRICKS	1000 // a lot!!!
#define	MAXPOWERUPS	10 // powerups on screen
#define	MAXBALLS	9
#define	MAXMOVES	10
class dxGame  
{
	protected:
		void MenuPressEnter();
		int Cheat(); // true/false are cheats available?
		void DeleteSave(int slot);
		void BlackFade(LPDIRECTDRAWSURFACE surf);
		int DoesSaveExist(int slot);
		void GetSaveTitle(int slot,char *text);
		void LoadGame(int slot);
		void SaveGame(int slot);
		void RedrawRect(int left,int top,int wid,int ht,int bricktoskip=-1);
		void RedrawLevelname();
		void RedrawBrick(int brick);
		void DrawPaddle(int x,int y,int wid);
		void DrawGameScreen();
		void LoadProperBack();
		void ClipPowerup(int thep);
		void DropCatch(int force=false);
		void LoadCurLevel();
		void NewBall();
		void NoPowerups();
		void DegradeBalls();
		int ChoosePowerup();
		void NewPowerup(int x,int y);
		void LoadEpisodes(char *fn);
		int  LoadLevelLvl(char *filename,int level);
		int LoadLevelInf(char *filename);
		LPDIRECTDRAW		lpDD;		// DirectDraw object
		LPDIRECTDRAWSURFACE	lpDDSPrimary,	// DirectDraw primary surface
					lpDDSBack,	// DirectDraw back surface
					lpDDSGame; // the game screen (bricks,background,etc)

		// sound stuff
		CDXMusicCd	*MusicCd;
		CDXSound	*Sound;
		CDXSoundBuffer	sndKillBlock,
				sndTinkBlock,
				sndBounce, // off edge of screen
				sndMult100,
				sndMult500,
				sndMult1000,
				sndMult1500,
				sndMult2000,
				sndExtraBall,
				sndNewHighscore,
				sndIntro,
				sndLoseBall,
				// powerups below
				sndBubble,sndCoolBall,
				sndFast,sndSlow,
				sndThin,sndWide,
				sndTripleBall,sndLotsBall,
				sndUpdown,
				sndGetKill,sndRecover,
				sndCatch,
				sndProtection;

		int	soundinit;

		int		quit;
		dxSurface	sprites,mousecursor,font8,font16,background;

		// powerups
		long	updown, // have they got the updown?
				kill, // is the paddle red?
				protection, // have protection bar?
				havecatch,caught,caughtball; // have they got a catch?

		enum {PWR_BUBBLE=0,PWR_FAST,PWR_SLOW,PWR_THIN,PWR_WIDE,
			PWR_TRIPLE,PWR_UPDOWN,PWR_KILL,PWR_CATCH,
			PWR_PROTECTION,PWR_BLACKBUBBLE,PWR_RANDOM,PWR_LAST};
		struct	_powerup { // falling powerups
			long	type,x,y,catchable;
			} powerup[MAXPOWERUPS];
		int	powerups;

		int	needredraw; // need to redraw the level? (# of bricks changed)

		// paddle stuff
		long	padx,pady,padwidth,
			pdeltax,pdeltay; // how far the paddle moved this frame

		struct	_ball {
			long	x,y,xv,yv,active,type;
			} ball[MAXBALLS];
		long	balls;
		struct	_brick {
			long	x,y,wid,ht,hits,flashtime;
			long	clr, // what index (for 8 bit mode)
				edge,high,low, // do they have an edge? what colour?
				red,green,blue; // what colour they are
			char	moves,curmove,
					movedir[MAXMOVES]; // 1=up 2=right 3=down 4=left
			short	curmovetime,curmovex,curmovey, // destination x,y
					movedist[MAXMOVES],
					movetime[MAXMOVES];
			} brick[MAXBRICKS];
		long	bricks,players; // one or two players
		char	levelname[100];
		long	bricksleft;

		// episode stuff
		struct	{
			char	name[100],basename[20],backname[30];
			int		levels;
			} episode[20];
		long	episodes,curepisode,curlevel,curballs,
			ballstartx,ballstarty,ballstartxv,ballstartyv,ballstartactive;

		// menusystem stuff
		int	menuopts,curopt,curmenu,inmenu,menusel, // inmenu true -> paused!
			whatin,mouseposx,mouseposy,lastmouse; // 0=game 1=highscores 2=titlescreen 3=aboutscreen

		// cd player
		int	curcdtrack;

		// scoring stuff
		long	mult,score;

		// framerate
		long	curframe,framerateinc,framerate,timesincelasthit;

		// highscores
		struct	_highscore {
			char	name[100],episodename[100];
			int		score,level;
			} highscore[HIGHSCORES];
		char	curnametext[100];
		int		curnameoff;
		void	LoadHighScores();
		void	AddHighScore(char *name,char *episodename,int score,int level);
		void	SaveHighScores();
		int	shiftup(int c);

	public:
		void SetDX(LPDIRECTDRAW _lpDD,
			LPDIRECTDRAWSURFACE _prim,
			LPDIRECTDRAWSURFACE _back) {lpDD=_lpDD; lpDDSPrimary=_prim; lpDDSBack=_back;}
		virtual void SetPause(int on);
		virtual int HasQuit();
		virtual void DrawFrame();
		virtual void UpdateFrame(char *keybuffer,char *keystat,long mousex,long mousey,int mousebutton,long deltams);
		virtual void DeInit();
		virtual int Init(HWND hWnd);
		dxGame();
		virtual ~dxGame();

};

#endif // !defined(AFX_DXGAME_H__2EA8B581_06C8_11D2_95DD_0000E8D1D170__INCLUDED_)
