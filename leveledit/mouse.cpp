#include <dos.h>
int	mouse_buttons=0;

int mgetx()
 {
 unsigned int	mp;

 if (!mouse_buttons) return(0);

	union	REGS	r;
	r.x.ax=3;
	int86(0x33,&r,&r);
    mp=r.x.cx/2;

 return(mp);
 }

int mgety()
 {
 unsigned mp;

 if (!mouse_buttons) return(0);
	union	REGS	r;
	r.x.ax=3;
	int86(0x33,&r,&r);
    mp=r.x.dx;
 return (mp);
 }

int mbutton(int b)
 {
 unsigned b_flag;

 if (!mouse_buttons) return(0);

	union	REGS	r;
	r.x.ax=3;
	int86(0x33,&r,&r);
    b_flag=r.x.bx;

 return((b_flag >> (b-1)) & 1);	/* 1 if button #'b' = down */
 }

void msetpos(int posx,int posy)
 {
 if (!mouse_buttons) return;
 	union	REGS	r;
	r.x.ax=4;
    r.x.cx=posx*2;
    r.x.dx=posy;
	int86(0x33,&r,&r);
 }

int mreset()
 {
 unsigned a_flag,b_flag;

	union	REGS	r;
	r.x.ax=0;
	int86(0x33,&r,&r);
    b_flag=r.x.bx;
    a_flag=r.x.ax;

 if (a_flag==0xffff) mouse_buttons=b_flag;	/* how many buttons */
 else    mouse_buttons=0;			/* no mouse installed */

 return (mouse_buttons);
 }

void msetborders(unsigned b_left,unsigned b_top,unsigned b_right,unsigned b_bottom)
 {
 if (!mouse_buttons) return;
 union REGS r;
 r.x.ax=7;
 r.x.cx=b_left*2;
 r.x.dx=b_right*2;
 int86(0x33,&r,&r);

 r.x.ax=8;
 r.x.cx=b_top;
 r.x.dx=b_bottom;
int86(0x33,&r,&r);
 }
