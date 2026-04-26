// dxSurface.cpp: implementation of the dxSurface class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>

#include "dxSurface.h"

/*void	DebugOut(char *text)
	{
	FILE	*fh;
	fh=fopen("debug.txt","ab");
	if (fh!=NULL) {
		fprintf(fh,"%s\r\n",text);
		fclose(fh);
		}
	}*/

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

	dxSurface::dxSurface()
	{
	colourkey=false;
	loadedon=false;

	lpDDSurf=NULL;
	lpDD=NULL;
	lpDDSBack=NULL;
	}

	dxSurface::dxSurface(LPDIRECTDRAW _lpdd)
	{
	lpDDSurf=NULL;
	lpDD=_lpdd;
	lpDDSBack=NULL;
	}

	dxSurface::~dxSurface() {
	UnLoad();
	}

void dxSurface::Load(char *name)
	{
	HRESULT			ddrval;
    DDSURFACEDESC	ddsd;
    BITMAP			bm;
	HBITMAP			hbm;

	strcpy(bitmapname,name);

    // get bitmap info
    hbm = OpenBitmap();
	if (!hbm) return; // error!
	GetObject(hbm, sizeof(bm), &bm);
	width = bm.bmWidth; height = bm.bmHeight;
	textwid = width/16; textht = height/16;
	DeleteObject(hbm);

	//*** fprintf debug width height

	// create the surface
    ddsd.dwSize = sizeof( ddsd );
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;
	if (!loadedon) {
		ddrval = lpDD->CreateSurface( &ddsd, &lpDDSurf, NULL );
		if (ddrval != DD_OK) {
			if (lpDDSurf != NULL) {lpDDSurf->Release(); lpDDSurf=NULL;}
			return; // error!
			}
		}
	loadedon=true;
	Restore(); // load it in
	}

void dxSurface::Restore()
// restore the surface memory and load the bitmaps in
	{
    HDC		hdcImage,hdc;
    HRESULT	hr;
    HBITMAP hbm;

	lpDDSurf->Restore(); // restore surface memory, not its contents though

    // Load our bitmap resource.
    hbm = OpenBitmap();
    if (!hbm) return; // error!

    //  select bitmap into a memoryDC so we can use it.
    hdcImage = CreateCompatibleDC(NULL);
    if (!hdcImage) return;
    SelectObject(hdcImage, hbm);

    if ((hr = lpDDSurf->GetDC(&hdc)) == DD_OK) {
		StretchBlt(hdc, 0, 0, width, height, hdcImage, 0, 0, width, height, SRCCOPY);
		lpDDSurf->ReleaseDC(hdc);
	    }

    DeleteDC(hdcImage);
    DeleteObject(hbm);
	}

HBITMAP dxSurface::OpenBitmap()
	{
	HBITMAP	hbm;
	// resource?
    hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), bitmapname, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hbm == NULL) // file?
	hbm = (HBITMAP)LoadImage(NULL, bitmapname, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);
    if (hbm == NULL) return NULL; // doesnt exist...
	return hbm;
	}

void	dxSurface::Display(int destx,int desty,int srcx,int srcy,int srcwid,int srcht,LPDIRECTDRAWSURFACE surf)
	{
	RECT	srcrect;
	HRESULT	ddrval;
	int i;

	srcrect.left=srcx; srcrect.right=srcx+srcwid;
	srcrect.top=srcy; srcrect.bottom=srcy+srcht;

	// clip away!!!
	if (destx+srcwid<=0 || desty+srcht<=0 ||
		destx>=640 || desty>=480) return; // totally offscreen?
	if (destx<0) {
		i=0-destx;
		destx=0; // destx+=i
		srcrect.left+=i; }
	if (desty<0) {
		i=0-desty;
		desty=0; // desty+=i
		srcrect.top+=i; }
	if (destx+srcwid>640) {
		i=destx+srcwid-640;
		srcrect.right-=i; }
	if (desty+srcht>480) {
		i=desty+srcht-480;
		srcrect.bottom-=i; }

	if (surf==NULL) surf=lpDDSBack;

	if (!surf || !lpDDSurf) return; // not a goer!
	if (lpDDSurf->IsLost()==DDERR_SURFACELOST) Load(bitmapname);
    while(1) // keep on trying
		{
		if (colourkey)
			ddrval = surf->BltFast(destx, desty, lpDDSurf,
					&srcrect, DDBLTFAST_SRCCOLORKEY );
		else
			ddrval = surf->BltFast(destx, desty, lpDDSurf,
					&srcrect, DDBLTFAST_NOCOLORKEY );
		ddrval=DD_OK; // ******
		if (ddrval == DD_OK) break; // done!
		if (ddrval == DDERR_SURFACELOST) Load(bitmapname); //Restore(); // fix it up
		else if (ddrval != DDERR_WASSTILLDRAWING) break; // weird error!
		break;
		}
	}


void dxSurface::UnLoad()
	{
	if (lpDDSurf != NULL) {
		lpDDSurf->Release();
		lpDDSurf=NULL;
		}
	}

void dxSurface::SetColourKey(int on)
	{
    DDCOLORKEY          ddck;
	if (!lpDDSurf) return;
	if (on) {
		ddck.dwColorSpaceLowValue  = ColorMatch(CLR_INVALID);
		ddck.dwColorSpaceHighValue = ddck.dwColorSpaceLowValue;
		lpDDSurf->SetColorKey(DDCKEY_SRCBLT, &ddck);
		colourkey=true;
		}
	else	colourkey=0;
	}

DWORD dxSurface::ColorMatch(COLORREF rgb)
	{
    COLORREF rgbT;
    HDC hdc;
    DWORD dw = CLR_INVALID;
    DDSURFACEDESC ddsd;
    HRESULT hres;

    //  use GDI SetPixel to color match for us
    if (rgb != CLR_INVALID && lpDDSurf->GetDC(&hdc) == DD_OK) {
		rgbT = GetPixel(hdc, 0, 0);             // save current pixel value
		SetPixel(hdc, 0, 0, rgb);               // set our value
		lpDDSurf->ReleaseDC(hdc); }

    // now lock the surface so we can read back the converted color
    ddsd.dwSize = sizeof(ddsd);
    while ((hres = lpDDSurf->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING)
	;

    if (hres == DD_OK) {
		dw  = *(DWORD *)ddsd.lpSurface;                     // get DWORD
			if(ddsd.ddpfPixelFormat.dwRGBBitCount < 32)
				dw &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount)-1;  // mask it to bpp
		lpDDSurf->Unlock(NULL); }

    //  now put the color that was there back.
    if (rgb != CLR_INVALID && lpDDSurf->GetDC(&hdc) == DD_OK) {
		SetPixel(hdc, 0, 0, rgbT);
		lpDDSurf->ReleaseDC(hdc); }

    return dw;
	}

void dxSurface::Text(char *text,int x, int y,int bias,LPDIRECTDRAWSURFACE surf)
	{
	int	curchar,c,curx,cury;

	curx=x; cury=y;
	for (curchar=0;curchar<10000 && text[curchar];curchar++)	{
		c=text[curchar];
		if (c=='\n') {
			cury+=textht;
			curx=x; }
		else	{
			c+=bias;
			Display(curx,cury,(c%16)*textwid,c/16*textht,textwid,textht,surf);
			curx+=textwid;
			}
		}
	}
