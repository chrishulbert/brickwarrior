// for DJGPP.

#include <sys/segments.h>
#include <stdarg.h>
#include <dpmi.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>

// 0x13=320x200  0x100=640x400  0x101=640x480
// 0x103=800x600 0x105=1024x768 0x107=1024x1024

typedef	unsigned char	byte;
typedef	unsigned long	ulong;

byte	*drawbuf;
byte	pal[768];
long	xres,yres,totalsize;

void	Set_Mode(unsigned mode)	{
	union REGS r;
	if (mode<0x100)	r.x.ax=mode;
	else	{
	  	r.x.ax=0x4f02;
		r.x.bx=mode; }
	int86(0x10,	&r,	&r); }

void	Graphics() {
	Set_Mode(0x101);
    xres=640; yres=480; totalsize=307200; }

void	Text() {
	Set_Mode(3); }

void	B_Clear() {
	memset(drawbuf,0,totalsize); }

void	PutPixel(int x,int y,byte colour) {
	drawbuf[y*xres+x]=colour; }

#define	CLIP() {\
	if (x+wid<0 || y+ht<0 || x>=640 || y>=480) return;\
	if (x<0) {wid+=x; x=0;}\
    if (y<0) {ht+=y; y=0;} \
    if (x+wid>640) wid=640-x;\
    if (y+ht>480) ht=480-y;}

void	Rectangle(int x,int y,int wid,int ht,byte colour) {
	int	i;
    long	off;

    CLIP();
    off=y*xres+x;
    for (i=0;i<ht;i++) {
       memset(drawbuf+off,colour,wid);
       off+=xres; }
	}

#define ULONG (unsigned long)
void	Line(int x1,int y1,int x2,int y2,byte colour) {
	 int dx,dy,long_d,short_d;
	 int d,add_dh,add_dl;
	 int inc_xh,inc_yh,inc_xl,inc_yl;
	 register int inc_ah,inc_al;
	 register int i;
	 byte *adr;

	 adr=drawbuf;

	 dx=x2-x1; dy=y2-y1;                          /* ranges */

	 if(dx<0){dx=-dx; inc_xh=-1; inc_xl=-1;}    /* making sure dx and dy >0 */
	 else    {        inc_xh=1;  inc_xl=1; }    /* adjusting increments */
	 if(dy<0){dy=-dy; inc_yh=-xres;
			  inc_yl=-xres;}/* to get to the neighboring */
	 else    {        inc_yh= xres; /* point along Y have to add */
			  inc_yl= xres;}/* or subtract this */

	 if(dx>dy){long_d=dx; short_d=dy; inc_yl=0;}/* long range,&making sure either */
	 else     {long_d=dy; short_d=dx; inc_xl=0;}/* x or y is changed in L case */

	 inc_ah=inc_xh+inc_yh;
	 inc_al=inc_xl+inc_yl;                      /* increments for point adress */
	 adr+=y1*xres+x1;                 /* address of first point */

	 d=2*short_d-long_d;                        /* initial value of d */
	 add_dl=2*short_d;                          /* d adjustment for H case */
	 add_dh=2*short_d-2*long_d;                 /* d adjustment for L case */

	 for(i=0;i<=long_d;i++)                     /* for all points in longer range */
	 {
      //if ((ULONG((long)adr-(long)drawbuf))<totalsize) // clip top & bottom
		  *adr=colour;                              /* rendering */

	  if(d>=0){adr+=inc_ah; d+=add_dh;}         /* previous point was H type */
	  else    {adr+=inc_al; d+=add_dl;}         /* previous point was L type */
	 }
    }
void	Box(int x,int y,int wid,int ht,byte colour) {
	CLIP();
	Line(x,      y,     x+wid-1,y,     colour);
	Line(x+wid-1,y,     x+wid-1,y+ht-1,colour);
	Line(x,      y+ht-1,x+wid-1,y+ht-1,colour);
	Line(x,      y,     x,      y+ht-1,colour);
	}

void	Sync() {
	do ; while(!(inportb(0x3da)&8)); }

void	B_Changebank(int bank) {
	union	REGS	r;
	r.x.ax=0x4f05;
	r.x.bx=0;
	r.x.dx=bank;
	int86(0x10,&r,&r);
	}

void	B_Show() {
	Sync();
	  	int	curbank;
		long	curoff;
		curbank=curoff=0;
		for	(;curoff<totalsize;curoff+=65536) {
			B_Changebank(curbank);
			movedata(_my_ds(),(ulong)drawbuf+curoff,__dpmi_segment_to_descriptor(0xa000),0,65536);
			curbank++;
		   	}
	}

