#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>

typedef unsigned char byte;
struct	_image {
	byte	*data;
	int		success; // ok to be worked with?
    long	width,height;
    byte	palette[768];
	} image;

int	minwid,minht; // minimum cut-off sizes for bricks (usu. 10x10 pixels)

void	LoadImage(char *fn)
	{
    FILE	*fh;
    long	i,j,k,b,c,rep;

    image.success=0;
    if ((fh=fopen(fn,"rb"))==NULL) return;

    fread(&i,4,1,fh); if (i!=0x0801050a) {fclose(fh); return;}

    fread(&i,4,1,fh); // topleft x & y
    i=0;
    fread(&i,2,1,fh); image.width=i+1;
    fread(&i,2,1,fh); image.height=i+1;

    if (image.width>1024 || image.height>1024) {fclose(fh); return;} // too big image

    fseek(fh,128,SEEK_SET); // image
    image.data=new byte[image.width*image.height];
    if (image.data==NULL) {fclose(fh); return;}

    for (i=j=0;i<image.width*image.height && j<1048576;j++) { // decode RLE
       	b=fgetc(fh);
        if ((b&0xc0)!=0xc0) {rep=1; c=b;}
    	if ((b&0xc0)==0xc0) {rep=b&0x3f; c=fgetc(fh);}
    	for (k=0;k<rep;k++) image.data[i++]=c;
        }

    fgetc(fh); // should be 0x0c
    fread((void *)image.palette,768,1,fh); // get the palette
    image.success=1; // Done!

   	fclose(fh);
    }

void	FreeImage()
	{
    if (image.data!=NULL) {delete image.data; image.data=NULL;}
    image.width=image.height=image.success=0;
    }

void	InfoImage()
	{
    printf ("Image info: %d x %d | Load success: %c\r\n",image.width,image.height,'N'+image.success*('Y'-'N'));
    }

long	GetWidth(long x,long y,int i)
	{
    int	curx;

    for (curx=x;curx<640 && image.data[y*image.width+curx]==i;curx++);
    return curx-x;
    }

void	ClearWidth(long x,long y,long wid)
	{
     long	i,j;
     i=y*image.width+x;
     for (j=0;j<wid;j++) image.data[i+j]=0;
     }

void	GetBoxSize(long	x,long y,long *wid,long *ht)
	{
    int	curwid,curht,cury,i;
    i=image.data[y*image.width+x];

    curwid=GetWidth(x,y,i); curht=1; cury=y+1;
    while (GetWidth(x,cury,i)==curwid && cury<480) {curht++; cury++;}
    *wid=curwid; *ht=curht;
    }

void	ClearBrick(int x,int y,int wid,int ht)
	{
    int i;
    for (i=0;i<ht;i++) ClearWidth(x,y+i,wid);
    }

#define	abs(a) (a<0?-a:a)

void	AddBrick(long x,long y,long wid,long ht,FILE *fh)
	{
    int	i,edge,hits;
    long	tmpwid,tmpht,r,g,b;

    i=image.data[y*image.width+x];
    r=image.palette[i*3];
    g=image.palette[i*3+1];
    b=image.palette[i*3+2];

    hits=1;
    edge=0; // does it have an edge? (grey or gold bricks)
    GetBoxSize(x-2,y-2,&tmpwid,&tmpht);
    if (tmpwid==wid+3 && tmpht<=2  &&
        image.data[(y-2)*640+x-3]!=image.data[(y-2)*640+x-2] ) {
      GetBoxSize(x,y+ht,&tmpwid,&tmpht);
      if (tmpwid==wid+2 && tmpht==2){
	      x-=2; y-=2; wid+=4; ht+=4;
    	  edge=1; hits=3;
	      if (abs(r-194)<50 && abs(g-154)<50 && abs(b-29)<50) hits=0; // gold!
          }
      }

    // title? (doesnt work really)
    // if (y+ht<60 && abs(r-79)<10 && abs(g-79)<10 && abs(b-79)<10) return;

    if (wid>=minwid && ht>=minht) { // minimum brick size
	    fprintf (fh,"{%d,%d,%d,%d,%02x%02x%02x,%d,%d}\r\n",x,y,wid,ht,
	            r,g,b,hits,edge);

	    ClearBrick(x,y,wid,ht);
        }
    }

