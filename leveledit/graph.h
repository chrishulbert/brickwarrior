// for DJGPP.

#ifndef __GRAPH_H
#define __GRAPH_H

// 0x13=320x200  0x100=640x400  0x101=640x480
// 0x103=800x600 0x105=1024x768 0x107=1024x1024

typedef unsigned char byte;
extern void	SetSinglePal(int c,int r,int g,int b);
extern void	B_Clear();
extern void	B_Show();
extern int	Init();
extern void	Deinit();
extern void	PutPixel(int x,int y,byte colour);
extern void	Line(int _x1,int _y1,int _x2,int _y2,byte colour);
extern void	Rectangle(int _x1,int _y1,int _x2,int _y2,byte colour);
extern void	Box(int x1,int y1,int x2,int y2,byte colour);
extern void	ScreenShot();
extern void	GTextColour(int fore,int back);
extern void	GText(char *format, ...);
extern void	GTextGoto(int x,int y);
extern void	GTextGetpos(int *x,int *y);

#define	RGB(r,g,b) (255 - b/43*36 - g/43*6 - r/43)
#define	MYWHITE (40)

#endif
