// dxSurface.h: interface for the dxSurface class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DXSURFACE_H__B22C18E1_052C_11D2_95DD_0000E8D1D170__INCLUDED_)
#define AFX_DXSURFACE_H__B22C18E1_052C_11D2_95DD_0000E8D1D170__INCLUDED_

#include <ddraw.h>

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class dxSurface  
	{
	protected:
		DWORD ColorMatch(COLORREF rgb);
		HBITMAP OpenBitmap();
		char	bitmapname[1024];
		long	width,height,textwid,textht;
		void Restore();
		LPDIRECTDRAWSURFACE	lpDDSurf,lpDDSBack;
		LPDIRECTDRAW	lpDD;
		int	colourkey,loadedon;

	public:
		void Text(char *text,int x,int y,int bias=0,LPDIRECTDRAWSURFACE surf=NULL);
		void SetColourKey(int on);
		void	UnLoad();
		void	Display(int destx,int desty,int srcx,int srcy,int srcwid,int srcht,LPDIRECTDRAWSURFACE surf=NULL);
		void	SetDD(LPDIRECTDRAW _lpDD) {lpDD=_lpDD;}
		void	SetBack(LPDIRECTDRAWSURFACE _lpDDSBack) {lpDDSBack=_lpDDSBack;}
		void	Load(char *name);
		dxSurface();
		dxSurface(LPDIRECTDRAW _lpDD);
		virtual ~dxSurface();
	};

#endif // !defined(AFX_DXSURFACE_H__B22C18E1_052C_11D2_95DD_0000E8D1D170__INCLUDED_)
