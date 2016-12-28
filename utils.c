/*
 * Subject : Some utilities
 * Author : Ilyes Gouta
 * Notes : <nothing>
 *
 */

#include <windows.h>
#include <stdio.h>

void DumpPalette(PALETTEENTRY* nColors)
{
	/*Dumps a JASC compatible palette*/

	FILE *file;
	char buffer[50];
	int k;

	file=fopen("c:\\palette.pal","wb");
	if (file==NULL) return;
	strcpy(buffer,"JASC-PAL\r\n0100\r\n256\r\n");
	fwrite(buffer,1,strlen(buffer),file);
	for (k=0;k<256;k++)
	{
		sprintf(buffer,"%d %d %d\r\n",nColors[k].peRed,nColors[k].peGreen,nColors[k].peBlue);
		fwrite(buffer,1,strlen(buffer),file);
	}
	fclose(file);
}