void	Convert(char *in,char *out)
	{
	printf("Convert: %s -> %s\r\n",in,out);

    printf("Loading:\r\n");
    LoadImage(in);
    if (!image.success) {printf("Error loading PCX!\r\n"); return;}
    if (image.width!=640 || image.height!=480) {printf("Wrong image dimensions! Has to be a 640x480 screenshot!\r\n"); return;}

    // convert!
    // 20,45 -> 619,479 (600x435)

    char	levelname[100];
    if (strrchr(in,'\\')) strcpy(levelname,strrchr(in,'\\')+1);
    else strcpy(levelname,in);
    if (strrchr(levelname,'.')) *strrchr(levelname,'.')=0;
    strlwr(levelname); levelname[0]=toupper(levelname[0]);

    FILE	*fh;
	fh=fopen(out,"wb");
    fprintf(fh,"; BrickWarrior Level file\r\n"\
            "; coords go from 0,0 to 640,480\r\n"\
            "; balls {x,y,xvel,yvel,active}\r\n"\
            "; bricks {x,y,wid,ht,RRGGBB,hits(0=unmovable),edge(0=none)}\r\n\r\n"\
            "Name = \x22%s\x22\r\n\r\n"\
            "[Balls]\r\n"\
			"{271,53,0,300,0}\r\n\r\n"\
            "[Bricks]\r\n",levelname);

    printf("Scanning:\r\n");

    // erase all the image outside the borders
    ClearBrick(0,0,20,480);
    ClearBrick(0,0,640,45);
    ClearBrick(620,0,20,480);
	//if (Init()) return;
    long	curx,cury,wid,ht;
    for (cury=45;cury<480;cury++) {
	    for (curx=20;curx<620;curx++) {
	        if (image.data[cury*640+curx]) {
	          GetBoxSize(curx,cury,&wid,&ht);
              AddBrick(curx,cury,wid,ht,fh);
	          }
			//curx+=wid-1;
	       	}
	/*B_Clear(); long x,y,off;
    for (y=off=0;y<image.height;y++) for (x=0;x<image.width;x++,off++) PutPixel(x,y,image.data[off]);
    B_Show();*/
    cprintf("\r%d%% done",(cury-45)*100/435);
       	}
	//Deinit();
    printf("\r100%% done!\r\n",(cury-45)*100/435);
	fclose(fh);

	FreeImage();
	}

int	main(int argc,char **args)
	{
	if (argc<2)	{
	   printf("Convert PCX -> INF file for BrickWarrior\r\n"\
			  "Usage: CONVERT <file.pcx> [minwidth] [minheight]\r\n\n"\
              "minwidth and minheight are the minimum size for a brick to be,\r\n"\
              "and may have to be fiddled with to get a level to convert well.\r\n"\
              "defaults are: minwidth 10 and minheight 10\r\n");
	   }
	else	{
     	minwid=10; minht=10;
        int	i;
        if (argc>=3) {
          i=atoi(args[2]);
          if (i) {minwid=i; printf ("Minimum width: %d pixels\r\n",i);}
            }
        if (argc>=4) {
          i=atoi(args[3]);
          if (i) {minht=i; printf ("Minimum height: %d pixels\r\n",i);}
            }

		char	lvlname[100];
		strcpy(lvlname,args[1]);
		if (!strrchr(lvlname,'.')) strcat(lvlname,".INF");
		else strcpy(strrchr(lvlname,'.'),".INF");
		Convert(args[1],lvlname);
		}
	}

