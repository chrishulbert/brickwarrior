#include <math.h>
#include <stdio.h>
#include <stdlib.h>	// random!!!
#include <time.h>

//#include "ddutil.h"
#include "dxGame.h"
#include "vkeys.h"
#include "settings.h"



void dxGame::UpdateFrame(char *keybuffer,char *keystat,long	mousex,long	mousey,int mousebutton,long	deltams)
	{
	int		i,j,k,l,ox,oy,delta,numbricks;
	long	speed,angle;

	
	framerateinc +=	(long)(1000.0/deltams);	// frame rate
	curframe++;
	if (curframe>=100) {framerate=framerateinc/100;	curframe=framerateinc=0;}
	if (inmenu)	deltams=0; // paused!
	if (deltams>50)	deltams=50;	// minimum 20 fps (if it goes over it is probably a hard drive buzz which sucks)

	if (inmenu) {
		mouseposx+=mousex;
		mouseposy+=mousey;
		MYCLAMP(mouseposx,0,639);
		MYCLAMP(mouseposy,0,479);

		if (!mousebutton && lastmouse && mouseposx>260) {
			i=(mouseposy-200) / 20;
			if (i>=0 && i<menuopts) {
				curopt=i;
				MenuPressEnter(); }
			} // if !mousebutton ...
		} // if inmenu

	// scan which keys have been pressed
	for	(i=0;i<256 && keybuffer[i];i++)	{
	// should have a bosskey here... ctrl-esc will do though
	if (Cheat()	&& keybuffer[i]==VK_F12)  {	// skiplevel CHEAT!
		curlevel++;
		LoadCurLevel();	}
	if (Cheat()	&& keybuffer[i]==VK_F8)	{ // speedupcheat
		for	(k=0;k<balls;k++) {
		speed=(long)sqrt(SQR(ball[k].xv/100)+SQR(ball[k].yv/100));
		angle=long(atan2(ball[k].yv,ball[k].xv)*100);
		speed+=50; // increase it!
		if (speed>1000)	speed=1000;
		ball[k].xv=long(cos(angle/100.0)*speed*100);
		ball[k].yv=long(sin(angle/100.0)*speed*100);
		}
		}
	if (Cheat()	&& keybuffer[i]==VK_F7)	{ // slow cheat
		for	(k=0;k<balls;k++) {
		speed=(long)sqrt(SQR(ball[k].xv/100)+SQR(ball[k].yv/100));
		angle=long(atan2(ball[k].yv,ball[k].xv)*100);
		speed-=50; // slow it!
		if (speed<0) speed=0;
		ball[k].xv=long(cos(angle/100.0)*speed*100);
		ball[k].yv=long(sin(angle/100.0)*speed*100);
		}
		}

	// f1=new game f2=save game f3=load game f4=cd menu
	if (!inmenu && keybuffer[i]==VK_F1)	{inmenu=2; curmenu=MENU_EPISODE; curopt=0;	menuopts=episodes;}	// select episode
	if (!inmenu && keybuffer[i]==VK_F2 && whatScreen==SCREEN_GAME)
		{inmenu=2; curmenu=MENU_SAVE; curopt=0;	menuopts=SAVEGAMES;}; // save
	if (!inmenu && keybuffer[i]==VK_F3)	{inmenu=2; curmenu=MENU_LOAD; curopt=0;	menuopts=SAVEGAMES;} // load
	if (!inmenu && keybuffer[i]==VK_F4)	{inmenu=2; curmenu=MENU_MUSIC; curopt=0;	menuopts=4;
			MusicCd->Read(); if	(curcdtrack>MusicCd->GetNumberOfTracks()) curcdtrack=1;} // cd player

	if (!inmenu && keybuffer[i]==VK_ESCAPE)	{curmenu=MENU_MAIN; curopt=0; menuopts=5; inmenu=1;}
	else if	(inmenu	&& keybuffer[i]==VK_ESCAPE)	{
		curopt=0;
		if (curmenu==MENU_MAIN || inmenu==2) inmenu=0;
		if (curmenu==MENU_PLAYERS) {curmenu=MENU_MAIN; menuopts=5;} // # of players
		if (curmenu==MENU_GAME)	{curmenu=MENU_MAIN; menuopts=5;} // game -> main
		if (curmenu==MENU_MUSIC) {curmenu=MENU_MAIN; curopt=1; menuopts=5;} // cd player
		if (curmenu==MENU_EPISODE)	{curmenu=MENU_GAME; menuopts=3;} // episodes -> game
		if (curmenu==MENU_LOAD)	{curmenu=MENU_GAME; curopt=1; menuopts=3;} // load -> game
		if (curmenu==MENU_SAVE)	{curmenu=MENU_GAME; curopt=2; menuopts=3;} // save -> game
		}
	if (inmenu && keybuffer[i]==VK_RETURN)
		MenuPressEnter();
		
	if (inmenu && keybuffer[i]==VK_UP) {curopt--; if (curopt<0)	curopt+=menuopts;}
	if (inmenu && keybuffer[i]==VK_DOWN) {curopt++;	if (curopt>=menuopts) curopt=0;}
	}

	if (whatScreen==SCREEN_GAME) { // in the game
	if (!inmenu) { // only move if not in menu
		// outside border 19,44,620,480 (things must be inside not on this line)
		i=padx;	padx+=mousex*100; MYCLAMP(padx,2000+padwidth/2,62000-padwidth/2);
		pdeltax=padx-i;	// how far did it move?
		i=pady;	pady+=mousey*100; MYCLAMP(pady,32000,48000);
		if (!updown) pady=42000;
		pdeltay=pady-i;	// how far up/down did it go?
		if (mousebutton	&& caught) caught=false; // let it go
		if (caught)	timesincelasthit=0;
		}
	else { // in the menu
		pdeltax=pdeltay=0;
		}

    // *** update moving bricks ***********************
	if (deltams) // dont move when paused
	for (i=0;i<bricks;i++)
        if (brick[i].moves) { // has to be a moving brick
			j=brick[i].curmovetime;
			brick[i].curmovetime+=(short)deltams;
			MYCLAMP (brick[i].curmovetime,0,brick[i].movetime[brick[i].curmove]);
			k=brick[i].curmovetime-j; // time passed
			l=brick[i].movetime[brick[i].curmove]; // total time
			// deltatime / time left * dist to go = how far to go
			// save old pos
			ox=brick[i].x/100; oy=brick[i].y/100;

			if (l-j>0) { // avoid division by zero's
				int	dist;
				dist=brick[i].curmovex - brick[i].x/100;
				brick[i].x += 100 * k * dist / (l-j);
				dist=brick[i].curmovey - brick[i].y/100;
				brick[i].y += 100 * k * dist / (l-j);
				}
			if (brick[i].curmovetime>=brick[i].movetime[brick[i].curmove]) {
				// next move
				brick[i].x=brick[i].curmovex*100;
				brick[i].y=brick[i].curmovey*100;
				brick[i].curmove++;
				if (brick[i].curmove>=brick[i].moves) brick[i].curmove=0;
				brick[i].curmovetime=0;
				k=brick[i].movedir[brick[i].curmove];
				if (k==1) // up
					brick[i].curmovey-=brick[i].movedist[brick[i].curmove];
				if (k==2) // right
					brick[i].curmovex+=brick[i].movedist[brick[i].curmove];
				if (k==3) // down
					brick[i].curmovey+=brick[i].movedist[brick[i].curmove];
				if (k==4) // left
					brick[i].curmovex-=brick[i].movedist[brick[i].curmove];
				}
			//needredraw=true;
			if (ox!=brick[i].x/100 || oy!=brick[i].y/100) { // only redraw it if it actually moved anywhere
				RedrawRect(ox,oy,(brick[i].wid+50)/100,(brick[i].ht+50)/100,i);
				RedrawBrick(i); // draw it now its moved
				}
			}

	for	(i=numbricks=0;i<bricks;i++) { // update flashing bricks and count # of bricks
		if (brick[i].flashtime)	{
		brick[i].flashtime-=deltams;
		if (brick[i].flashtime<=0) {brick[i].flashtime=0; RedrawBrick(i);}//needredraw=true;}
		}
		if (brick[i].hits) numbricks++;	// number of uninvincible bricks
		}
	bricksleft=numbricks; // count # of bricks
	if (!numbricks)	{ // finished the level!
		if (curlevel<episode[curepisode].levels) {
		// ***** adds a new ball if you have 3 when you finish
		if (balls>=3) {
			curballs++;	// if they have all 3 balls when they finish a level then they get a new ball
			if (soundinit) sndExtraBall.Play();
			}
		curlevel++;
		LoadCurLevel();	}
		else {
		// go to high scores now!
		curlevel=-1; // a win!
		score+=10*mult*mult;
		mult=0;
		if (score>=highscore[HIGHSCORES-1].score) {
			if (soundinit) sndNewHighscore.Play();
			whatScreen=SCREEN_TYPENAME;	// made it into the high scores!
			curnametext[0]=0; curnameoff=0;	// start the name off
			}
		else whatScreen=SCREEN_HIGHSCORE;
		LoadProperBack();
		}
		}

	// update powerups
	for	(i=0;i<powerups;i++) {
		powerup[i].y+=deltams*20; // (deltams/1000*20000 -> 200 pixels per sec down)
		j=0;
		if (powerup[i].y>48500)	j=1; // kill it
		if (powerup[i].catchable &&
		MYINSIDE(powerup[i].x,powerup[i].y,padx-padwidth/2-1500,pady-1000,padx+padwidth/2+1500,pady+1000)) {
		int	type;
		type=powerup[i].type;
		while (type==PWR_RANDOM) type=ChoosePowerup();
		switch (type) {	// --sounds--
			case PWR_BUBBLE:
			case PWR_BLACKBUBBLE:
			DropCatch();
			for	(k=0;k<balls;k++) {
				if (ball[k].type==0) ball[k].type=2; // green -> bubble
				//else if	(ball[k].type==1) ball[k].type=2; // yellow -> bubble
				else {// yellow, bubble or above
					if (type==PWR_BUBBLE)
						ball[k].type=3; // laser
					if (type==PWR_BLACKBUBBLE)
						ball[k].type=4; // purple
					}
				}
			if (!kill && soundinit) sndCoolBall.Play();
			break;
			case PWR_PROTECTION:
			if (!kill && soundinit)	sndProtection.Play();
			DropCatch();
			protection=1;
			break;
			case PWR_CATCH:
			if (!kill && soundinit)	sndCatch.Play();
			caught=false;
			havecatch=true;
			break;
			case PWR_UPDOWN:
			if (!kill && soundinit)	sndUpdown.Play();
			DropCatch();
			updown=1;
			break;
			case PWR_FAST:
			if (!kill && soundinit)	sndFast.Play();
			DropCatch();
			for	(k=0;k<balls;k++) {	// how bodgy can you get?
				speed=(long)sqrt(SQR(ball[k].xv/100)+SQR(ball[k].yv/100));
				angle=long(atan2(ball[k].yv,ball[k].xv)*100);
				speed+=100;	// increase it!
				if (speed>800) speed=800;
				ball[k].xv=long(cos(angle/100.0)*speed*100);
				ball[k].yv=long(sin(angle/100.0)*speed*100);
				}
			break;
			case PWR_SLOW:
			if (!kill && soundinit)	sndSlow.Play();
			DropCatch();
			for	(k=0;k<balls;k++) {
				speed=(long)sqrt(SQR(ball[k].xv/100)+SQR(ball[k].yv/100));
				angle=long(atan2(ball[k].yv,ball[k].xv)*100);
				speed-=100;	// slow it!
				if (speed<200) speed=200;
				ball[k].xv=long(cos(angle/100.0)*speed*100);
				ball[k].yv=long(sin(angle/100.0)*speed*100);
				}
			break;
			case PWR_WIDE:
			if (!kill && soundinit)	sndWide.Play();
			DropCatch();
			padwidth+=2000;	// 10 pixels each side
			if (padwidth>=12000) {
				padwidth-=2000;	// undo the expansion...
				padwidth/=2; // got too big!
				padx-=padwidth/2;
				// drop half the paddle
				if (powerups<MAXPOWERUPS) {
				powerup[powerups].x=padx+padwidth;
				powerup[powerups].y=pady;
				powerup[powerups].type=0xff;
				powerup[powerups].catchable=false;
	
				ClipPowerup(powerups);
				powerups++;
				}
				}
			break;
			case PWR_THIN:
			if (!kill && soundinit)	sndThin.Play();
			DropCatch();
			if (padwidth>6000)
				padwidth-=2000;	// 5 pixels each side
			break;

			case PWR_TRIPLE:
			DropCatch();
			l=balls;
			for	(k=0;k<l;k++) {
				int	newballtypea,newballtypeb; // the new ball types
				newballtypea=newballtypeb=ball[k].type;
				if (ball[k].type==0) newballtypea=newballtypeb=1;
				if (ball[k].type==1) newballtypea=0;
				// above code makes a yellow or green split into 2 yellows and 1 green
				if (balls<MAXBALLS)	{ // add ball B
					ball[balls].x=ball[k].x;
					ball[balls].y=ball[k].y;
					speed=(long)sqrt(SQR(ball[k].xv/100)+SQR(ball[k].yv/100));
					angle=long(atan2(ball[k].yv,ball[k].xv)*100)-50;
					ball[balls].xv=long(cos(angle/100.0)*speed*100);
					ball[balls].yv=long(sin(angle/100.0)*speed*100);
					ball[balls].active=ball[k].active;
					ball[balls].type=newballtypea;
					balls++;
					}
				if (balls<MAXBALLS)	{ // add ball B
					ball[balls].x=ball[k].x;
					ball[balls].y=ball[k].y;
					speed=(long)sqrt(SQR(ball[k].xv/100)+SQR(ball[k].yv/100));
					angle=long(atan2(ball[k].yv,ball[k].xv)*100)+50;
					ball[balls].xv=long(cos(angle/100.0)*speed*100);
					ball[balls].yv=long(sin(angle/100.0)*speed*100);
					ball[balls].active=ball[k].active;
					ball[balls].type=newballtypeb;
					balls++;
					}
				} // for k...
			if (!kill && soundinit)	{
				if (balls<=3) {sndTripleBall.Stop(); sndTripleBall.Play();}
				else {sndLotsBall.Stop(); sndLotsBall.Play();}
				}
			break;
			case PWR_KILL:
			if (soundinit) sndGetKill.Play();
			if (havecatch) DropCatch(true);	// only drop the catch! (phew) (true=force it to drop even if you have a bubble/laser)
			else kill=2; // dead paddle
			break;
			};
		if (kill==1) {kill=0; score+=500000; if	(soundinit)	sndRecover.Play();}
		if (kill==2) kill=1;
		
		j=1; // caught the powerup, so remove it!
		}
		if (j) {
		memmove(&powerup[i],&powerup[i+1],(powerups-i-1)*sizeof(_powerup));
		i--; powerups--;
		}
		}

	for	(i=0;i<balls;i++) {
		ox=ball[i].x; oy=ball[i].y;
		delta=max((abs(ball[i].xv)+abs(ball[i].yv))*deltams/300000,1);

		if (caught)	{
		if (caughtball==i) {
			ball[i].x+=pdeltax;
			ball[i].y+=pdeltay;
			if (ball[i].x<2000+BALLWID*50) ball[i].x=2000+BALLWID*50;
			if (ball[i].x>62000-BALLWID*50)	ball[i].x=62000-BALLWID*50;
			//if (ball[i].y<4500+BALLHT*50) ball[i].y=4500+BALLHT*50; // not really needed unless you have full screen movement with the updown and catch
			}
		}
		else 
		for	(l=1;l<=delta;l++) {
		ball[i].x=ox+ball[i].xv*deltams*l/delta/1000;
		ball[i].y=oy+ball[i].yv*deltams*l/delta/1000;
		//ball[i].x = ball[i].x + ball[i].xv * frametime;
		//ball[i].y = ball[i].y + ball[i].yv * frametime;

		// hit paddle (must be going down for the paddle to affect it) and the paddle must NOT be red (killed)
		if (ball[i].yv>0 &&	!kill && MYINSIDE(ball[i].x,ball[i].y,padx-padwidth/2-BALLWID*50,pady-500-BALLHT*50,padx+padwidth/2+BALLWID*50,pady+500+BALLHT*50))	{
			timesincelasthit=0;
			score+=10*mult*mult;
			mult=0;

			if (!ball[i].active) {
			ball[i].xv*=2;
			ball[i].yv*=2;
			ball[i].active=5; // it has now hit the paddle
			// the 5 means it was just caught this frame
			}
			// use isoc, so that the speed remains the same
			angle=270+90*(ball[i].x-padx)/padwidth;
			// the / 100 gets the velocities into a range that can be squared without an overflow
			speed=long(sqrt(SQR(ball[i].xv/100)+SQR(ball[i].yv/100))+.5);
			ball[i].xv=long(cos(angle/57.67)*speed*100);
			ball[i].yv=long(sin(angle/57.67)*speed*100);
			if (havecatch) {caught=true; caughtball=i;}
			l=delta; // dont move the ball any more
			}

		int	hits,oxv,oyv,ox,oy,tx,ty,bx,by,k1,k2;
		hits=0;	oxv=ball[i].xv;	oyv=ball[i].yv;
		ox=ball[i].x; oy=ball[i].y;
		if (ball[i].active==1) // true means it is active, but not caught just this frame
		for	(j=0;j<bricks;j++)
			if (MYINSIDE(ball[i].x,ball[i].y,brick[j].x-BALLWID*50,brick[j].y-BALLHT*50,
			brick[j].x+brick[j].wid+BALLWID*50,brick[j].y+brick[j].ht+BALLHT*50)) {
			k=WhichSide(ball[i].x,ball[i].y,brick[j].x,brick[j].y,
				brick[j].x+brick[j].wid,brick[j].y+brick[j].ht);
			if (hits==0) {
				tx=brick[j].x;
				ty=brick[j].y;
				bx=brick[j].x+brick[j].wid;
				by=brick[j].y+brick[j].ht;
				k1=k;
				hits++;
				}
			else	{
				tx=min(brick[j].x,tx);
				ty=min(brick[j].y,ty);
				bx=max(brick[j].x+brick[j].wid,bx);
				by=max(brick[j].y+brick[j].ht,by);
				hits++;
				if (hits==2) k2=k;
				}

			if (ball[i].type!=3) {// not a laser
				// *** bounce the ball off moving bricks x1234x
				long	movexv,moveyv;
				movexv=moveyv=0;
				
				l=-1;
				if (brick[j].moves) { // moving brick
					l=brick[j].curmove;
					// movexv/yv is in pixels per sec
					// ballxv is in 100 pixels per sec
					// movetime is in milliseconds
					if (brick[j].movetime[l]>0) { // don't do a div by 0
						if (brick[j].movedir[l]==1) // up
							moveyv=-brick[j].movedist[k] * 1000 / brick[j].movetime[l];
						if (brick[j].movedir[l]==2) // right
							movexv=brick[j].movedist[k] * 1000 / brick[j].movetime[l];
						if (brick[j].movedir[l]==3) // down
							moveyv=brick[j].movedist[k] * 1000 / brick[j].movetime[l];
						if (brick[j].movedir[l]==4) // left
							movexv=-brick[j].movedist[k] * 1000 / brick[j].movetime[l];
						} // if movetime>0
					} // if moving brick
				
				if (k==RIGHT) {ball[i].xv=ABS(ball[i].xv) + 40*movexv; ball[i].x=brick[j].x+brick[j].wid+BALLWID*50 + 2*movexv;}
				if (k==LEFT) {ball[i].xv=-ABS(ball[i].xv) + 40*movexv; ball[i].x=brick[j].x-BALLWID*50 + 2*movexv;}
				if (k==BOTTOM) {ball[i].yv=ABS(ball[i].yv) + 40*moveyv; ball[i].y=brick[j].y+brick[j].ht+BALLHT*50 + 2*moveyv;}
				if (k==TOP) {ball[i].yv=-ABS(ball[i].yv) + 40*moveyv; ball[i].y=brick[j].y-BALLHT*50 + 2*moveyv;}
				l=delta;
				} // not a laser
			if (!brick[j].flashtime	&& (brick[j].hits>0 || ball[i].type==4)) { // non-gold brick or you have a purple ball
				brick[j].hits--;
				if (brick[j].hits<=0 ||	ball[i].type>=3) { // kill brick (instant kill with superballs)
					// drop powerup!
					if (bricksleft!=1 && !(rand()%POWERUPCHANCE)) NewPowerup(brick[j].x+brick[j].wid/2,brick[j].y+brick[j].ht/2);
					if (mult<100 &&	mult+balls>=100	&& soundinit) sndMult100.Play();
					if (mult<500 &&	mult+balls>=500	&& soundinit) sndMult500.Play();
					if (mult<1000 && mult+balls>=1000 && soundinit)	sndMult1000.Play();
					if (mult<1500 && mult+balls>=1500 && soundinit)	sndMult1500.Play();
					if (mult<2000 && mult+balls>=2000 && soundinit)	sndMult2000.Play();
					mult+=balls; // bigger mult with more balls

					int	_left,_top,_wid,_ht;
					_left=brick[j].x/100; _top=brick[j].y/100; _wid=brick[j].wid/100; _ht=brick[j].ht/100;
					memmove(&brick[j],&brick[j+1],sizeof(_brick)*(bricks-j-1));
					bricks--; j--;
					RedrawRect(_left,_top,_wid,_ht);//needredraw=true;
					if (soundinit) {sndKillBlock.Stop(); sndKillBlock.Play();}
					}
				else {
					// flash colour
					brick[j].flashtime=150;	RedrawBrick(j);//needredraw=true;
					if (soundinit) {sndTinkBlock.Stop(); sndTinkBlock.Play();}
					}
				timesincelasthit=0;
				} // if hits!=0
			else // it must be a gold brick, or it is a laser
				if (soundinit && ball[i].type!=3) {sndTinkBlock.Stop();	sndTinkBlock.Play();}
			} // if myinside
			// j=0 to bricks
		if (hits==2	&& ball[i].type!=3)	{ // check the bounce, it is weird (unless laser)
			k=WhichSide(ball[i].x,ball[i].y,tx,ty,bx,by);
			if (k==k1 || k==k2)	{ // acceptable?
				if (k==RIGHT) {ball[i].xv=ABS(oxv); ball[i].yv=oyv;} //ball[i].x=bx+BALLWID*50; ball[i].y=oy;}
				if (k==LEFT) {ball[i].xv=-ABS(oxv); ball[i].yv=oyv;} //ball[i].x=tx-BALLWID*50; ball[i].y=oy;}
				if (k==BOTTOM) {ball[i].yv=ABS(oyv); ball[i].xv=oxv;}// ball[i].y=by+BALLHT*50; ball[i].x=ox;}
				if (k==TOP)	{ball[i].yv=-ABS(oyv); ball[i].xv=oxv;} //ball[i].y=ty+BALLHT*50; ball[i].x=ox;}
				}
			l=delta;
			}

		if (ball[i].active==5) ball[i].active=1; // it was just caught this frame

		// clip offscreen
		// outside border 19,44,620,480 (things must be inside, not on this line)
		if (ball[i].x<2000+BALLWID*50) {ball[i].x=2000+BALLWID*50; ball[i].xv=-ball[i].xv; l=delta;
			if (soundinit) {sndBounce.Stop(); sndBounce.Play();}}
		if (ball[i].x>62000-BALLWID*50)	{ball[i].x=62000-BALLWID*50; ball[i].xv=-ball[i].xv; l=delta;
			if (soundinit) {sndBounce.Stop(); sndBounce.Play();}}
		if (ball[i].y<4500+BALLHT*50) {ball[i].y=4500+BALLHT*50; ball[i].yv=-ball[i].yv; l=delta;
			if (soundinit) {sndBounce.Stop(); sndBounce.Play();}}

		// saved by the protection
		if (protection && ball[i].y>48000-BALLHT*50	&& (!caught	|| caughtball!=i)) {
			protection--;
			ball[i].yv=-ABS(ball[i].yv);
			ball[i].y=48000-BALLHT*50;
			if (soundinit) {sndBounce.Stop(); sndBounce.Play();}
			timesincelasthit=0;
			}

		// lost a ball
		if (ball[i].y>48000+BALLHT*50 && (!caught || caughtball!=i)) {
			memmove(&ball[i],&ball[i+1],(balls-i-1)*sizeof(_ball));
			balls--;
			i--;
			break; }
		} // for (l->delta
		} // for (i->balls

	// if it hasnt touched the paddle for 40 seconds, make the balls go laser
	j=timesincelasthit;
	timesincelasthit+=deltams;
	if (timesincelasthit>40000)	{ // 40 secs
		for	(i=0;i<balls;i++) ball[i].type=3; // laser
		timesincelasthit=0;
		if (soundinit) sndCoolBall.Play();
		}

	if (timesincelasthit>10000 &&	// after 10 seconds and...
		timesincelasthit%2000 <	j%2000)	// ...every 2 seconds
		for	(k=0;k<balls;k++) {
		speed=(long)sqrt(SQR(ball[k].xv/100)+SQR(ball[k].yv/100));
		angle=long(atan2(ball[k].yv,ball[k].xv)*100);
		angle+=rand()%100-50;
		ball[k].xv=long(cos(angle/100.0)*speed*100);
		ball[k].yv=long(sin(angle/100.0)*speed*100);
		}

	if (balls==0) {
		if (curballs>0)	{
		if (soundinit) sndLoseBall.Play();
		NoPowerups();
		NewBall(); curballs--;}	// lost a ball
		else {
		// go to high scores now!
		score+=10*mult*mult;
		mult=0;
		if (score>=highscore[HIGHSCORES-1].score) {
			if (soundinit) sndNewHighscore.Play();
			whatScreen=SCREEN_TYPENAME;	// made it into the high scores!
			curnametext[0]=0; curnameoff=0;	// start the name off
			}
		else whatScreen=SCREEN_HIGHSCORE;
		LoadProperBack();
		}
		}
	}

	else
	if (whatScreen==SCREEN_TITLE) {
		}
	else
	if (whatScreen==SCREEN_HIGHSCORE) {	// high scores
		}
	else
	if (whatScreen==SCREEN_TYPENAME) { // high scores
	if (!inmenu)
		for	(i=0;i<256 && keybuffer[i];i++)	{
		if (isprint(keybuffer[i]) && curnameoff<19)	{
			if (GetKeyState(VK_SHIFT)&0x80)
			curnametext[curnameoff]=shiftup(keybuffer[i]);
			else	curnametext[curnameoff]=tolower(keybuffer[i]);
			curnameoff++;
			curnametext[curnameoff]=0;
			}
		if (keybuffer[i]==VK_BACK && curnameoff>0) {
			curnameoff--;
			curnametext[curnameoff]=0;
			}
		if (keybuffer[i]==VK_RETURN) {
			AddHighScore(curnametext,episode[curepisode].name,score,curlevel);
			SaveHighScores();
			whatScreen=SCREEN_HIGHSCORE;
			LoadProperBack();
			}
		} // for i...
	} // whatScreen==SCREEN_typename

	lastmouse=mousebutton;
	}

