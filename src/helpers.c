#include "helpers.h"

Side WhichSide(long x,long y,long x1,long y1,long x2,long y2) {
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
