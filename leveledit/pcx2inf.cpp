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

#define	NORMWID	20
#define	NORMHT	20
int	minwid,minht; // size of bricks

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

void	Convert(char *in,char *out)
	{
	printf("PCX2INF: %s -> %s\r\n",in,out);

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
            "; bricks {x,y,wid,ht,RRGGBB,hits(0=unmovable)}\r\n\r\n"\
            "Name = \x22%s\x22\r\n\r\n"\
            "[Balls]\r\n"\
			"{271,53,0,300,0}\r\n\r\n"\
            "[Bricks]\r\n",levelname);

    printf("Scanning:\r\n");

    long	curx,cury,wid,ht,c;
    for (cury=45;cury<480;cury+=minht+1) {
	    for (curx=20;curx<620;curx+=minwid+1) {
			c=image.data[cury*640+curx];
	        if (c) {
				fprintf (fh,"{%d,%d,%d,%d,%02x%02x%02x,%d,%d}\r\n",
				curx,cury,minwid,minht,image.palette[c*3],image.palette[c*3+1],image.palette[c*3+2],1);
	          }
	       	}
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
			  "Usage: CONVERT <file.pcx> [width] [height]\r\n\n"\
              "width and height are the size for bricks,\r\n"\
              "defaults are: width %d and height %d\r\n",NORMWID,NORMHT);
	   }
	else	{
     	minwid=NORMWID; minht=NORMHT;
        int	i;
        if (argc>=3) {
          i=atoi(args[2]);
          if (i) {minwid=i; printf ("Width: %d pixels\r\n",i);}
            }
        if (argc>=4) {
          i=atoi(args[3]);
          if (i) {minht=i; printf ("Height: %d pixels\r\n",i);}
            }

		char	lvlname[100];
		strcpy(lvlname,args[1]);
		if (!strrchr(lvlname,'.')) strcat(lvlname,".INF");
		else strcpy(strrchr(lvlname,'.'),".INF");
		Convert(args[1],lvlname);
		}
	}