void dxGame::DrawFrame()
	{
	int		i;
	char	text[200];
	DDBLTFX	ddbltfx;
	RECT	rbrick;

	ddbltfx.dwSize=sizeof(ddbltfx);

	// display background
	if (whatScreen!=SCREEN_GAME) // SCREEN_GAME draws its own background
	background.Display(0,0,0,0,640,480);

	if (whatScreen==SCREEN_GAME) { // in the game
	// draw STUFF
	DrawGameScreen();
	lpDDSBack->BltFast(NULL,NULL,lpDDSGame,NULL,DDBLTFAST_NOCOLORKEY);

	// display frame rate if control is held
	if (GetKeyState(VK_CONTROL)&0x80) {
		sprintf	(text,"%d",framerate);
		font8.Text(text,640-strlen(text)*8,472);

		i=long(sqrt(SQR(ball[0].xv/100)+SQR(ball[0].yv/100))+.5);
		sprintf(text,"%d",i);
		font8.Text(text,640-strlen(text)*8,464);
		}

	// Display stats
	sprintf	(text,"Balls:%-3dLevel:%-3dMult:%-5dScore:%d",curballs,curlevel,mult,score);
	font16.Text(text,21,24);

	if (protection>0) {
		ddbltfx.dwFillColor=MYRGB(204,153,0);
		rbrick.left=20;	rbrick.top=480-protection; rbrick.bottom=480; rbrick.right=620;
		lpDDSBack->Blt(&rbrick,NULL,NULL,DDBLT_COLORFILL,&ddbltfx);	// protection bar
		}

	// display the powerups
	for	(i=0;i<powerups;i++) {
		if (powerup[i].type!=0xff) {
		sprites.Display(powerup[i].x/100-15,powerup[i].y/100-5,
			powerup[i].type*30,22,30,10);

		/*if (powerup[i].type!=PWR_BUBBLE	&&
			powerup[i].type!=PWR_BLACKBUBBLE) {
			// DRAW A FUNKY ROTATING POWERUP
			j=(powerup[i].y/1000)%17-8;	// -8 to 8 inclusive
			if (j>=0)		
				sprites.Display(powerup[i].x/100-11,powerup[i].y/100-4+j,
					powerup[i].type*30+4,33,22,8-j);
			else
				sprites.Display(powerup[i].x/100-11,powerup[i].y/100-4,
					powerup[i].type*30+4,33-j,22,8+j);
			}*/
		}
		else	DrawPaddle(powerup[i].x/100,powerup[i].y/100,50);
		}

	// display the balls
	for	(i=0;i<balls;i++)
		sprites.Display(ball[i].x/100-6,ball[i].y/100-6,ball[i].type*12,0,12,12);

	// display the paddle
	// smallest=5000 shrinks at>=11000 has split at >=10000
	if (padwidth>=10000) {// draw with split
		int	x,y,wid;
		x=padx/100;	y=pady/100;	wid=padwidth/100;
		DrawPaddle(x-wid/4-1,y,wid/2);
		DrawPaddle(x+wid/4+1,y,wid/2+1);
		}
	else
		DrawPaddle(padx/100,pady/100,padwidth/100);
	}
	if (whatScreen==SCREEN_HIGHSCORE) {
	//font16.Text("High Scores for BrickWarrior:",320-29*4,100);

	for	(i=0;i<HIGHSCORES;i++) {
		if (highscore[i].level!=-1)
		sprintf	(text,"%2d %-20s%-15sLevel %-2d Score:%d",i+1,highscore[i].name,highscore[i].episodename,highscore[i].level,highscore[i].score);
		else
		sprintf	(text,"%2d %-20s%-15sWon!     Score:%d",i+1,highscore[i].name,highscore[i].episodename,highscore[i].score);
		font16.Text(text,0,140+i*16);
		}
	}

	if (whatScreen==SCREEN_TYPENAME) {
	font16.Text("Congrats! You're in the high scores!\nType in your name:",320-29*4,120);
	sprintf	(text,"%s%c",curnametext,GetTickCount()%1000<500?'_':' ');
	font16.Text(text,100,230);
	}

	if (whatScreen==SCREEN_ABOUT) {
	font8.Text(VERSIONNAME,0,472);
	}

	// display menu
	if (inmenu)	{
	// funky new thing...
	BlackFade(lpDDSBack);

	font16.Text("\xd",260,200+curopt*20,CLRBIAS);
	if (curmenu==MENU_MAIN)	{
		font16.Text("BrickWarrior",240,170,CLRBIAS);
		font16.Text("Game",280,200,CLRBIAS);
		font16.Text("CD player",280,220,CLRBIAS);
		font16.Text("High scores",280,240,CLRBIAS);
		font16.Text("About/Help",280,260,CLRBIAS);
		font16.Text("Quit",280,280,CLRBIAS);
		}
	if (curmenu==MENU_PLAYERS)	{
		font16.Text("How many players?",240,170,CLRBIAS);
		font16.Text("One player",280,200,CLRBIAS);
		font16.Text("Two players",280,220,CLRBIAS);
		}
	if (curmenu==MENU_GAME)	{ // game
		font16.Text("What do you want to do?",240,170,CLRBIAS);
		font16.Text("Play a new game",280,200,CLRBIAS);
		font16.Text("Load a game",280,220,CLRBIAS);
		if (whatScreen==SCREEN_GAME)
		font16.Text("Save this game",280,240,CLRBIAS);
		else 
		font16.Text("Cannot save",280,240,CLRBIAS);
		}
	if (curmenu==MENU_LOAD)	{// load game
		font16.Text("Load game",240,170,CLRBIAS);
		for	(i=0;i<SAVEGAMES;i++) {
		GetSaveTitle(i+1,text);
		font16.Text(text,280,200+i*20,CLRBIAS);
		}
		}
	if (curmenu==MENU_SAVE)	{ // save game
		font16.Text("Save game",240,170,CLRBIAS);
		for	(i=0;i<SAVEGAMES;i++) {
		GetSaveTitle(i+1,text);
		font16.Text(text,280,200+i*20,CLRBIAS);
		}
		}
	if (curmenu==MENU_EPISODE)	{
		font16.Text("Choose an episode:",240,170,CLRBIAS);
		for	(i=0;i<episodes;i++)
		font16.Text(episode[i].name,280,200+i*20,CLRBIAS);
		}
	if (curmenu==MENU_MUSIC)	{
		if (!MusicCd->GetNumberOfTracks())
		strcpy(text,"CD player - CD not inserted");
		else
		sprintf	(text,"CD player - CD has %d tracks",MusicCd->GetNumberOfTracks());
		font16.Text(text,240,170,CLRBIAS);
		
		if (!MusicCd->GetNumberOfTracks())
		strcpy(text,"Re-scan CD");
		else
		sprintf	(text,"Play track %d",curcdtrack);
		font16.Text(text,280,200,CLRBIAS);
		font16.Text("Choose next track",280,220,CLRBIAS);
		font16.Text("Choose last track",280,240,CLRBIAS);
		font16.Text("Stop CD playing",280,260,CLRBIAS);
		}
	mousecursor.Display (mouseposx,mouseposy,0,0,32,32);
	} // if inmenu
} // drawframe
