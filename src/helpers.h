// This is for random helpers.
// The intention is for these helpers to be roughly game-independent.

#include <stdint.h>

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
Side which_side(long x,long y,long x1,long y1,long x2,long y2);

uint32_t rgb(int r, int g, int b);

// GLFW-compatible keycodes as per Sokol.
typedef enum {
    KEYCODE_F1        = 290,
    KEYCODE_F2        = 291,
    KEYCODE_F3        = 292,
    KEYCODE_F4        = 293,
    KEYCODE_F5        = 294,
    KEYCODE_F6        = 295,
    KEYCODE_F7        = 296,
    KEYCODE_F8        = 297,
    KEYCODE_F9        = 298,
    KEYCODE_F10       = 299,
    KEYCODE_F11       = 300,
    KEYCODE_F12       = 301,
    KEYCODE_ESCAPE    = 256,
    KEYCODE_ENTER     = 257,
    KEYCODE_KP_ENTER  = 335,
    KEYCODE_UP        = 265,
    KEYCODE_DOWN      = 264,
    KEYCODE_LEFT      = 263,
    KEYCODE_RIGHT     = 262,
    KEYCODE_BACKSPACE = 259,
    KEYCODE_DELETE    = 261,
} Keycode;
