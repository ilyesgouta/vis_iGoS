/*
 * Subject : disto prototypes
 * Author : Ilyes Gouta
 * Notes : <nothing>
 *
 */

#ifndef __DISTORTION__
#define __DISTORTION__

typedef struct Distortion
{
	void (*Disto)(unsigned char* in_Surf,unsigned char* out_Surf);
	void (*InitTable) (void);
	char effect_name[20];
} Distortion;

typedef struct DistortionSet
{
	Distortion* disto;
	DistortionSet* next;
} DistortionSet;

/* the distortion functions */

void Distort(unsigned char* in_Surf,unsigned char* out_Surf);
void Identity (unsigned char* in_Surf,unsigned char* out_Surf);
void Noise (unsigned char* in_Surf,unsigned char* out_Surf);
//void BumpMap (unsigned char* in_Surf,unsigned char* out_Surf);

/* the members */

void InitWater(void);
void InitZoom(void);
void InitRoto(void);
void InitBowl(void);
void InitBowl2(void);
void InitSwirl(void);
void InitShift(void);
void InitRotoZoomer(void);

/*Utilities*/

extern "C" void DumpPalette(PALETTEENTRY* nColors);

/* protos */

void free_distortion_set(DistortionSet* set);
int get_distortion_count(DistortionSet* disto_set);
void add_distortion(DistortionSet** set,Distortion* disto);
Distortion* get_distortion(DistortionSet* set,int index);
void _init_(LPDIRECTDRAW ddraw);

/* some externs */

extern Distortion WATER;
extern Distortion ZOOM;
extern Distortion ROTO;
extern Distortion BOWL;
extern Distortion BOWL2;
extern Distortion SWIRL;
extern Distortion SHIFT;
extern Distortion ROTOZOOMER;
extern Distortion IDENT;
extern Distortion NOISE;
extern Distortion MOSAIC;
//extern Distortion BUMPMAP;

#endif;