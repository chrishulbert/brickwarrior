#define WIN32_LEAN_AND_MEAN
#define	INITGUID
#include <windows.h>
#include <windowsx.h>
#include <ddraw.h>

#include "dxGame.h"
#include "resource.h"
#include "getdxver.h"
//#include "dsutil.h"
//#include "input.h"

#include "settings.h"

extern	dxGame	theGame;

LPDIRECTDRAW		lpDD;		// DirectDraw object
LPDIRECTDRAWSURFACE	lpDDSPrimary;	// DirectDraw primary surface
LPDIRECTDRAWSURFACE	lpDDSBack;	// DirectDraw back surface
//LPDIRECTDRAWSURFACE	lpDDSOne;	// Offscreen surface 1
LPDIRECTDRAWPALETTE	lpDDPal;	// DirectDraw palette
BOOL				bActive;	// is application active?
HWND	hwnd; // the main window handle
int		mousebutton; // is the mouse button down?

char	keybuffer[256],keystat[256];
int		curkeybuf;

LPDIRECTDRAWPALETTE	DDLoadPalette(LPCSTR szBitmap)
	{
    LPDIRECTDRAWPALETTE	ddpal;
    int                 i,n,fh;
    HRSRC               h;
    LPBITMAPINFOHEADER  lpbi;
    PALETTEENTRY        ape[256];
    RGBQUAD *           prgb;

    //
    // build a 332 palette as the default.
    //
    for (i=0; i<256; i++)
    {
	ape[i].peRed   = (BYTE)(((i >> 5) & 0x07) * 255 / 7);
	ape[i].peGreen = (BYTE)(((i >> 2) & 0x07) * 255 / 7);
	ape[i].peBlue  = (BYTE)(((i >> 0) & 0x03) * 255 / 3);
	ape[i].peFlags = (BYTE)0;
    }

    //
    // get a pointer to the bitmap resource.
    //
    if (szBitmap && (h = FindResource(NULL, szBitmap, RT_BITMAP)))
    {
	lpbi = (LPBITMAPINFOHEADER)LockResource(LoadResource(NULL, h));
	if (!lpbi)
	    OutputDebugString("lock resource failed\n");
	prgb = (RGBQUAD*)((BYTE*)lpbi + lpbi->biSize);

	if (lpbi == NULL || lpbi->biSize < sizeof(BITMAPINFOHEADER))
	    n = 0;
	else if (lpbi->biBitCount > 8)
	    n = 0;
	else if (lpbi->biClrUsed == 0)
	    n = 1 << lpbi->biBitCount;
	else
	    n = lpbi->biClrUsed;

	//
	//  a DIB color table has its colors stored BGR not RGB
	//  so flip them around.
	//
	for(i=0; i<n; i++ )
	{
	    ape[i].peRed   = prgb[i].rgbRed;
	    ape[i].peGreen = prgb[i].rgbGreen;
	    ape[i].peBlue  = prgb[i].rgbBlue;
	    ape[i].peFlags = 0;
	}
    }
    else if (szBitmap && (fh = _lopen(szBitmap, OF_READ)) != -1)
    {
	BITMAPFILEHEADER bf;
	BITMAPINFOHEADER bi;

	_lread(fh, &bf, sizeof(bf));
	_lread(fh, &bi, sizeof(bi));
	_lread(fh, ape, sizeof(ape));
	_lclose(fh);

	if (bi.biSize != sizeof(BITMAPINFOHEADER))
	    n = 0;
	else if (bi.biBitCount > 8)
	    n = 0;
	else if (bi.biClrUsed == 0)
	    n = 1 << bi.biBitCount;
	else
	    n = bi.biClrUsed;

	//
	//  a DIB color table has its colors stored BGR not RGB
	//  so flip them around.
	//
	for(i=0; i<n; i++ )
	{
	    BYTE r = ape[i].peRed;
	    ape[i].peRed  = ape[i].peBlue;
	    ape[i].peBlue = r;
	}
    }

	if (!lpDD) return NULL;
    lpDD->CreatePalette(DDPCAPS_8BIT, ape, &ddpal, NULL);

    return ddpal;
	} // DDLoadPalette

