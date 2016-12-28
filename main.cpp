/*
 * Subject : iGoS's visualization for Winamp
 * Author : Ilyes Gouta
 * Notes : <nothing>
 *
 */

#include <windows.h>
#include <ddraw.h>
#include <math.h>
#include <stdio.h>
#include "vis.h"
#include "disto.h"
#include "frontend.h"

#define Timer                    20

#define WIDTH                   640
#define HEIGHT                  480

#define PALETTE_TIMER            22
#define DEFORMER_TIMER           23
#define TEXT_TIMER               24

#define DEFORMERS_COUNT          12

#define FONT_SIZE                28
#define TEXT_DISPLAY_INTERVAL    10
#define MAINTAIN_TEXT            50

char App[]="iGoS visualization plug-in";

__int64 BlurMask=0xf8f8f8f8f8f8f8f8;
__int64 Mask=0xfefefefefefefefe;
__int64 FadeMask=0x0101010101010101;

int Palette_Time=5,Deformer_Time=10;

LPDIRECTDRAW DDObj=NULL;
LPDIRECTDRAW2 Object=NULL;
LPDIRECTDRAWSURFACE Surf=NULL,BackSurf=NULL,OffScreen=NULL;
DDSURFACEDESC SDesc,in_Lock,out_Lock;
LPDIRECTDRAWPALETTE Palette=NULL;
LPPALETTEENTRY p_entries=NULL;
PALETTEENTRY Colors[256],nColors[256];
DDSCAPS caps;
DDBLTFX FX;
BOOL showfps=FALSE;
HWND hPlugWnd;
BOOL EnableBump=FALSE;

BOOL Lock_Def=FALSE,Lock_Pal=FALSE;
int fps,mfps;

int Frames_To_Maintain=0;
char Text[]="iGoS plug-in for Winamp";
int X_Text,Y_Text;

DistortionSet* set=NULL;
Distortion* distortion=NULL;

unsigned char light_map[256*256];
unsigned char mem[640*480];

void Identity (unsigned char* in_Surf,unsigned char* out_Surf);

BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd,UINT Message,WPARAM wP,LPARAM lP);

void config(struct winampVisModule *this_mod);
int init(struct winampVisModule *this_mod);
int render(struct winampVisModule *this_mod);
void quit(struct winampVisModule *this_mod);

winampVisModule *getModule(int which);
winampVisHeader hdr = {0x101,"iGoS visualization plug-in for Winamp v0.0",getModule};

winampVisModule mod = {"iGoS plug-in",NULL,NULL,0,0,5,5,0,2,{ 0, },{ 0, },config,init,render,quit};

winampVisModule *getModule(int which)
{
	switch (which)
	{
		case 0: return &mod;
		default:return NULL;
	}
}

#ifdef __cplusplus
extern "C" {
#endif
__declspec( dllexport ) winampVisHeader* winampVisGetHeader()
{
	return &hdr;
}
#ifdef __cplusplus
}
#endif

void __inline ZeroSurface(unsigned char* lpSurf)
{
	__asm
	{
		mov ecx, 38400;
		mov edi,lpSurf;
		pxor MM0,MM0;
__Zero:
		movq [edi],MM0;
		movq [edi+8],MM0;
		movq [edi+16],MM0;
		movq [edi+24],MM0;
		add edi,32;
		sub ecx,4;
		jnz __Zero
		emms;
	}
}

void SetPalette(unsigned char r,unsigned char g,unsigned char b)
{
	int x;
	if (Palette->GetEntries(0, 0, 256, nColors) != DD_OK) return;
	for (x=0;x<128;x++)
	{
		nColors[x].peRed=r*x/128;
		nColors[x].peGreen=g*x/128;
		nColors[x].peBlue=b*x/128;
		nColors[x+128].peRed=r+(255-r)*x/128;
		nColors[x+128].peGreen=g+(255-g)*x/128;
		nColors[x+128].peBlue=b+(255-b)*x/128;
	}
	if(Palette->SetEntries(0, 0, 256, nColors) != DD_OK) return;
}

