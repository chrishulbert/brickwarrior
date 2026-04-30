#include <stdlib.h>

#include "helpers.h"

Side which_side(long x,long y,long x1,long y1,long x2,long y2) {
	// 1 = right, 2=bottom, 3=left, 4=top
	int	dx,dy;
	dx = PCNT(x	- AVG(x1,x2),x2-x1);
	dy = PCNT(y	- AVG(y1,y2),y2-y1);
	if (ABS(dx)>ABS(dy)) { // left or right
        if (dx>0) return SIDE_RIGHT;
        return SIDE_LEFT;
	} else { // top/bottom
        if (dy>0) return SIDE_BOTTOM;
        return SIDE_TOP;
	}
}

// Converts 0-255 rgb into an 0xAABBGGRR colour.
// Clamps it for you.
uint32_t rgb(int r, int g, int b) {
	#define MAX(a, b) ((a) > (b) ? (a) : (b))
	#define MIN(a, b) ((a) < (b) ? (a) : (b))
	#define CLAMP(a) (MAX(MIN(a, 255), 0))
	return 0xff000000 +
		(((uint32_t)CLAMP(b)) << 16) +
		(((uint32_t)CLAMP(g)) << 8) +
		((uint32_t)CLAMP(r));
}

int maxi(int a, int b) {
    return a > b ? a : b;
}

int mini(int a, int b) {
    return a < b ? a : b;
}

float randf(float min, float max) {
	return min + ((float)rand()) / ((float)RAND_MAX) * (max - min);
}