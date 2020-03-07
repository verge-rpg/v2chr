/* Verge 2 CHR graphic editor module: VIDEO.C */
/* Palette, Screen Setting, and Drawing       */
/*   Brad Smith January 1999                  */

#include <stdlib.h>
#include <i86.h>
// Watcom C interrupt syntax reference
//     regs.w.ax=short;
//     union REGS regs;
//     int386(intnm,&regs,&regs);

unsigned int sw;
unsigned int sh;
unsigned int swd;
unsigned char* vid;
unsigned char vmd;

unsigned char* vesabuff;

void (*plot)(int x,int y, unsigned char c);
void (*hline)(int x1,int y,int x2, unsigned char c);
void (*vline)(int x,int y1,int y2, unsigned char c);
void (*box)(int x1,int y1,int x2,int y2, unsigned char c);
void (*cls)();
unsigned char (*getpix)(int x, int y);

void SDVideo()
{ union REGS regs;
  regs.w.ax=0x03;
  int386(0x10,&regs,&regs);
  if(vmd==3) { free(vesabuff); }
  vmd=0;
  vid=0;
  sw=0;
  swd=0;
  sh=0;
return; }

#define VMD_13H    1 // 320x200 chained
#define VMD_MXL    2 // 320x240 unchained
#define VMD_VESA   3 // 640x480 VESA

void vesainit();

void InitScreen(char mode)
{ union REGS regs;

  if(!mode) { SDVideo(); return; }
  if(mode<=2)
 { regs.w.ax=0x13;
   int386(0x10,&regs,&regs);
   vid=(unsigned char *)0xA0000;

   if(mode==VMD_13H) { vmd=1; sw=320; sh=200; swd=320; return; }

  /* VGA setup is taken care of, now unchaining */

// VGA register ports reference
//   Sequencer is at 0x3c4
//   CRTC is at 0x3d4

   if(mode==VMD_MXL)
   { outpw(0x3c4, 0x0604); // Chain 4 off
     outpw(0x3d4, 0xE317); // Word mode off
     outpw(0x3d4, 0x0014); // DWord mode off

     outp(0x3c2, 0xe3);  // vertical sync polarity
     outpw(0x3d4, 0x2c11); // write protect off
     outpw(0x3d4, 0x0d06); // vertical total
     outpw(0x3d4, 0x3e07); // overflow register
     outpw(0x3d4, 0xea10); // vertical retrace start
     outpw(0x3d4, 0xac11); // vertical retrace end, protect
     outpw(0x3d4, 0xdf12); // vertical display enable end
     outpw(0x3d4, 0xe715); // start vertical blanking
     outpw(0x3d4, 0x0616); // end vertical blanking

     vmd=2;
     sw=320;
     sh=240;
     swd=80;
     return;
   }
 }
 else if (mode==3)
 {
   vesabuff=malloc(640*480);
   if(vesabuff==NULL)
   { printf("Not enough memory to run VESA 640x480 mode.\n");
     exit(0xFF); }
   regs.w.bx=0x0101;
   regs.w.ax=0x4f02;
   int386(0x10,&regs,&regs);
   /* screen size is 344044 */
   sw=640;
   sh=480;
   swd=640;
   vid=(unsigned char *)0xA0000;
   vmd=3;
 return;}


return; }

/* 13h prototypes */
void plot13h(int x, int y, unsigned char c);
void hline13h(int x1, int y1, int x2, unsigned char c);
void vline13h(int x1, int y1, int y2, unsigned char c);
void box13h(int x1, int y1, int x2, int y2, unsigned char c);
void cls13h();
unsigned char getpix13h(int x, int y);

/* Mode-X low prototypes */
void plotmxl(int x, int y, unsigned char c);
void hlinemxl(int x1, int y1, int x2, unsigned char c);
void vlinemxl(int x1, int y1, int y2, unsigned char c);
void boxmxl(int x1, int y1, int x2, int y2, unsigned char c);
void clsmxl();
unsigned char getpixmxl(int x, int y);

/* VESA prototypes */
void vesabox(int x1,int y1, int x2, int y2, unsigned char c);
void vesavline(int x, int y1, int y2, unsigned char c);
void vesahline(int x1,int y, int x2, unsigned char c);
void vesacls();
unsigned char vesagetpix(int x, int y);
void vesapixel(int x, int y, unsigned char c);

void PointPrim(char mode)
{ if(mode==VMD_13H)
  { plot=&plot13h;
    hline=&hline13h;
    vline=&vline13h;
    box=&box13h;
    cls=&cls13h;
    getpix=&getpix13h;
    swd=320;
    return;
  }
  if(mode==VMD_MXL)
  { plot=&plotmxl;
    hline=&hlinemxl;
    vline=&vlinemxl;
    box=&boxmxl;
    cls=&clsmxl;
    getpix=&getpixmxl;
    swd=80;
    return;
  }
  if(mode==VMD_VESA)
  { plot=&vesapixel;
    hline=&vesahline;
    vline=&vesavline;
    box=&vesabox;
    cls=&vesacls;
    getpix=&vesagetpix;
    swd=640;
    return;
  }
return; }