void CALLBACK PaletteProc(HWND  hwnd,UINT  uMsg,UINT  idEvent,DWORD  dwTime)
{
	if (!Lock_Pal) SetPalette(rand()%255,rand()%255,rand()%255);
}

void CALLBACK DeformerProc(HWND  hwnd,UINT  uMsg,UINT  idEvent,DWORD  dwTime)
{
	if (!Lock_Def)
	{
		distortion=get_distortion(set,rand()%get_distortion_count(set));
		if (distortion->InitTable) distortion->InitTable();
	}
}

void CALLBACK TextProc(HWND,UINT,UINT,DWORD)
{
	Frames_To_Maintain=MAINTAIN_TEXT;
	X_Text=rand()%( (639-(FONT_SIZE>>1)*lstrlen(Text))>0 ? (639-(FONT_SIZE>>1)*lstrlen(Text)) : 1);Y_Text=rand()%400;
}

void config(struct winampVisModule *this_mod)
{
	char message[]="This plug-in can be altered at run-time.\n\nKeys :\n\nL : Locks/unlocks the current map.\n"
		"N : Changes the map (at random).\nSPACE : Toggles on/off FPS and the current effect's name.\n"
		"P : Changes the palette.\nO : Locks/unlocks the current palette.\nESC : Exit.\n\nThis plug-in requires DirectX 3 (or above) and an MMX processor.\n\n"
		"Web site : http://members.nbci.com/igsite\nE-Mail : igouta@voila.fr\n\n"
		"Built on "__DATE__" at "__TIME__".\n(C) entirely Ilyes Gouta 2001.";

	MessageBox(this_mod->hwndParent,message,App,MB_ICONINFORMATION);
}

void __init_stuff()
{
	FILE* file;

	file=fopen("c:\\light_pattern.raw","rb");
	if (file) {fread(light_map,1,sizeof(light_map),file);fclose(file);}

	add_distortion(&set,&WATER);
	add_distortion(&set,&ZOOM);
	add_distortion(&set,&ROTO);
	add_distortion(&set,&BOWL);
	add_distortion(&set,&SWIRL);
	add_distortion(&set,&SHIFT);
	add_distortion(&set,&ROTOZOOMER);
	add_distortion(&set,&IDENT);
	add_distortion(&set,&MOSAIC);
	add_distortion(&set,&NOISE);

	distortion=get_distortion(set,rand()%get_distortion_count(set));
	if (distortion->InitTable) distortion->InitTable();
}

