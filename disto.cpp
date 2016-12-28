/*
 * Subject : Deformers
 * Author : Ilyes Gouta
 * Notes : <nothing>
 *
 */

#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <ddraw.h>
#include <string.h>
#include "disto.h"
#include "ddutil.h"

#define VIEW  50
#define PI    3.141529f

__int16 TableX[640*480],TableY[640*480];

void add_distortion(DistortionSet** set,Distortion* disto)
{
	DistortionSet* tmp;

	if (!*set)
	{

		*set=(DistortionSet*) GlobalAlloc(GPTR,sizeof(DistortionSet));
		(*set)->disto=disto;
		return;
	} else
	{
		tmp=*set;
		while (tmp->next) tmp=tmp->next;
		tmp->next=(DistortionSet*) GlobalAlloc(GPTR,sizeof(DistortionSet));
		tmp->next->disto=disto;
	}
}

int get_distortion_count(DistortionSet* disto_set)
{
	int i=0;
	DistortionSet* tmp=disto_set;
	while (tmp)
	{
		i++;
		tmp=tmp->next;
	}
	return i;
}

void free_distortion_set(DistortionSet* set)
{
	if (set->next) free_distortion_set(set->next);
	GlobalFree(set);
	set=NULL;
}

Distortion* get_distortion(DistortionSet* set,int index)
{
	DistortionSet* tmp=set;
	int i=0;

	if (!set) return NULL;

	while (tmp && i<index)
	{
		tmp=tmp->next;
		i++;
	}
	if (tmp) return (tmp->disto); else return NULL;
}

void __inline MapRect (unsigned char* in_Surf,unsigned char *out_Surf,int ax,int ay,int bx,int by,int cx,int cy)
{
	float dx,dy,sx,sy;
	int x,y,offset=0;

	dx=(float)(bx-ax)/640;
	dy=(float)(cy-ay)/480;
	sx=(float) ax;sy=(float) ay;

	for (y=0;y<480;y++)
	{
		for (x=0;x<640;x++)
		{
			out_Surf[offset]=in_Surf[(int)sx+((int)sy<<7)+((int)sy<<9)];
			sx+=dx;
			offset++;
		}
		sy+=dy;
		sx=(float) ax;
	}

}

void __inline Polar(int x,int y,float* d,float* r)
{
	*d=sqrt((float) (x*x+y*y));
	*r=acos((float) (x/(*d)));
	if (y<0) *r=-*r;
}

void __inline Cartesian(float d,float r,int* x,int* y)
{
	*x=(int) (d*cos(r));
	*y=(int) (d*sin(r));
}

void Distort(unsigned char* in_Surf,unsigned char* out_Surf)
{
	int x,y,_x,_y,offset=0;

	for (y=0;y<480;y++)
	{
		for (x=0;x<640;x++)
		{
			_x=TableX[offset];
			_y=TableY[offset];
			if (_x>=0 && _x<640 && _y>=0 && _y<480) out_Surf[offset]=in_Surf[_x+(_y<<7)+(_y<<9)]; else out_Surf[offset]=0;
			offset++;
		}
	}
}

void Identity (unsigned char* in_Surf,unsigned char* out_Surf)
{
	__asm
	{
		mov esi,in_Surf;
		mov edi,out_Surf;
		mov ecx,307200;
__Copy:
		movq MM0,[esi];
		movq MM1,[esi+8];
		movq MM2,[esi+16];
		movq MM3,[esi+24];

		movq [edi],MM0;
		movq [edi+8],MM1;
		movq [edi+16],MM2;
		movq [edi+24],MM3;

		add esi,32;
		add edi,32;
		sub ecx,32;
		jnz __Copy;
		emms;
	}
}

void Noise (unsigned char* in_Surf,unsigned char* out_Surf)
{
	int x,y,u,v;

	for (y=1;y<479;y++)
	{
		u=(rand()%3)-1;v=(rand()%3)-1;
		for (x=1;x<639;x++)
		{
			out_Surf[x+(y<<7)+(y<<9)]=in_Surf[x+u+((y+v)<<7)+((y+v)<<9)];
		}
	}
}