void InitVideo(char mode)
{ InitScreen(mode);
  PointPrim(mode);
  if(mode) cls();
return; }

/* Drawing Primitives */

#include <string.h>

/* 13h primitives */
void plot13h(int x, int y, unsigned char c)
{ vid[x+(y*swd)]=c; return; }

void hline13h(int x1, int y1, int x2, unsigned char c)
{ int dlen;

  if(x1>x2) {int x3; x3=x2; x2=x1; x1=x3; }
  dlen=x2-x1+1;

  memset(vid+x1+(y1*swd),c,dlen);
return; }

void vline13h(int x1, int y1, int y2, unsigned char c)
{ int dlen;
  unsigned char* p;

  if(y1>y2) {int y3; y3=y2; y2=y1; y1=y3; }
  dlen=y2-y1+1;

  p=vid+(y1*swd)+x1;

  while(dlen)
  { p[0]=c;
    p+=swd;
    dlen--;
  }
return; }

void box13h(int x1, int y1, int x2, int y2, unsigned char c)
{ int xlen,ylen;
  unsigned char* p;

  if(x1>x2) {int x3; x3=x2; x2=x1; x1=x3; }
  if(y1>y2) {int y3; y3=y2; y2=y1; y1=y3; }
  xlen=x2-x1+1;
  ylen=y2-y1+1;

  p=vid+(y1*swd)+x1;

  while(ylen)
  { memset(p,c,xlen);
    p+=swd;
    ylen--;
  }
return; }

void cls13h()
{ memset(vid,0,64000); return; }

unsigned char getpix13h(int x, int y)
{ return vid[x+(y*swd)]; }

/* Mode-X low primitives */

// VGA register ports reference
//   Sequencer is at 0x3c4
//   CRTC is at 0x3d4

void plotmxl(int x, int y, unsigned char c)
{ outp(0x3c4,0x02);
  outp(0x3c5,1<<(x&3));
  vid[(swd*y)+(x>>2)]=c;
return;}

void hlinemxl(int x1, int y1, int x2, unsigned char c)
{ unsigned char *p;
  int i;
  int xm1,xm2;
  int xmln;

  if(x1>x2) {int x3; x3=x2; x2=x1; x1=x3; }
  xm1=x1>>2;
  xm2=x2>>2;

  p=vid+(x1>>2)+(y1*swd);

  if(xm1==xm2) /* lines shorter than 4 */
  { for(i=(x1&3);i<=(x2&3);i++)
    { outp(0x3c4,0x02);
      outp(0x3c5,1<<i);
      p[0]=c;
    }
  return; }

  /* first */
  for(i=(x1&3);i<4;i++)
  { outp(0x3c4,0x02);
    outp(0x3c5,1<<i);
    p[0]=c;
  }
  p++;

  /* middle */
  xm1++;
//  xm2--;
  xmln=xm2-xm1;
  while(xmln)
  { outp(0x3c4,0x02);
    outp(0x3c5,0x0f);
    p[0]=c;
    p++;
    xmln--;
  }

  /* last */
  for(i=0;i<=(x2&3);i++)
  { outp(0x3c4,0x02);
    outp(0x3c5,1<<i);
    p[0]=c;
  }

return;}

void vlinemxl(int x1, int y1, int y2, unsigned char c)
{ unsigned char *p;
  int dlen;

  outp(0x3c4,0x02);
  outp(0x3c5,1<<(x1&3));

  if(y1>y2) {int y3; y3=y2; y2=y1; y1=y3; }
  dlen=y2-y1+1;

  p=vid+(y1*swd)+(x1>>2);

  while(dlen)
  { p[0]=c;
    p+=swd;
    dlen--;
  }
return;}

void boxmxl(int x1, int y1, int x2, int y2, unsigned char c)
{ int dlen;

  if(y1>y2) {int y3; y3=y2; y2=y1; y1=y3; }
  dlen=y2-y1+1;

  while(dlen)
  { hline(x1,y1,x2,c);
    y1++;
    dlen--;
  }
return;}

void clsmxl()
{ outp(0x3c4,0x02);
  outp(0x3c5,0x0f); /* Write to all planes */
  memset(vid,0,swd*sh);
return; }

unsigned char getpixmxl(int x, int y)
{ outp(0x3ce,0x04); /* Read port set */
  outp(0x3cf,x&3);  /* Read plane set */
return vid[(swd*y)+(x>>2)]; }

/* Palette Setting */

void setpal(unsigned char *pal)
{ int i;
  outp(0x03c6,0xff);
  outp(0x03c8,0);
  for(i=0;i<768;i++)
  { outp(0x03c9,pal[i]); }
return; }