int init(struct winampVisModule *this_mod)
{
	WNDCLASS wndClass;
	HRESULT hr;
	int index;

	ShowCursor(FALSE);
	ShowWindow(this_mod->hwndParent,SW_MINIMIZE);

	wndClass.style=0;
	wndClass.lpfnWndProc=WndProc;
	wndClass.cbClsExtra=0;
	wndClass.cbWndExtra=0;
	wndClass.hInstance=this_mod->hDllInstance;
	wndClass.hIcon=NULL;
	wndClass.hCursor=NULL;
	wndClass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName=NULL;
	wndClass.lpszClassName="PluginFrame";

	RegisterClass(&wndClass);

	hPlugWnd=CreateWindow("PluginFrame",App,WS_POPUP,CW_USEDEFAULT,CW_USEDEFAULT,WIDTH,HEIGHT,NULL,NULL,this_mod->hDllInstance ,NULL);
	ShowWindow(hPlugWnd,SW_SHOWNORMAL);

	SetWindowLong(hPlugWnd,GWL_USERDATA,(LONG) this_mod);

	hr=DirectDrawCreate(NULL,&DDObj,NULL);
	if (hr != DD_OK) 
	{
		MessageBox(NULL,"Can't initialize DirectDraw object !",App,MB_ICONSTOP);
		quit(NULL);
		return 1;
	}

	hr = DDObj->QueryInterface(IID_IDirectDraw2,(LPVOID *) &Object); 
	if (hr != DD_OK) 
	{
		MessageBox(NULL,"Can't find IID_DirectDraw2 interface !",App,MB_ICONSTOP);
		quit(NULL);
		return 1; 
	}

	hr=Object->SetCooperativeLevel(hPlugWnd,DDSCL_ALLOWREBOOT|DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN);
	if (hr != DD_OK) 
	{
		MessageBox(NULL,"Can't set the cooperative level !",App,MB_ICONSTOP);
		quit(NULL);
		return 1;
	}

	hr=Object->SetDisplayMode(WIDTH,HEIGHT,8,0,0);
	if (hr != DD_OK)
	{
		MessageBox(NULL,"Can't set display mode !",App,MB_ICONSTOP);
		quit(NULL);
		return 1;
	}

	ZeroMemory(&SDesc,sizeof(SDesc));

	SDesc.dwSize = sizeof(SDesc); 
	SDesc.dwFlags = DDSD_CAPS;
	SDesc.ddsCaps.dwCaps =DDSCAPS_PRIMARYSURFACE|DDSCAPS_SYSTEMMEMORY;

	hr=Object->CreateSurface(&SDesc,&Surf,NULL);
	if (hr != DD_OK) 
	{
		MessageBox(NULL,"Can't create primary surface !",App,MB_ICONSTOP);
		quit(NULL);
		return 1;
	}

	ZeroMemory(&SDesc,sizeof(SDesc));

	SDesc.dwSize = sizeof(SDesc); 
	SDesc.dwFlags = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
	SDesc.dwWidth=WIDTH;
	SDesc.dwHeight=HEIGHT;
	SDesc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;

	hr=Object->CreateSurface(&SDesc,&BackSurf,NULL);
	if (hr != DD_OK) 
	{
		MessageBox(NULL,"Can't create back surface !",App,MB_ICONSTOP);
		quit(NULL);
		return 1;
	}

	ZeroMemory(&SDesc,sizeof(SDesc));

	SDesc.dwSize = sizeof(SDesc); 
	SDesc.dwFlags = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
	SDesc.dwWidth=WIDTH;
	SDesc.dwHeight=HEIGHT;
	SDesc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;

	hr=Object->CreateSurface(&SDesc,&OffScreen,NULL);
	if (hr != DD_OK)
	{
		MessageBox(NULL,"Can't create offscreen surface !",App,MB_ICONSTOP);
		quit(NULL);
		return 1;
	}

	for (index = 0; index <= 255 ; index++) 
	{ 
		Colors[index].peFlags = 0; 
		Colors[index].peRed = 0;
		Colors[index].peGreen = index; 
		Colors[index].peBlue = 0; 
	}
     
	if (Object->CreatePalette(DDPCAPS_8BIT,Colors,&Palette,NULL) != DD_OK) 
	{
		MessageBox(NULL,"Can't create the palette !",App,MB_ICONSTOP);
		quit(NULL);
		return 1;
	}

	Surf->SetPalette(Palette);
	BackSurf->SetPalette(Palette);
	OffScreen->SetPalette(Palette);

	ZeroMemory(&FX,sizeof(FX));

	FX.dwSize=sizeof(FX);
	FX.dwFillColor=0;

	Surf->Blt(NULL,NULL,NULL,DDBLT_COLORFILL|DDBLT_WAIT,&FX);
	BackSurf->Blt(NULL,NULL,NULL,DDBLT_COLORFILL|DDBLT_WAIT,&FX);
	OffScreen->Blt(NULL,NULL,NULL,DDBLT_COLORFILL|DDBLT_WAIT,&FX);

	srand(GetTickCount());
	SetPalette(rand()%255,rand()%255,rand()%255);

	__init_stuff();

	SetTimer(hPlugWnd,PALETTE_TIMER,1000*Palette_Time,(TIMERPROC) PaletteProc);
	SetTimer(hPlugWnd,DEFORMER_TIMER,1000*Deformer_Time,(TIMERPROC) DeformerProc);
	SetTimer(hPlugWnd,TEXT_TIMER,1000*TEXT_DISPLAY_INTERVAL,(TIMERPROC) TextProc);

	//Starting...
	return 0;
}