void	SetSinglePal(int c,int red,int g,int b)
	{
	union	REGS	r;
	r.x.ax=0x1010;
	r.x.bx=c;
	r.h.ch=g; r.h.cl=b; r.h.dh=red;
	int86(0x10,&r,&r);
    }

void	Set_Palette(byte *palette)	{
	union	REGS	r;
	r.x.ax=0x1012;
	r.x.bx=0;
	r.x.cx=256;
	r.x.dx=(ulong)palette;
	int86(0x10,&r,&r);
	}

int	Init() {
	Graphics();

	if ((drawbuf=(byte *)malloc(totalsize))==NULL) {
		printf ("Not enough image memory!\n");
		return 1; }

	// generate the palette
	int	r,g,b,c;
    for (c=255,b=0;b<6;b++) for (g=0;g<6;g++) for (r=0;r<6;r++,c--)
        {pal[c*3]=r*63/5; pal[c*3+1]=g*63/5; pal[c*3+2]=b*63/5;}

    pal[7*3]=50;pal[7*3+1]=50;pal[7*3+2]=50;

	Set_Palette((byte *)pal);
	return 0; }

void	Deinit() {
	free(drawbuf);
	Text();	}

void	ScreenShot() {
    FILE	*fh;
    long	i,j,k,l;
    byte	thepal[768];

    for (i=0;i<768;i++) thepal[i]=4*pal[i];

    fh=fopen("scrnshot.pcx","wb");
    if (fh==NULL) return;
    i=0x0801050a; fwrite(&i,4,1,fh); // maker,version,compress,bitsperpixel
    i=0; fwrite(&i,4,1,fh); // topleft 0,0 (as 2 shorts)
    i=xres-1; j=yres-1; fwrite(&i,2,1,fh); fwrite(&j,2,1,fh); // res
    i=300; fwrite(&i,2,1,fh); fwrite(&i,2,1,fh); // dpi (any value)
    fwrite(thepal,16,3,fh); // first 16 colours
    i=0x0101; fwrite(&i,2,1,fh); // vid mode & num of planes
    i=xres; fwrite(&i,2,1,fh); // bytes per line
    while (ftell(fh)<128) fputc(0,fh); // 0's

    for (i=0;i<totalsize;i++) { // image
       if ((drawbuf[i]&0xc0)==0xc0) fputc(0xc1,fh);
       fputc(drawbuf[i],fh);
       }

    fputc(0xc,fh); fwrite(thepal,256,3,fh); // palette

    fclose(fh);
	}

// text stuff!!!
long	textdata[]={
#include "font.h"
	};

#define	TEXTCOLOUR 40
int		curtextx,curtexty,forecolour=TEXTCOLOUR,backcolour=0;
void	OutChar(int x,int y,char c)
	{
    long	bits;

    int	cx,cy;
    for (cy=0;cy<8;cy++) {
        bits=*((byte *)textdata+c*8+cy);
	    for (cx=0;cx<8;cx++) {
           if (bits & 1)
	           PutPixel(x+cx,y+cy,forecolour); // the text colour
           else PutPixel(x+cx,y+cy,backcolour); // black
           bits/=2;
	       }
       	}
	}
void	GTextColour(int fore,int back)
		{
        if (fore==-1) forecolour=TEXTCOLOUR;
		else forecolour=fore;
        if (back==-1) backcolour=0;
		else backcolour=back;
        }
void	GTextGoto(int x,int y) {curtextx=x; curtexty=y;}
void	GTextGetpos(int *x,int *y) {*x=curtextx; *y=curtexty;}

void	OutText(int x,int y,char *text)
	{
	int	curchar,c,curx,cury;

	curx=x; cury=y;
	for (curchar=0;curchar<10000 && text[curchar];curchar++)	{
		c=text[curchar];
		if (c=='\n') {
			cury+=8;
			curx=0; }
        else
        if (c=='\r') {
          curx=0;
          }
		else if (c<128)	{
			OutChar(curx,cury,c);
			curx+=8;
			}
		}
     curtextx=curx;
     curtexty=cury;
     }


void	GText(char *format, ...)
	{
    char	text[10000];
	va_list arg;

	va_start(arg, format);
	vsprintf(text, format, arg);
	va_end(arg);

    OutText(curtextx,curtexty,text);
	}