void	finiObjects()
	{ // deallocate to your hearts content!
	if( lpDD != NULL ) {
		if( lpDDSPrimary != NULL ) {
			lpDDSPrimary->Release();
			lpDDSPrimary = NULL; }

		if( lpDDPal != NULL ) {
			lpDDPal->Release();
			lpDDPal = NULL; }

		lpDD->Release();
		lpDD = NULL; }
	}

long FAR PASCAL WindowProc( HWND hWnd, UINT message, 
			    WPARAM wParam, LPARAM lParam )
	{
	switch( message )
		{
		case WM_SYSCOMMAND: // ignore all alt-related stuff
			break;

		case WM_ACTIVATEAPP:
			bActive = wParam;
			break;

		case WM_LBUTTONDOWN:
			mousebutton=true;
			return true; // dealt with

		case WM_LBUTTONUP:
			mousebutton=false;
			return true; // dealt with

		case WM_KEYDOWN: // store the key pressed
			keybuffer[curkeybuf]=wParam;
			keybuffer[++curkeybuf]=0;
			return true;

		case WM_SETCURSOR:
			SetCursor(NULL);
			return TRUE;

		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;
		}

	return DefWindowProc(hWnd, message, wParam, lParam);
	} /* WindowProc */

BOOL initFail(HWND hwnd,char *text,char *title)
	{
	finiObjects();
	MessageBox(hwnd,text,title,MB_OK);
	DestroyWindow(hwnd);
	return FALSE;
	} /* initFail */

#define	CLASSNAME	"REDSHIFTBRICKWARRIOR"
#define	TITLE	"BrickWarrior"
BOOL	doInit(HINSTANCE hInstance,int nCmdShow)
	{
	WNDCLASS		wc;
	DDSURFACEDESC	ddsd;
	DDSCAPS		ddscaps;
	HRESULT		ddrval;

	/*
	* set up and register window class
	*/
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_MAIN));
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = GetStockBrush(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = CLASSNAME;
	RegisterClass( &wc );

	/*
	* create a window
	*/
	hwnd = CreateWindowEx(
		0,
		CLASSNAME,
		TITLE,
		WS_POPUP,
		0,
		0,
		0,//GetSystemMetrics(SM_CXSCREEN),
		0,//GetSystemMetrics(SM_CYSCREEN),
		NULL,
		NULL,
		hInstance,
		NULL );

	if (!hwnd) return FALSE;

	ShowWindow(hwnd,nCmdShow);
	UpdateWindow(hwnd);

	// is directx installed?
	DWORD	dxver,dxplat;
	GetDXVersion(&dxver,&dxplat);
	if (dxver < 0x500 ) { // bodgy old version!
		if (!dxver) // no directx at all!
			return initFail(hwnd,"You need to install DirectX!",TITLE);
		else // old directx
			return initFail(hwnd,"You need to install the newer DirectX 5",TITLE);
		}

	// creation! (of the main directx object ;) )
	ddrval = DirectDrawCreate( NULL, &lpDD, NULL );
	if( ddrval != DD_OK ) return initFail(hwnd,"Couldn't create DirectDraw object (install DirectX 5)",TITLE);

	// Get exclusive mode
	ddrval = lpDD->SetCooperativeLevel( hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN );
	if( ddrval != DD_OK ) return initFail(hwnd,"Couldn't set the cooperative level (try closing other games or rebooting)",TITLE);

	// Set the video mode to whatever
	ddrval = lpDD->SetDisplayMode(WIDTH,HEIGHT,BITS);
	if( ddrval != DD_OK ) return initFail(hwnd,"Couldn't set the display mode (try a reboot or getting new video drivers from 'http://www.drivershq.com')",TITLE);

	// Create the primary surface with 1 back buffer
	ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
		DDSCAPS_FLIP |
		DDSCAPS_COMPLEX;
	ddsd.dwBackBufferCount = 1;
	ddrval = lpDD->CreateSurface( &ddsd, &lpDDSPrimary, NULL );
	if (ddrval != DD_OK) return initFail(hwnd,"Cannot create the primary surface (try a reboot)",TITLE);

	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	ddrval = lpDDSPrimary->GetAttachedSurface(&ddscaps, &lpDDSBack);
	if (ddrval != DD_OK) return initFail(hwnd,"Cannot get the back buffer (try a reboot)",TITLE);

	// create and set the palette if needed
	if (BITS==8) {
		lpDDPal = DDLoadPalette("sprites.bmp");
		if (lpDDPal) lpDDSPrimary->SetPalette(lpDDPal);
		}
	else	lpDDPal=NULL;

	return TRUE;
	} /* doInit */

