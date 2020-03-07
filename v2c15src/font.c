/* V2CHR editor FONT.C module, largely taken from Brad Smith's */
/* NewChaos libraries */

#include "video.h"
#include "fontdef.h" /* defines every character VS ASCII */
#include <stdio.h>

#include "fontset.h" /* approx 629 bytes of font data */
void (*blitfont)(int x, int y, unsigned char *string, unsigned char c);

void blitfont13h(int x, int y, unsigned char *string,unsigned char c)
{ int xoff,yoff,yoc,i,j;
  unsigned char k;
  xoff=x;
  yoff=y*swd;
  while(string[0]!=0)
  { yoc=yoff+xoff;
    i=fontchr[string[0]];
    i*=6;
    for(j=6;j;j--)
    { k=fontset[i];
      if(k & 1) { vid[yoc]=c; }
      if(k & 2) { vid[yoc+1]=c; }
      if(k & 4) { vid[yoc+2]=c; }
      if(k & 8) { vid[yoc+3]=c; }
      if(k & 16) { vid[yoc+4]=c; }
      i++;
      yoc+=swd;
    }
    xoff+=6;
  string++;}
return;}

void blitfontmxl(int x, int y, unsigned char *string,unsigned char c)
{ unsigned char k;
  int i,j;
  while(string[0]!=0)
  { i=fontchr[string[0]];
    i*=6;
    for(j=6;j;j--)
    { k=fontset[i];
      if(k & 1) { plot(x,y,c); }
      if(k & 2) { plot(x+1,y,c); }
      if(k & 4) { plot(x+2,y,c); }
      if(k & 8) { plot(x+3,y,c); }
      if(k & 16) { plot(x+4,y,c); }
      i++;
      y++;
    }
    x+=6;
    y-=6;
  string++;}
return;}

void fontfuncs()
{ if(vmd==VMD_13H)
  { blitfont=&blitfont13h;
    return; }
  if(vmd>=VMD_MXL)
  { blitfont=&blitfontmxl;
    return; }
return;}

/* End of module. */