void InitZoom(void)
{
	int x,y,offset=0;

	for (y=0;y<480;y++)
	{
		for (x=0;x<640;x++)
		{
			TableX[offset]=(int) ((320-x)/5+x);
			TableY[offset]=(int) (y);
			offset++;
		}
	}
}

void InitWater(void)
{
	int x,y,offset=0;
	float z,d,u,v;

	for (y=0;y<480;y++)
	{
		for (x=0;x<640;x++)
		{
			u=(float) (x-320);v=(float) (y-240);
			d=sqrt(u*u+v*v);
			z=40.0*fabs(sin(d/20))+20;
			TableX[offset]=(int) ((VIEW*u/z-u)/8+x);
			TableY[offset]=(int) ((VIEW*v/z-v)/8+y);
			offset++;
		}
	}
}

void InitRoto(void)
{
	int x,y,offset=0;
	float u,v;

	for (y=0;y<480;y++)
	{
		for (x=0;x<640;x++)
		{
			u=(float) (x-320);v=(float) (y-240);
			TableX[offset]=(int) (cos(0.0349f)*u+sin(0.0349f)*v-u+x);
			TableY[offset]=(int) (-sin(0.0349f)*u+cos(0.0349f)*v-v+y);
			offset++;
		}
	}
}

void InitSwirl(void)
{
	int x,y,u,v,offset=0;
	float r,d;

	for (y=0;y<480;y++)
	{
		for (x=0;x<640;x++)
		{
			u=x-320;v=y-240;
			Polar(u,v,&d,&r);
			r=r+0.1*cos(d/20);
			Cartesian(d,r,&u,&v);
			TableX[offset]=(int) (u+320);
			TableY[offset]=(int) (v+240);
			offset++;
		}
	}
}

void InitShift(void)
{
	int x,y,offset=0;

	for (y=0;y<480;y++)
	{
		for (x=0;x<640;x++)
		{
			TableX[offset]=(int) (x+5);
			TableY[offset]=(int) (y);
			offset++;
		}
	}
}

void InitBowl(void)
{
	int x,y,u,v,offset=0;
	float r,d;

	for (y=0;y<480;y++)
	{
		for (x=0;x<640;x++)
		{
			u=x-320;v=y-240;
			Polar(u,v,&d,&r);
			d=1.25*400*fabs(asin(0.75f*d/400.0f));
			r=r+0.02;
			Cartesian(d,r,&u,&v);
			TableX[offset]=(int) (u+320);
			TableY[offset]=(int) (v+240);
			offset++;
		}
	}
}

void InitRotoZoomer(void)
{
	int x,y,u,v,offset=0;
	float r,d;

	for (y=0;y<480;y++)
	{
		for (x=0;x<640;x++)
		{
			u=x-320;v=y-240;
			Polar(u,v,&d,&r);
			d=d*1.02;r=r+0.07;
			Cartesian(d,r,&u,&v);
			TableX[offset]=(int) (u+320);
			TableY[offset]=(int) (v+240);
			offset++;
		}
	}
}

void InitMosaic(void)
{
	int x,y,offset=0;
	float u,v,r,d,tmpx,tmpy;

	for (y=0;y<480;y++)
	{
		for (x=0;x<640;x++)
		{
			u=(float)(x-320)/320.0f;v=(float)(y-240)/240.0f;
			tmpx=u*0.94f+sin(v*PI*8)*0.03f;
			tmpy=v*0.94f+sin(u*PI*8)*0.03f;
			TableX[offset]=(int) (320.0f*tmpx+320);
			TableY[offset]=(int) (240.0f*tmpy+240);
			offset++;
		}
	}
}

Distortion WATER={Distort,InitWater,"Water"};
Distortion ZOOM={Distort,InitZoom,"Zoom"};
Distortion ROTO={Distort,InitRoto,"Roto"};
Distortion BOWL={Distort,InitBowl,"Bowl"};
Distortion SWIRL={Distort,InitSwirl,"Swirl"};
Distortion SHIFT={Distort,InitShift,"Shift"};
Distortion ROTOZOOMER={Distort,InitRotoZoomer,"RotoZoomer"};
Distortion IDENT={Identity,NULL,"None"};
Distortion NOISE={Noise,NULL,"Noise"};
Distortion MOSAIC={Distort,InitMosaic,"Mosaic"};