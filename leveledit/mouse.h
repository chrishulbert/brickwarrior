#ifndef __MOUSE_H
#define __MOUSE_H

extern int	mgetx();
extern int	mgety();
extern int	mbutton(int b);
extern void	msetpos(int posx,int posy);
extern void msetborders(unsigned b_left,unsigned b_top,unsigned b_right,unsigned b_bottom);
extern int	mreset();
extern int	mouse_buttons;	/* amount of buttons on the mouse */

#endif