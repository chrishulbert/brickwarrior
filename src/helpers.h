// This is for random helpers.

// Make strncasecmp available on all platforms.
#ifdef _WIN32
    #include <string.h>
    #define strncasecmp _strnicmp
    #define strcasecmp _stricmp
#else
    #include <strings.h>
#endif

#define	SQR(a) (a*a)
// clamp a to the range b->c
#define	MYCLAMP(a,b,c) {if ((a)<(b)) (a)=(b); if ((a)>(c)) (a)=(c);}
// is x,y inside x1,y1->x2,y2?
#define	MYINSIDE(x,y,x1,y1,x2,y2) (x>x1	&& x<x2	&& y>y1	&& y<y2)
#define	ABS(a) ((a)>=0?(a):-(a)) // absolute
#define	AVG(a,b) ((a+b)/2) // average
#define	PCNT(a,b) (b?(a)*100/(b):100) // percent

// Which side of the square x1,y1->x2,y2 is x,y on?
typedef enum {
	SIDE_RIGHT=1,
    SIDE_BOTTOM,
    SIDE_LEFT,
    SIDE_TOP,
} Side;
Side WhichSide(long x,long y,long x1,long y1,long x2,long y2);