void BumpMap (unsigned char* in_Surf,unsigned char* out_Surf)
{
	int deltax,deltay;
	int u,v;

	for (int y=1;y<480;y++)
	{
		for (int x=0;x<640;x++)
		{
			u=(y<<7)+(y<<9);
			v=((y-1)<<7)+((y-1)<<9);
			deltax=in_Surf[x+u]-in_Surf[x-1+u];
			deltay=in_Surf[x+u]-in_Surf[x+v];
			if (!(deltax<128 && deltax>-128)) deltax=128;  
			if (!(deltay<128 && deltay>-128)) deltay=128;
			out_Surf[x+u]=in_Surf[x+u]+light_map[128+deltax+((128+deltay)<<8)];
		}
	}
}

int render(struct winampVisModule *this_mod)
{
	HRESULT hr;
	int s,k;
	unsigned char* p;
	RECT r={5,5,635,475};
	HDC dc;HPEN pen;

	ZeroMemory(&in_Lock,sizeof(in_Lock));
	ZeroMemory(&out_Lock,sizeof(out_Lock));

	in_Lock.dwSize=sizeof(in_Lock);
	out_Lock.dwSize=sizeof(out_Lock);

	hr=OffScreen->Lock(NULL,&in_Lock,NULL,NULL);
	if (hr == DD_OK)
	{
		p=(unsigned char*) in_Lock.lpSurface;
		hr=BackSurf->Lock(NULL,&out_Lock,NULL,NULL);
		if (hr == DD_OK)
		{
			distortion->Disto((unsigned char *) in_Lock.lpSurface,(unsigned char*) out_Lock.lpSurface);
			
			//  Blurring

			__asm 
				{
					mov ecx,38240;
					mov edi,in_Lock.lpSurface;
					
					mov esi,out_Lock.lpSurface;
					add edi,640;
					
					add esi,640;
					movq MM5,[BlurMask];
					movq MM7,[Mask];
					movq MM0,[FadeMask];
					
			Blur:

					movq MM1,[esi-1];
					movq MM2,[esi+1];

					movq MM6,[esi];
					pand MM1,MM5;

					pand MM6,MM7;
					movq MM3,[esi-640];

					psrlq MM6,1;
					pand MM2,MM5;

					psrlq MM1,3;
					pand MM3,MM5;

					psrlq MM2,3;
					movq MM4,[esi+640];

					paddusb MM1,MM2;
					psrlq MM3,3;

					pand MM4,MM5;
					paddusb MM1,MM3;

					psrlq MM4,3;
					paddusb MM1,MM4;

					paddusb MM1,MM6
					add edi,8;

					psubusb MM1,MM0;

					add esi,8;
					movq [edi-8],MM1;

					dec ecx;
					jnz Blur;

					emms;
			}
			
			BackSurf->Unlock(out_Lock.lpSurface);
			}
		OffScreen->Unlock(in_Lock.lpSurface);
		}

	OffScreen->GetDC(&dc);
	pen=CreatePen(PS_SOLID,1,0xffffff);
	SelectObject(dc,pen);
	MoveToEx(dc,32,(int)(0.5f*(this_mod->waveformData[0][0]^128))+176,NULL);
	for (k=0;k<576;k++)
	{
		LineTo(dc,32+k,(int)(0.5f*(this_mod->waveformData[0][k]^128))+176);
	}
	OffScreen->ReleaseDC(dc);
	DeleteObject(pen);

	if (Frames_To_Maintain != 0)
	{
		HDC dc;HFONT hOldFont,hFont;
		int pos;char*name;
		pos=SendMessage(this_mod->hwndParent ,WM_WA_IPC,0,IPC_GETLISTPOS);
		name=(char*) SendMessage(this_mod->hwndParent ,WM_WA_IPC,pos,IPC_GETPLAYLISTTITLE);
		OffScreen->GetDC(&dc);
		hFont=CreateFont(FONT_SIZE,0, 0, 0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,VARIABLE_PITCH,"Arial");
		SetBkMode(dc,TRANSPARENT);
		SetTextColor(dc,RGB(255,255,255));
		SelectObject(dc,hFont);
		TextOut(dc,X_Text,Y_Text,name,lstrlen(name));
		OffScreen->ReleaseDC(dc);
		DeleteObject(hFont);
		Frames_To_Maintain--;
	}
	
	if (EnableBump)
	{
		OffScreen->Lock(NULL,&in_Lock,NULL,NULL);
		BumpMap((unsigned char*) in_Lock.lpSurface,mem);
		OffScreen->Unlock(in_Lock.lpSurface);
		Surf->Lock(NULL,&in_Lock,NULL,NULL);
		Identity(mem,(unsigned char*) in_Lock.lpSurface);
		Surf->Unlock(in_Lock.lpSurface);
	} else Surf->Blt(NULL,OffScreen,NULL,0,NULL);

	if (showfps==TRUE)
	{
		HDC dc;char buff[255];
		Surf->GetDC(&dc);
		wsprintf(buff,"FPS : %d current effect : %s",mfps,distortion->effect_name);
		TextOut(dc,1,1,buff,lstrlen(buff));
		Surf->ReleaseDC(dc);
	}
	
	fps++;
	return 0;
}

