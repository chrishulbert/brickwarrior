#ifndef __SETTINGS_H
#define __SETTINGS_H

#define	WIDTH	640
#define	HEIGHT	480
#define	BITS	8

// RGB accepts 0-255

#if	BITS==24 // 24 bit version
#define	MYRGB(r,g,b) RGB(b,g,r)
#endif

#if BITS==16 // 16 bit version (565)
#define	MYRGB(r,g,b) (((r/8)<<11)+((g/4)<<5)+(b/8))
#endif

#if BITS==8 // 8 bit version
#define	MYRGB(r,g,b) (b/43*36+g/43*6+r/43)
#endif

#endif