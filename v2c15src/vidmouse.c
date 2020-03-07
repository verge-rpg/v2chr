/* V2 CHR graphic editor module: VIDMOUSE.C */
/* DOS Interrupt driven Mouse driver interface */
/* Brad Smith 1999 */

/* Mouse routines borrowed from the NewChaos editor shell */
/* which was also written by Brad Smith. */

#include <i86.h>
#include "video.h"

void (*mseblit)(int x, int y);
void (*msebolt)(int x, int y);
int mousex,mousey,mousek;
int mox,moy,mok;
unsigned char mousebox[64];

unsigned char msecursors[192]
 = { 1  ,179,179,166,0  ,0  ,0  ,0  ,
     179,179,179,168,166,0  ,0  ,0  ,
     179,179,172,170,168,166,0  ,0  ,
     176,176,174,172,170,168,166,0  ,
     0  ,176,176,174,172,170,168,166,
     0  ,0  ,176,176,174,172,170,170,
     0  ,0  ,0  ,176,176,174,172,0  ,
     0  ,0  ,0  ,0  ,176,174,0  ,0  ,

     255,255,255,255,255,0  ,0  ,0  ,
     255,255,  0,  0,  0,  0,  0,  0,
     255,  0,255,  0,  0,  0,  0,  0,
     255,  0,  0,255,  0,  0,  0,  0,
     255,  0,  0,  0,255,  0,  0,  0,
     0  ,  0,  0,  0,  0,255,  0,  0,
     0  ,  0,  0,  0,  0,  0,225,  0,
     0  ,  0,  0,  0,  0,  0,  0,255  } ;
unsigned char *msecurs=msecursors;

/* prototype drawmouse functions */
void mseblit13h(int x, int y);
void msebolt13h(int x, int y);
void mseblitmxl(int x, int y); /* These ones are actually generic */
void mseboltmxl(int x, int y); /* These ones are actually generic */
void mseblitvesa(int x, int y);
void mseboltvesa(int x, int y);

void msedrawfuncs(char mode)
{ if(mode==VMD_13H)
  { mseblit=&mseblit13h;
    msebolt=&msebolt13h;
    return; }
  if(mode==VMD_MXL)
  { mseblit=&mseblitmxl;
    msebolt=&mseboltmxl;
    return; }
  if(mode==VMD_VESA)
  { mseblit=&mseblitvesa;
    msebolt=&mseboltvesa;
    return; }
return; }

int mseinit()
{ union REGS regs;

  regs.w.ax=0x0;
  int386(0x33, &regs, &regs);
  if(regs.w.ax!=0xFFFF) return 0;
  regs.w.ax=7;
  regs.w.cx=0;
  regs.w.dx=((sw<<1)-1);
  int386(0x33, &regs, &regs);
  regs.w.ax=8;
  regs.w.cx=0;
  regs.w.dx=(sh-1);
  int386(0x33, &regs, &regs);

  msedrawfuncs(vmd);
return 1;}

void msepoll()
{ union REGS regs;

  regs.w.ax=0x3;
  int386(0x33, &regs, &regs);
  mok=mousek;
  mousek=regs.w.bx&0x01;
  mousek+=regs.w.bx&0x02;
  regs.w.ax=0x03;
  int386(0x33, &regs, &regs);
  mox=mousex;
  moy=mousey;
  mousex=regs.w.cx>>1;
  mousey=regs.w.dx;
return; }

void msebolt13h(int x, int y)
{ int i,j;
  unsigned char *off;
  j=sw-x;
  if(j>8) j=8;
  off=(y*swd+x)+vid;
  for(i=0;i<8;i++)
  { if(y+i<sh)
    { memcpy(off,mousebox+(i*8),j); }
    off+=swd;
  }
return; }

void mseblit13h(int x, int y)
{ int i,j,k,l;
  unsigned char *off;
  l=sw-x;
  if(l>8) l=8;
  off=(y*swd+x)+vid;
  for(i=0;i<8;i++)
  { if(y+i<sh)
    { memcpy(mousebox+(i*8),off,l); }
    off+=swd;
  }
  k=0;
  off=(y*swd+x)+vid;
  for(i=0;i<8;i++)
  { for(j=0;j<8;j++)
    { if(msecurs[k] && j<l)
      { off[0]=msecurs[k]; }
      k++;
      off++;
    }
    off+=(swd-8);
  }
return; }

void mseboltmxl(int x, int y)
{ int i,ix,iy;
  for(i=0;i<64;i++)
  { ix=(i&7)+x;
    iy=(i>>3)+y;
    if(ix<sw && iy<sh) { plot(ix,iy,mousebox[i]); }
  }
return; }

void mseblitmxl(int x, int y)
{ int i,ix,iy;
  for(i=0;i<64;i++)
  { ix=(i&7)+x;
    iy=(i>>3)+y;
    if(ix<sw && iy<sh) { mousebox[i]=getpix(ix,iy); }
  }
  for(i=0;i<64;i++)
  { ix=(i&7)+x;
    iy=(i>>3)+y;
    if(ix<sw && iy<sh && msecurs[i]) { plot(ix,iy,msecurs[i]); }
  }
return; }

void mseblitvesa(int x, int y)
{ int i,ix,iy;
  for(i=0;i<64;i++)
  { ix=(i&7)+x;
    iy=(i>>3)+y;
    if(ix<sw && iy<sh) { mousebox[i]=getpix(ix,iy); }
  }
  for(i=0;i<64;i++)
  { ix=(i&7)+x;
    iy=(i>>3)+y;
    if(ix<sw && iy<sh && msecurs[i]) { vesarealplot(ix,iy,msecurs[i]); }
  }
return;}

void mseboltvesa(int x, int y)
{ int i,ix,iy;
  for(i=0;i<64;i++)
  { ix=(i&7)+x;
    iy=(i>>3)+y;
    if(ix<sw && iy<sh) { vesarealplot(ix,iy,mousebox[i]); }
  }
return;}

/* End of module. First edition completed January 30th 1999 */