HRESULT restoreAll( void )
	{
	HRESULT	ddrval;

	if (!lpDDSPrimary) return DD_OK; // should be an error actually...
	ddrval = lpDDSPrimary->Restore();
	return ddrval;
	} /* restoreAll */

void	ClearBackBuffer()
	{
	DDBLTFX     ddbltfx;
	HRESULT     ddrval;

	if (!lpDDSBack) return; // should be an error
	// Erase the background
	ddbltfx.dwSize = sizeof( ddbltfx );
	ddbltfx.dwFillColor = 0; // black
	while( 1 ) {
		ddrval = lpDDSBack->Blt (NULL, NULL,
			NULL, DDBLT_COLORFILL, &ddbltfx);

		if (ddrval == DD_OK ) break; // yes!!!
		if (ddrval == DDERR_SURFACELOST)
			if (restoreAll()!=DD_OK) return; // try to restore it
		if (ddrval != DDERR_WASSTILLDRAWING) return;
		}
	} /* ClearBackBuffer */

void updateFrame( void )
	{
	HRESULT		ddrval;

	if (!lpDDSPrimary) return; // should be an error
	// Flip the surfaces
	while( 1 ) {
		ddrval = lpDDSPrimary->Flip( NULL, 0 );

		if (ddrval == DD_OK) break; // cool
		if (ddrval == DDERR_SURFACELOST) {
			ddrval = restoreAll();
			if (ddrval != DD_OK) break; } // uhoh... didnt work
		if (ddrval != DDERR_WASSTILLDRAWING) break; // some weird error! RUN!!!
		}
	ClearBackBuffer(); // clear the back buffer
	} /* updateFrame */

int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
			LPSTR lpCmdLine, int nCmdShow)
	{
	MSG		msg;
	DWORD	newtime,oldtime,deltatime;
	int	oldactive;
	POINT	mousepos;

	lpCmdLine = lpCmdLine;
	hPrevInstance = hPrevInstance;

	if (!doInit(hInstance,nCmdShow)) return FALSE;

	theGame.SetDX(lpDD,lpDDSPrimary,lpDDSBack);
	theGame.Init(hwnd);

	newtime=GetTickCount();
	keybuffer[0]=curkeybuf=0;
	mousebutton=false;
	oldactive=true;

	SetCursorPos(320,240);
	while (1)
		{
		// get delta time
		oldtime=newtime;
		newtime=GetTickCount(); // same as timeGetTime
		//if (newtime<oldtime) deltatime=newtime-oldtime;
		deltatime=newtime-oldtime;
		
		if (theGame.HasQuit()) PostMessage(hwnd, WM_CLOSE, 0, 0);
		keybuffer[0]=curkeybuf=0;
		while (PeekMessage(&msg,NULL,0,0,PM_NOREMOVE)) {
			if (!GetMessage(&msg,NULL,0,0)) {
				// deallocate the surfaces
				theGame.DeInit();
				finiObjects();
				return msg.wParam;
				}
			TranslateMessage(&msg); 
			DispatchMessage(&msg);
			}
		if( bActive ) {
			for (int i=0;i<256;i++) keystat[i]=(char)GetKeyState(i)&0x80; // get keys
			GetCursorPos(&mousepos);

			theGame.UpdateFrame(keybuffer,keystat,mousepos.x-320,mousepos.y-240,mousebutton,deltatime);

			SetCursorPos(320,240);

			theGame.DrawFrame();
			updateFrame(); // flip then clear back
			}
		else // make sure we go to sleep if we have nothing else to do
			WaitMessage();

		if (oldactive && !bActive) theGame.SetPause(true); // go into menu when they alt-tab out
		oldactive=bActive;
		}
	} /* WinMain */