void quit(struct winampVisModule *this_mod)
{
	if (Palette != NULL)
	{
		Palette->Release();
		Palette=NULL;
	}

	if (OffScreen != NULL)
	{
		OffScreen->Release();
		OffScreen=NULL;
	}

	if (Surf != NULL)
	{
		Surf->Release();
		Surf=NULL;
	}

	if (Object)
	{
		Object->RestoreDisplayMode();
		Object->Release();
		Object=NULL;
	}
	if (DDObj)
	{
		DDObj->Release();
		DDObj=NULL;
	}

	DestroyWindow(hPlugWnd);
	ShowCursor(TRUE);
	ShowWindow(this_mod->hwndParent,SW_RESTORE);
}

void CALLBACK FPS_TimerProc(HWND,UINT,UINT,DWORD)
{
	mfps=fps;
	fps=0;
}

LRESULT CALLBACK WndProc(HWND hWnd,UINT Message,WPARAM wP,LPARAM lP)
{
	switch(Message)
	{
	case WM_CREATE:
		{
			SetTimer(hWnd,Timer,1000,(TIMERPROC) FPS_TimerProc);
			return 0;
		}
	case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	case WM_KEYDOWN:
		{
			if (wP==32) {showfps=!showfps;return 0;}
			if (wP=='P') {PaletteProc(hWnd,WM_TIMER,PALETTE_TIMER,0);return 0;}
			if (wP=='B') {EnableBump=!EnableBump;return 0;}
			if (wP=='L') {Lock_Def=!Lock_Def;return 0;}
			if (wP=='O') {Lock_Pal=!Lock_Pal;return 0;}
			if (wP=='N') {DeformerProc(hWnd,WM_TIMER,DEFORMER_TIMER,0);return 0;}
			if (wP=='E') {DumpPalette(nColors);return 0;}
			if (wP==27) {winampVisModule *this_mod = (winampVisModule *) GetWindowLong(hWnd,GWL_USERDATA);quit(this_mod);return 0;}

			winampVisModule *this_mod = (winampVisModule *) GetWindowLong(hWnd,GWL_USERDATA);
			PostMessage(this_mod->hwndParent,Message,wP,lP);
			return 0;
		}
	default:return (DefWindowProc(hWnd,Message,wP,lP));
	}
}