void getpal(unsigned char *pal)
{ int i;
  outp(0x03c6,0xff);
  outp(0x03c7,0);
  for(i=0;i<768;i++)
  { pal[i]=inp(0x03c9); }
return; }

void setcol(unsigned char c, unsigned char r, unsigned char g, unsigned char b)
{ outp(0x03c6,0xff);
  outp(0x03c8,c);
  outp(0x03c9,r);
  outp(0x03c9,g);
  outp(0x03c9,b);
return; }

void getcol(unsigned char c, unsigned char *r, unsigned char *g, unsigned char *b)
{ outp(0x03c6,0xff);
  outp(0x03c7,c);
  *r=inp(0x03c9);
  *g=inp(0x03c9);
  *b=inp(0x03c9);
return; }

// VESA code

void vesabank(int bank)
{ union REGS regs;
  regs.w.ax=0x4f05;
  regs.w.bx=0;
  regs.w.dx=bank;
  int386(0x10,&regs,&regs);
return; }

void vesabuffer()
{ if(vmd==3)
  { int bsize;
    int bank;
    int boff;
    int banksize=65536;
    bsize=640*480;
    bank=0;
    while(bsize>=banksize)
    { vesabank(bank);
      memcpy(vid,vesabuff+(bank<<16),banksize);
      bsize-=banksize;
      bank++;
    }
    if(bsize)
    { vesabank(bank);
      memcpy(vid,vesabuff+(bank<<16),bsize);
    }
  }
return; }

void vesapixel(int x, int y, unsigned char c)
{ vesabuff[x+(y*swd)]=c; return; }

void vesahline(int x1, int y1, int x2, unsigned char c)
{ int dlen;

  if(x1>x2) {int x3; x3=x2; x2=x1; x1=x3; }
  dlen=x2-x1+1;

  memset(vesabuff+x1+(y1*swd),c,dlen);
return; }

void vesavline(int x1, int y1, int y2, unsigned char c)
{ int dlen;
  unsigned char* p;

  if(y1>y2) {int y3; y3=y2; y2=y1; y1=y3; }
  dlen=y2-y1+1;

  p=vesabuff+(y1*swd)+x1;

  while(dlen)
  { p[0]=c;
    p+=swd;
    dlen--;
  }
return; }

void vesabox(int x1, int y1, int x2, int y2, unsigned char c)
{ int xlen,ylen;
  unsigned char* p;

  if(x1>x2) {int x3; x3=x2; x2=x1; x1=x3; }
  if(y1>y2) {int y3; y3=y2; y2=y1; y1=y3; }
  xlen=x2-x1+1;
  ylen=y2-y1+1;

  p=vesabuff+(y1*swd)+x1;

  while(ylen)
  { memset(p,c,xlen);
    p+=swd;
    ylen--;
  }
return; }

void vesacls()
{ memset(vesabuff,0,640*480); return; }

unsigned char vesagetpix(int x, int y)
{ return vesabuff[x+(y*swd)]; }

void vesarealplot(int x, int y, unsigned char c)
{ int a,bank;
  a=(y*swd)+x;
  bank=a>>16;
  a-=(bank<<16);
  vesabank(bank);
  vid[a]=c;
return; }

/* OLD Cheezy vesa routines

unsigned char vesagetpix(int x, int y)
{ unsigned char c;
  int a,bank;
  a=(y*swd)+x;
  bank=a>>16;
  a-=(bank<<16);
  vesabank(bank);
  c=vid[a];
return c; }

void vesacls()
{ int bsize;
  int bank;
  int banksize=65536;
  bsize=640*480;
  bank=0;
  while(bsize>=banksize)
  { vesabank(bank);
    memset(vid,0,banksize);
    bsize-=banksize;
    bank++;
  }
  if(bsize)
  { vesabank(bank);
    memset(vid,0,bsize);
  }
return; }  

void vesahline(int x1,int y, int x2, unsigned char c)
{ int i;
  if(x1>x2) {int x3; x3=x2; x2=x1; x1=x3; }
  for(i=x1;i<=x2;i++)
  { vesapixel(i,y,c); }
return; }

void vesavline(int x, int y1, int y2, unsigned char c)
{ int i;
  if(y1>y2) {int y3; y3=y2; y2=y1; y1=y3; }
  for(i=y1;i<=y2;i++)
  { vesapixel(x,i,c); }
return; }

void vesabox(int x1,int y1, int x2, int y2, unsigned char c)
{ int i;
  if(x1>x2) {int x3; x3=x2; x2=x1; x1=x3; }
  if(y1>y2) {int y3; y3=y2; y2=y1; y1=y3; }
  for(i=y1;i<=y2;i++)
  { vesahline(x1,i,x2,c); }
return; }

End of old cheezy vesa routines */

// End of VESA code.

/* End of module - First edition completed January 30th 1999, Brad Smith */
