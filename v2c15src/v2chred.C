//   Verge 2 CHR character graphic file editor
//               Brad Smith 1998

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h> /* Ahh, the usual four... well, 3 and conio. */

/* --- Two quick functions --- */
#include <time.h> /* cheezy timer, watcom specific version */
unsigned long int cheezytimerstart;
int timerinterval=CLOCKS_PER_SEC/100; /* set the timer to 100 t/s */
void initctimer() { cheezytimerstart=clock(); return; }
int ctimer() { return (clock()-cheezytimerstart)/timerinterval; }

#include <i86.h> /* I need the 16h-02h shift status interrupt */
unsigned char shiftstats()
{ union REGS regs; unsigned char rval=0;
  regs.h.ah=0x02; int386(0x16,&regs,&regs); rval=regs.h.al;
return rval; }
/* insert, caps, num, scroll, alt, ctrl, lshift, rshift (7-0) */
/* --- End of quick functions --- */

#include "video.h"
#include "vidmouse.h"
extern void parseconfig(); /* Found in v2chrcfg.c */
#include "font.h"
extern void guidrawb(int x1, int y1, int x2, int y2, char* text);
extern void guidrawbc(int x1, int y1, int x2, int y2, char* text);
extern void guidrawbd(int x1, int y1, int x2, int y2, char* text);
extern void guisysgrey();
extern int loadpcx(char *filename, unsigned char *bm,int w,int h,unsigned char *pal);
extern int savepcx(unsigned char *filename,unsigned char *bm, int w, int h, unsigned char *pal);

typedef struct
{ int frames;
  int height;
  int width;
  int hotspotx;
  int hotspoty;
  int hotspotwidth;
  int hotspotheight;
} chrtemplt;

typedef struct { unsigned char list[200]; } scriptop;
typedef struct { unsigned short int quan[200]; } scriptqn;

/* --- Global Variables --- */
unsigned char syspal[768]; /* For returning the system palette to normal */

char stpvmd;
char backupon;
char backupfile[76];
int backuptime;
unsigned char gcols[9]={255,8,239,16,12,10,20,24,8};
chrtemplt chrtdef={30,32,16,0,16,16,16};

unsigned char *cdata;
int cdtalloc, cdtused;
int cfrmsize;
int cframes,cheight,cwidth,chotx,choty,chotw,choth;
scriptop scrops[8];
scriptqn squant[8];
char cfname[75];
char *cfront; /* points to 13 character filename */
char chrtype=0; /* v1 or v2 type, 0=v2 1=v1 */
int cfup=0; int cfdown=0; int cfleft=0; int cfright=0;

unsigned char edpal[768];

char edmode=0;
int edframe=0;
int anistr=0;
int anifrm=0;
int aniprt=0;
int anitime=0;
int anion=0;
unsigned char ecoll=0;
unsigned char ecolr=255;
char gridon=0;
int cfnamemax;

int ecleft=0; /* left side of CHR edit box */
int ecxmax=0; int ecymax=0; // bottom right corner
char scrollxon=0;
char scrollyon=0;
int scroffx=0;
int scroffy=0;
int scrbx1,scrbx2; /* sides of the x scrollbutton */
int scrby1,scrby2; /* top and bottom of the y scrollbutton */
int scrxmax,scrymax;

unsigned char zoom=2; // goes up to 8 (zoom=3 because it's <<)
char zoomcfg=0;
char invon=0; // whether or not screen colours (0/255) are inverted.

char clipstats=0;
int clipwidth=0;
int clipheight=0;
//static char *clipstattxt[3]={"Empty","Frame","Shape"};
unsigned char *clipboard;
unsigned char *tempchr;
unsigned char *undo;

char drawmode=0;

int screensavertimer;

/* --- Load and Save routines --- */
unsigned short int v1ccnum[8][16] =
 { {16,10,17,10,16,10,15,10,18,10,19,10,18,10,15,10},
   {11,10,12,10,11,10,10,10,13,10,14,10,13,10,10,10},
   {6,10,7,10,6,10,5,10,8,10,9,10,8,10,5,10},
   {1,10,2,10,1,10,0,10,3,10,4,10,3,10,0,10},
   {15,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {10,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {5,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0} };


int loadchr(char *fnm)
{ short int lcframes,lcheight,lcwidth,lchotx,lchoty,lchotw,lchoth;
  long int lcfup,lcfdown,lcfright,lcfleft;
  char cver;
  unsigned char *lcdata;
  FILE *f;
  int i;

  f=fopen(fnm,"rb"); if(f==NULL) { return 2; }

  cver=fgetc(f);
  if((cver!=1 && cver!=2) || chrtype==1) /* Not version 1,assume V1 chr. */
  { lcframes=30; lcheight=32; lcwidth=16;
    lchotx=0; lchoty=16; lchotw=16; lchoth=16;
    lcdata=malloc(31*32*16+1); /* leave a little bit for buffer */
    if(lcdata==NULL)
    { fclose(f);
      return 1; }

    lcdata[0]=cver;
    i=1;

    while(!feof(f) && i<(30*32*16))
    { lcdata[i]=fgetc(f); /* One at a time rather than an fread is slower */
      i++; }              /* but is more stable if the file is corrupted. */

    strcpy(scrops[0].list,"FWFWFWFWFWFWFWFW");
    strcpy(scrops[1].list,"FWFWFWFWFWFWFWFW");
    strcpy(scrops[2].list,"FWFWFWFWFWFWFWFW");
    strcpy(scrops[3].list,"FWFWFWFWFWFWFWFW");
    for(i=0;i<4;i++) { scrops[i].list[16]=255; }
    for(i=0; i<4; i++) { memcpy(&squant[i].quan,&v1ccnum[i],32); }
    lcfleft=15; lcfright=10; lcfup=5; lcfdown=0;
  }
  else if(cver==1) // V2 Chr, version 1 (uncompressed, had 8 scripts)
  { int j,k,l;
    long int sln;
    char decom[2048]; /* 2k buffer for undoing the scripts */
    char cp;
    fread(&lcwidth,1,2,f); fread(&lcheight,1,2,f);
    fread(&lchotx,1,2,f); fread(&lchoty,1,2,f);
    fread(&lchotw,1,2,f); fread(&lchoth,1,2,f);
    fread(&lcframes,1,2,f);
    if(lcwidth<0 || lcwidth>500) return 1;
    if(lcheight<0 || lcheight>500) return 1;
    if(lcframes<0 || lcframes>1024) return 1;
    if(lchotx<-999 || lchotx>9999) return 1;
    if(lchoty<-999 || lchoty>9999) return 1;
    if(lchotw<-999 || lchotw>9999) return 1;
    if(lchoth<-999 || lchoth>9999) return 1;
    lcdata=malloc((lcwidth*lcheight*lcframes)+1);
    if(lcdata==NULL) { fclose(f); return 1; }
    fread(lcdata,1,lcwidth*lcheight*lcframes,f);
    for(i=0;i<4;i++) /* time for my nifty script decompressor */
    { fread(&sln,1,4,f); k=0; fread(decom,1,sln,f); sln--;
      for(j=0;j<sln;j=j)
      { scrops[i].list[k]=decom[j]; j++; l=j;
        while(decom[l]!='F' && decom[l]!='W' && l<sln) l++;
        cp=decom[l]; decom[l]=0;
        squant[i].quan[k]=atoi(decom+j);
        j=l; decom[j]=cp; k++;
      }
      scrops[i].list[k]=255; }
    /* End of old decompressor, now to try and get the 4 idles... */
    fread(&sln,1,4,f); fread(decom,1,sln,f); sln--;
    for(j=0;j<sln;j++) { if(decom[j]=='W') { k=j; j=sln; } } decom[k]=0;
    lcfleft=atoi(decom+1);
    fread(&sln,1,4,f); fread(decom,1,sln,f); sln--;
    for(j=0;j<sln;j++) { if(decom[j]=='W') { k=j; j=sln; } } decom[k]=0;
    lcfright=atoi(decom+1);
    fread(&sln,1,4,f); fread(decom,1,sln,f); sln--;
    for(j=0;j<sln;j++) { if(decom[j]=='W') { k=j; j=sln; } } decom[k]=0;
    lcfup=atoi(decom+1);
    fread(&sln,1,4,f); fread(decom,1,sln,f); sln--;
    for(j=0;j<sln;j++) { if(decom[j]=='W') { k=j; j=sln; } } decom[k]=0;
    lcfdown=atoi(decom+1);
  }
  else if(cver==2) // V2 Chr, version 2 (RLE compressed, 4 scripts, idles)
  { int j,k,l;
    long int sln;
    char decom[2048]; /* 2k buffer for undoing the scripts */
    char cp; long int buffsize; unsigned char *combuff;
    fread(&lcwidth,1,2,f); fread(&lcheight,1,2,f);
    fread(&lchotx,1,2,f); fread(&lchoty,1,2,f);
    fread(&lchotw,1,2,f); fread(&lchoth,1,2,f);
    fread(&lcframes,1,2,f);
    if(lcwidth<0 || lcwidth>500) return 1;
    if(lcheight<0 || lcheight>500) return 1;
    if(lcframes<0 || lcframes>1024) return 1;
    if(lchotx<-999 || lchotx>9999) return 1;
    if(lchoty<-999 || lchoty>9999) return 1;
    if(lchotw<-999 || lchotw>9999) return 1;
    if(lchoth<-999 || lchoth>9999) return 1;
    lcdata=malloc((lcwidth*lcheight*lcframes)+1);
    if(lcdata==NULL) { fclose(f); return 1; }
    fread(&buffsize,1,4,f);
    combuff=malloc(buffsize);
    if(combuff==NULL) { free(lcdata); fclose(f); return 1; }
    fread(combuff,1,buffsize,f);
    { int bpos; int cpos; int repn; unsigned char repc;
      int totalsize=lcwidth*lcheight*lcframes;
      repn=0; bpos=0; cpos=0; repc=0;
      while(cpos<buffsize || bpos<totalsize)
      { if(!repn)
        { if(combuff[cpos]==0xFF)
          {cpos++; repn=combuff[cpos]; cpos++; repc=combuff[cpos]; cpos++;}
          else { repn=1; repc=combuff[cpos]; cpos++; } }
        else { repn--; if(bpos<totalsize) { lcdata[bpos]=repc; bpos++; } }
    } }
    free(combuff);
    fread(&lcfleft,1,4,f);
    fread(&lcfright,1,4,f);
    fread(&lcfup,1,4,f);
    fread(&lcfdown,1,4,f);
    for(i=0;i<4;i++)
    { fread(&sln,1,4,f); k=0; fread(decom,1,sln,f); sln--;
      for(j=0;j<sln;j=j)
      { scrops[i].list[k]=decom[j]; j++; l=j;
        while(decom[l]!='F' && decom[l]!='W' && l<sln) l++;
        cp=decom[l]; decom[l]=0;
        squant[i].quan[k]=atoi(decom+j);
        j=l; decom[j]=cp; k++;
      }
      scrops[i].list[k]=255;
  } }

  fclose(f);
  free(cdata);

  cdata=lcdata; cframes=lcframes;
  cheight=lcheight; cwidth=lcwidth;
  chotx=lchotx; choty=lchoty;
  chotw=lchotw; choth=lchoth;
  cfup=lcfup; cfright=lcfright; cfdown=lcfdown; cfleft=lcfleft;

  cfrmsize=cwidth*cheight;
  cdtused=cframes*cfrmsize;
  cdtalloc=cdtused+cfrmsize+1;
  cfront=cfname;
  i=strlen(cfname);
  while(i)
  { i--; if(cfname[i]=='\\' || cfname[i]=='/') { cfront=cfname+i+1; i=0; }}

  edframe=0;
  edmode=0;
  if(cheight>sh-5) { edmode=1; }
  if(cwidth>(sw/3)) { edmode=1; }
  ecleft=89+((cwidth+1)*(edmode^1));

  if(!zoomcfg)
  { int zx,zy;
    zoom=0; zx=0; zy=0;
    for(i=0;i<4;i++)
    { if((cheight*(1<<i))<(sh-10)) zy=i; } /* always get max zoom. */
    for(i=0;i<4;i++)
    { if((ecleft+(cwidth*(1<<i)))<(sw-10)) zx=i; }
    if(zx>=zy) { zoom=zy; } else { zoom=zx; }
  }

  ecxmax=(cwidth*(1<<zoom))+ecleft;
  ecymax=cheight*(1<<zoom);
  scrollxon=0;
  scrollyon=0;

  if(cheight*(1<<zoom)>(sh-11)) /* large sprites need scrolling */
  { scroffy=0;
    scrollyon=1;
    ecymax=sh-11; }
  if((ecleft+(cwidth*(1<<zoom)))>(sw-11))
  { scroffx=0;
    scrollxon=1;
    ecxmax=sw-11; }

  if(scrollxon) { ecxmax-=(ecxmax-ecleft)%(1<<zoom); }
  if(scrollyon) { ecymax-=ecymax%(1<<zoom); }

  anion=1;
  if(edmode==1) anion=0;
  anistr=0;
  anifrm=0;
  aniprt=0;
  anitime=0;

  if(anion)
  { i=0;
    while(scrops[anistr].list[i]!='F' && scrops[anistr].list[i]!=255) i++;
    if(scrops[anistr].list[i]==255) anion=0; }
  if(anion)
  { if(scrops[anistr].list[aniprt]=='F')
    { anifrm=squant[anistr].quan[aniprt]; }
    else if(scrops[anistr].list[aniprt]=='W')
    { anitime=squant[anistr].quan[aniprt]; }
    else if(scrops[anistr].list[aniprt]==255)
    { aniprt=0; }
    aniprt++; }

return 0; }

int savechr(char *fnm, char chrtmtype) /* saves as v2 or v1 */
{ long int wdword;
  short int wword;
  char wchar;
  FILE *f;
  int i,j,k;
  int scrlen; char scrtx[2048]; /* 2k script buffer. */

  f=fopen(fnm,"wb");
  if(f==NULL) return 1; /* Yech, bad filename */

  if(chrtmtype==1) /* Save as V1 (programmer's note: easy code!) */
  { for(i=0;i<cframes;i++) /* make sure they can save a 20 framer */
    { fwrite(cdata+(i*cwidth*cheight),1,cwidth*cheight,f); } }
  /* It'll save the larger frames if they're foolish enough as well. :P */
  else /* Ahh, the REAL one... */
  { unsigned char *combuff; int bufflen;
    wchar=2; fwrite(&wchar,1,1,f); /* version */
    wword=cwidth; fwrite(&wword,1,2,f);
    wword=cheight; fwrite(&wword,1,2,f);
    wword=chotx; fwrite(&wword,1,2,f);
    wword=choty; fwrite(&wword,1,2,f);
    wword=chotw; fwrite(&wword,1,2,f);
    wword=choth; fwrite(&wword,1,2,f);
    wword=cframes; fwrite(&wword,1,2,f);
    combuff=malloc((cframes*cwidth*cheight)+1);
    if(combuff==NULL) { fclose(f); return 1; }
    { int bpos,cpos,byt,samect,totalen;
      totalen=cwidth*cframes*cheight;
      bufflen=0; bpos=0; cpos=0;
      do
      { byt=cdata[bpos]; bpos++; samect=1;
        while(samect<254 && bpos<totalen && byt==cdata[bpos])
        { samect++; bpos++; }
        if(samect==2 && byt != 0xFF)
        { combuff[cpos]=byt; cpos++; bufflen++; }
        if(samect==3 && byt != 0xFF)
        { combuff[cpos]=byt; combuff[cpos+1]=byt; cpos+=2; bufflen+=2; }
        if(samect>3 || byt == 0xFF)
        { combuff[cpos]=0xFF; combuff[cpos+1]=samect; cpos+=2; bufflen+=2; }
        combuff[cpos]=byt; cpos++; bufflen++;
      } while(bpos<totalen);
    }
    fwrite(&bufflen,1,4,f); fwrite(combuff,1,bufflen,f); free(combuff);

    wdword=cfleft; fwrite(&wdword,1,4,f);
    wdword=cfright; fwrite(&wdword,1,4,f);
    wdword=cfup; fwrite(&wdword,1,4,f);
    wdword=cfdown; fwrite(&wdword,1,4,f);

    for(i=0;i<4;i++)
    { scrlen=0; scrtx[0]=0; k=0;
      for(j=0;j<200;j++) /* find the actual length of the script */
      { if(scrops[i].list[j]==255) { scrlen=j; j=201; break; } }
      for(j=0;j<scrlen;j++)
      { scrtx[k]=scrops[i].list[j]; k++; /* Always F or W */
        itoa(squant[i].quan[j],scrtx+k,10);
        k+=strlen(scrtx+k); } /* hopefully order of ops is okay here. */
      scrlen=strlen(scrtx)+1; /* I could just use k+1, but... meh... */
      wdword=scrlen; fwrite(&wdword,1,4,f);
      fwrite(scrtx,1,scrlen,f);
  } }
  fclose(f);
return 0; }

/*--- Prototypes, mainly for the module file */
char chreditor();
char gpclosebox();
void gpnewchr();
void scrdani();
void scripted();
void v2cescreensaver();
void gpabout();
void pickframe();

/* --- Main, setup and error handling --- */
char main(int argc, char **argv)
{ char ercd;
  int i;
  char loadon,loadfile[76];

  loadon=0;
  loadfile[0]=0;
  stpvmd=1;
  backupon=1;
  strcpy(backupfile,"$BACK$.CHR");
  strcpy(cfname,"DEFAULT.CHR");
  cfront=cfname;
  i=strlen(cfname);
  while(i)
  { i--;
    if(cfname[i]=='\\' || cfname[i]=='/')
    { cfront=cfname+i+1;
      i=0; }
  }

  parseconfig();

  for(i=1;i<argc;i++)
  { if(argv[i][0]=='-')
    { if(!stricmp(argv[i],"-v13h"))
      { stpvmd=1; }
      else if(!stricmp(argv[i],"-vmodex"))
      { stpvmd=2; }
      else if(!stricmp(argv[i],"-vvesa"))
      { stpvmd=3; }
      else if(!stricmp(argv[i],"-bon"))
      { backupon=1; }
      else if(!stricmp(argv[i],"-boff"))
      { backupon=0; }
      else if(!strnicmp(argv[i],"-bf",3))
      { strcpy(backupfile,argv[i]+3); }
      // Consider an ELSE use spawned maped mode
    }
    else
    { loadon=1;
      strcpy(loadfile,argv[i]); }
  }

  if((stpvmd>3) || !stpvmd) stpvmd=1;

  if((chrtdef.frames>1024) || !(chrtdef.frames)) chrtdef.frames=30;
  if((chrtdef.height>500) || !(chrtdef.height)) chrtdef.height=32;
  if((chrtdef.width>500) || !(chrtdef.width)) chrtdef.width=16;
  /* let's set a few maximums here, we need limits somewhere... */

  cdata=NULL;
  cframes=0; cheight=0; cwidth=0; chotx=0; choty=0; chotw=0; choth=0;
  for(i=0;i<8;i++)
  { scrops[i].list[0]=255;   // 255 is considered a terminating operation
    squant[i].quan[0]=0;   } // unnecessary

  cframes=chrtdef.frames;
  cheight=chrtdef.height;
  cwidth=chrtdef.width;
  chotx=chrtdef.hotspotx;
  choty=chrtdef.hotspoty;
  chotw=chrtdef.hotspotwidth;
  choth=chrtdef.hotspotheight;

  cfrmsize=cwidth*cheight;
  cdtused=cframes*cfrmsize;
  cdtalloc=cdtused+cfrmsize+1;
  if(cdtalloc>20971520)
  { cdata=NULL;
    printf("Target character is too large.\n");
    printf("Reduce the size to less than 20MB.\n");
    exit(0x03); }
  cdata=calloc(cdtalloc,1);

/*  for(i=0;i<cdtalloc;i++) 
  { cdata[i]=((((i/cwidth)+64)&255)+(((i%cwidth)+64)&255)); } */

  if(cdata==NULL)
  { printf("Not enough memory. Aborting program.\n");
    exit(0x02); }
  cdtalloc=cdtused+cfrmsize;

  if(loadon)
  { strcpy(cfname,loadfile); loadchr(cfname); }

  clipboard=malloc(cfrmsize*3);
  if(clipboard==NULL)
  { free(cdata);
    printf("Not enough memory. Aborting program.\n");
    exit(0x02); }
  tempchr=clipboard+cfrmsize;
  undo=tempchr+cfrmsize;
  memcpy(undo,cdata,cfrmsize); /* preload the first frame */

  InitVideo(stpvmd);
  cls();
  getpal(syspal);
  setpal(edpal);

  fontfuncs();

  ercd=chreditor();

  setpal(syspal);
  SDVideo();

  free(cdata);
  free(clipboard);

  printf("V2 CHR graphic editor finished.\n");
return ercd; }

/* --- Functions --- */

/* this fill function was written entirely by Brad Smith. The only help with it */
/* was unwittingly given by Ric, when I noticed the word recursive in his code. */
/* That word solved all my thinking problems, and enabled me to write this, a */
/* rather nifty fill algorythm, if I can say so myself. I don't know how others */
/* have done this in the past, but this one works. I couldn't sort through Ric's */
/* fill code becuase it was full of rather annoying TEFRblahblahs... */
void fill(unsigned char *bmp, int x, int y, int bw, int bh, unsigned char fc, unsigned char rc)
{ int lx,rx;
  int epx,kpx,epy,kpy;
  char eps,lpf;
  int i;
  unsigned char *tb;

  if(rc==fc) { return; } /* Idiot calls like that could screw this up. */

 confill: /* loops to prevent unnecessary recursions. */
  tb=bmp+(y*bw); /* It's a little faster this way. */

  /* this goes in horizontal lines. */
  lx=x; /* start from the given point. */
  while(tb[lx]==rc && lx>=0)
  { tb[lx]=fc; lx--; }
  lx++; /* we now have the left boundary */

  rx=x+1; /* now for the right... */
  while(tb[rx]==rc && rx<bw)
  { tb[rx]=fc; rx++; }
  rx--;

  /* now that our horizontal line is established, we first check the */
  /* upper expanse for entry points, then the lower, when a second is */
  /* discovered, we recurse to the first, then continue on. */

  eps=0; /* current number of entrypoints. */
  if(y) /* We don't want to scan off the image... */
  { tb=bmp+((y-1)*bw);
    for(i=lx;i<=rx;i++)
    { if(tb[i]==rc && eps==0) /* if we've found the first... */
      { eps=1; epx=i; epy=y-1; }
      else if(tb[i]==rc && eps==1)
      { if(tb[i-1]!=rc) /* if this one isn't attached... */
        { eps=2; kpx=i; kpy=y-1;
          fill(bmp,epx,epy,bw,bh,fc,rc); /* recurse at a second entry. */
          epx=kpx; epy=kpy; eps=1; }
  } } }

  lpf=0; /* when going downwards, I need to check for original points. */
  if(y<(bh-1))
  { tb=bmp+((y+1)*bw);
    for(i=lx;i<=rx;i++)
    { if(tb[i]==rc && eps==0)
      { eps=1; epx=i; epy=y+1; lpf=1; }
      else if(tb[i]==rc)
      { if(tb[i-1]!=rc || lpf==0)
        { eps=2; kpx=i; kpy=y+1;
          fill(bmp,epx,epy,bw,bh,fc,rc);
          epx=kpx; epy=kpy; eps=1; }
  } } }
  if(eps==1)      /* if there's still an entry remaining... */
  { x=epx; y=epy; /* move, and continue, rather than recursion. */
    goto confill; }

return; }

void drawed()
{ int i,j,k,l;
  char strc[30];
  l=1<<zoom;

  if(!scrollxon && !scrollyon)
  { for(j=0;j<cheight;j++)
    { for(i=0;i<cwidth;i++)
      { box(ecleft+(i*l),j*l,
            ecleft+(i*l)+l-1,(j*l)+l-1,
            cdata[(edframe*cfrmsize)+(j*cwidth)+i]);
      }
    }
    if(l>1 && gridon)
    { for(j=0;j<cheight;j++)
      { hline(ecleft,j*l,ecxmax,gcols[0]); }
      for(i=0;i<cwidth;i++)
      { vline(ecleft+(i*l),0,ecymax,gcols[0]); }
    }
  }
  else
  { int xfw,yfw,rj,ri;
    if(!scrollxon) scroffx=0;
    if(!scrollyon) scroffy=0;
    xfw=(ecxmax-ecleft)>>zoom;
    yfw=ecymax>>zoom;
    for(j=0;j<yfw;j++)
    { for(i=0;i<xfw;i++)
      { rj=j+scroffy;
        ri=i+scroffx;
        if(rj<cheight && ri<cwidth)
        { box(ecleft+(i*l),j*l,
          ecleft+(i*l)+l-1,(j*l)+l-1,
          cdata[(edframe*cfrmsize)+(rj*cwidth)+ri]);
        }
      }
    }
    if(l>1 && gridon)
    { for(j=0;j<yfw;j++)
      { hline(ecleft,j*l,ecxmax,gcols[0]); }
      for(i=0;i<xfw;i++)
      { vline(ecleft+(i*l),0,ecymax,gcols[0]); }
    }
  }

  vline(ecleft-1,0,ecymax,gcols[6]);
  vline(ecxmax,0,ecymax,gcols[5]);
  hline(ecleft,ecymax,ecxmax,gcols[5]);
  plot(ecxmax,ecymax,gcols[8]);
  plot(ecleft-1,ecymax,gcols[3]);

  if(scrollxon || scrollyon)
  { int bi,bk,bm,bn;
    if(scrollxon)
    { guidrawbc(ecleft+8,sh-11,sw-22,sh-1,"");
      guidrawb(ecleft-1,sh-11,ecleft+8,sh-1,"Ü");
      guidrawb(sw-22,sh-11,sw-12,sh-1,"Ý");
      bi=(ecxmax-ecleft)>>zoom;      /* displayed */
      bk=(sw-ecleft)-31;             /* screenspace */
      bm=(bi*bk)/cwidth;             /* width of the control */
      bn=(scroffx*bk)/cwidth;        /* left side of control */
      bn+=ecleft+9;
      guidrawb(bn,sh-11,bn+bm,sh-1,"");
      scrbx1=bn;
      scrbx2=bn+bm;
      scrxmax=cwidth-((ecxmax-ecleft)>>zoom);
    }
    if(scrollyon)
    { guidrawbc(sw-11,10,sw-1,sh-22,"");
      guidrawb(sw-11,0,sw-1,10,"Þ");
      guidrawb(sw-11,sh-22,sw-1,sh-12,"ß");
      bi=ecymax>>zoom;
      bk=sh-33;
      bm=(bi*bk)/cheight;
      bn=(scroffy*bk)/cheight;
      bn+=11;
      guidrawb(sw-11,bn,sw-1,bn+bm,"");
      scrby1=bn;
      scrby2=bn+bm;
      scrymax=cheight-(ecymax>>zoom);
    }
  }
  guidrawb(sw-11,sh-11,sw-1,sh-1,itoa(zoom,strc,10));
return; }

void drawpal()
{ int i,j,l;
  int tty,ttx;
  int xic,yic;
  char numstr[6];

  tty=(sh-(4<<4))-1;
  ttx=(4<<4);

  hline(0,tty,ttx,gcols[6]);
  vline(ttx,tty,sh-1,gcols[5]);
  plot(ttx,tty,gcols[3]);
  tty++;

  for(j=0;j<16;j++)
  { for(i=0;i<16;i++)
    { box((i<<2),(j<<2)+tty,(i<<2)+3,(j<<2)+(tty+3),i+(j<<4)); }
  }

  guidrawb( 0,tty-12,31,tty-2,"");
  guidrawb(33,tty-12,64,tty-2,"");
  box( 1,tty-11,30,tty-3,ecoll);
  box(34,tty-11,63,tty-3,ecolr);
  xic=ecoll&15; yic=ecoll>>4;
  hline(xic<<2,(yic<<2)+tty,(xic<<2)+3,255);
  vline(xic<<2,(yic<<2)+tty,(yic<<2)+3+tty,255);
  xic=ecolr&15; yic=ecolr>>4;
  hline(xic<<2,(yic<<2)+tty+3,(xic<<2)+3,0);
  vline((xic<<2)+3,(yic<<2)+tty,(yic<<2)+3+tty,0);
  blitfont(2,tty-10,itoa(ecoll,numstr,10),ecoll+4);
  blitfont(35,tty-10,itoa(ecolr,numstr,10),ecolr+4);
return; }

void drawani()
{ int i,j;
  if(anion && (anifrm<cframes) )
  { for(j=0;j<cheight;j++)
    { for(i=0;i<cwidth;i++)
      { plot(87+i,0+j,cdata[(anifrm*cfrmsize)+(j*cwidth)+i]); }
  } }
  else box(87,0,cwidth+86,cheight-1,gcols[4]);
  vesabuffer();
return; }

void eddrwscr()
{ int i,j,l;
  char strc[30];

  cls();

  // edmode 0 is normal
  // edmode 1 is without animation

/* MODIFICATION 1.2: Increases the box size. */
//  box(0,0,83,111,gcols[3]);
  box(0,0,83,122,gcols[3]);
  vline(84,0,122,gcols[5]);
  hline(0,122,84,gcols[5]);
  hline(0,122,84,gcols[5]);
  plot(84,122,gcols[8]);

  drawpal();

  guidrawb( 1, 1,40,11,"Close");
  guidrawb(42, 1,82,11,"About");
  guidrawb( 1,13,40,23,"New");
  guidrawb(42,13,82,23,drawmode?"Fill":"Draw");

  guidrawb( 1,25,28,35,"Load");
  guidrawb(55,25,82,35,"Save");
  guidrawb(30,25,53,35,"QSv");
  guidrawbc( 1,37,82,47,cfront);   
//  guidrawb( 1,49,82,59,"CHR Status");
  guidrawb( 1,49,41,59,"Stats");
  guidrawb(43,49,82,59,"Undo");

//  guidrawb(1,50,82,60,clipstattxt[clipstats]);
  guidrawb( 1,62,41,72,"Frame");
  guidrawb(43,62,82,72,"Shape");
  if(clipstats) { guidrawb( 1,74,41,84,"Paste");
                  guidrawb(43,74,82,84,"Mask"); }
  else
  { guidrawbd( 1,74,41,84,"Paste");
    guidrawbd(43,74,82,84,"Mask"); }

  itoa(edframe,strc,10);
  i=strlen(strc);
  strc[i]='/'; i++;
  strc[i]=0;
  itoa(cframes,strc+i,10);
  guidrawb(1,87,82,97,strc);
  guidrawb(1,99,10,109,"<");
  guidrawb(73,99,82,109,">");
  guidrawb(12,99,71,109,"Scripts");

  guidrawbc(1,111,82,120,""); // MODIFY 1.2: x,y box.

  l=1<<zoom;

  if(!edmode)
  { if(cheight<sh) guidrawb(86,0,87+cwidth,cheight,"");
    else if (cheight==sh) guidrawb(86,0,87+cwidth,sh,"");
    drawani(); }
  drawed();

  vesabuffer();
return; }

void cehold()
{ mseblit(mousex,mousey);
  while(mousek)
  { msepoll();
    if(mousex!=mox || mousey!=moy)
    { msebolt(mox,moy);
      mseblit(mousex,mousey);
  } }
  msebolt(mousex,mousey);
return; }

void ceoff()
{ char b=mousek;
  mox=mousex;
  moy=mousey;
  mseblit(mox,moy);
  while(mousek==b && mousex==mox && mousey==moy) { msepoll(); }
  msebolt(mox,moy);
return; }

void gpasbfc(int x,int y,char *strng, unsigned char col)
{ int leng;
  leng=strlen(strng)*6;
  leng=leng>>1;
  blitfont(x-leng,y,strng,col);
  vesabuffer();
return; }

void bfons(int x, int y, char* c)
{ blitfont(x+1,y+1,c,gcols[1]); blitfont(x,y,c,gcols[0]); return;
  vesabuffer(); }
#define nboxwid 220
#define nboxhig 80
void ncfieldedit(int x1, int y1, int x2, int y2, char *tex, int len, int n)
{ unsigned char c=0;
  char pos=0;
  memset(tex,0,len+2); /* make sure to fill it with zeroes */
  itoa(n,tex,10); /* put the number in there */
  guidrawbc(x1,y1,x2,y2,"");
  pos=strlen(tex);
  tex[pos]='_';
  blitfont(x1+2,y1+2,tex,gcols[0]);
  vesabuffer();
  cehold();
  while(c!=13)
  { c=getch();
    if(c==0) getch();
    else if(c==8)
    { if(pos) { tex[pos]=0; pos--; tex[pos]='_'; }
      guidrawbc(x1,y1,x2,y2,"");
      blitfont(x1+2,y1+2,tex,gcols[0]);
      vesabuffer(); }
    else if(c>='0' && c<='9' || c=='-') /* handy ascii limiting trick */
    { if(pos<len)
      { tex[pos]=c; pos++; tex[pos]='_'; tex[pos+1]=0;}
      guidrawbc(x1,y1,x2,y2,"");
      blitfont(x1+2,y1+2,tex,gcols[0]);
      vesabuffer(); }
    else if(c==27)
    { c=13; itoa(n,tex,10); /* return to original on ESC */ }
  }
  tex[pos]=0;
return; }

int fileselect(char *mask, int mode, char *filen);
extern void gpchrstat();
extern void gpexportpcx();

char handleclick(int k, int x, int y)
{ if(x>1 && x<82 && y>1 && y<111) // drawing panel
  { if(x<40 && y<11)
    { guidrawbc(1,1,40,11,"Close");
      vesabuffer();
      cehold();
      if(gpclosebox())
      { eddrwscr();
        return 0; }
      return 1; } // Close program
    else if(x>42 && y<11)
    { guidrawbc(42,1,82,11,"About");
      vesabuffer();
      cehold();
      gpabout();
      return 0; }
    else if(x>42 && y>13 && y<23)
    { guidrawbc(42,13,82,23,drawmode?"Fill":"Draw");
      if(!drawmode) { drawmode=1; } else { drawmode=0; }
      vesabuffer();
      cehold();
      eddrwscr();
    return 0; }
    else if(y>49 && y<59 && x<41)
    { guidrawbc(1,49,41,59,"Stats");
      vesabuffer();
      cehold(); gpchrstat(); eddrwscr(); return 0; }
    else if(y>49 && y<59 && x>43)
    { guidrawbc(43,49,82,59,"Undo");
      vesabuffer();
      cehold();
      memcpy(tempchr,undo,cfrmsize);
      memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);
      memcpy(cdata+(edframe*cfrmsize),tempchr,cfrmsize);
      eddrwscr(); return 0; }
    else if(x<40 && y>13 && y<23)
    { guidrawbc(1,13,40,23,"New");
      vesabuffer();
      cehold();
      gpnewchr();
      eddrwscr();
    return 0; }
    else if(x<28 && y>26 && y<35)
    { char fnm[20]; char cttemp;
      guidrawbc( 1,25,28,35,"Load");
      vesabuffer();
      cehold(); cttemp=chrtype;
      strcpy(fnm,cfront);
      if(!fileselect("*.CHR",1,fnm))
      { int oldwidth,oldheight; oldwidth=cwidth; oldheight=cheight;
        if(!loadchr(fnm))
        { strcpy(cfront,fnm);
          if(oldwidth!=cwidth || oldheight!=cheight)
          { free(clipboard);
            clipboard=malloc(cfrmsize*3); clipstats=0;
            if(clipboard==NULL)
            { free(cdata); setpal(syspal);  SDVideo();
              printf("Not enough memory. Aborting program.\n");
              exit(0x02); }
          tempchr=clipboard+cfrmsize; 
          undo=tempchr+cfrmsize; }
          memcpy(undo,cdata,cfrmsize); /* preload the first frame */
        }
        else
        { guisysgrey();
  guidrawb(10,10,sw-11,20,"Unable to load specified file. Press a key..."); 
          if(!getch()) getch(); }
      }
      eddrwscr(); chrtype=cttemp;
    }
    else if(x>30 && x<53 && y>25 && y<35)
    { char fnm[20];
      guidrawbc(30,25,53,35,"QSv");
      vesabuffer();
      cehold();
      strcpy(fnm,cfront); chrtype=0;
      if(savechr(fnm,chrtype))
      { guisysgrey();
        guidrawb(10,10,sw-11,20,"Unable to save file. Press a key..."); 
        vesabuffer();
        if(!getch()) getch(); eddrwscr(); }
      guidrawb(30,25,53,35,"QSv");
      vesabuffer();
    }
    else if(x>55 && y>25 && y<35)
    { char fnm[20]; chrtype=0;
      guidrawbc(55,25,82,35,"Save");
      vesabuffer();
      cehold();
      strcpy(fnm,cfront);
      if(!fileselect("*.CHR",0,fnm))
      { if(!savechr(fnm,chrtype))
        { strcpy(cfront,fnm); }
        else { guisysgrey();
          guidrawb(10,10,sw-11,20,"Unable to save file. Press a key..."); 
          vesabuffer();
          if(!getch()) getch(); }
      }
    eddrwscr(); }
    else if(x<10 && y>99 && y<109)
    { guidrawbc(1,99,10,109,"<");
      vesabuffer();
      cehold();
      if(edframe) edframe--;
      memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);
      eddrwscr(); }
    else if(x>73 && y>99 && y<109)
    { guidrawbc(73,99,82,109,">");
      vesabuffer();
      cehold();
      if(edframe<(cframes-1)) edframe++;
      memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);
      eddrwscr();
    return 0; }
    else if(x<41 && y>62 && y<72)
    { int i,j;
      guidrawbc( 1,62,41,72,"Frame");
      for(j=0;j<cheight;j++) { for(i=0;i<cwidth;i++)
      { clipboard[i+(j*cwidth)]=cdata[(edframe*cfrmsize)+i+(j*cwidth)]; }}
      clipstats=1; /* frame */
      vesabuffer();
      cehold();
      guidrawb( 1,62,41,72,"Frame");
//      guidrawb(1,50,82,60,clipstattxt[clipstats]);
      guidrawb( 1,74,41,84,"Paste");
      guidrawb(43,74,82,84,"Mask");
      vesabuffer();
    return 0; }
    else if(x>43 && y>62 && y<72)
    { int x1,x2,y1,y2,csx;
      char locato[20];
      guidrawbc(43,62,82,72,"Shape");
      vesabuffer();
        guisysgrey();
        drawed();
        guidrawb(1,1,82,34,"Pick Corners");
        guidrawbc(3,10,80,20,"");
        guidrawbc(3,22,80,32,"");

        vesabuffer();
        csx=2; // Counts times through mouse loop.
mousecutloop:
        msepoll();
        mox=mousex; moy=mousey;
        mseblit(mousex,mousey);
        while(!kbhit() && !mousek)
        { msepoll();
          if(mousex!=mox || mousey!=moy)
          { msebolt(mox,moy);
            guidrawbc(1,111,82,120,""); // x,y box.
            if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
            { int dx,dy;
              char nums[25];
              dx=mousex-ecleft;
              dy=mousey;
              dx=dx>>zoom;
              dy=dy>>zoom;
              dx+=scroffx;
              dy+=scroffy;
              itoa(dx,nums,10);
              nums[strlen(nums)+1]=0;
              nums[strlen(nums)]=',';
              itoa(dy,nums+strlen(nums),10);
              blitfont(3,113,nums,gcols[0]);
            }
            mseblit(mousex,mousey);
          }
        }
        msebolt(mousex,mousey);
        x=mousex; y=mousey;

        while(kbhit()) { getch(); }

        if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
        { int dx,dy;
          dx=mousex-ecleft;
          dy=mousey;
          dx=dx>>zoom;
          dy=dy>>zoom;
          dx+=scroffx;
          dy+=scroffy;
          if(csx==2)
          { x1=dx; y1=dy;
            csx=1;
            itoa(x1,locato,10);
            dx=strlen(locato);
            locato[dx]=',';
            itoa(y1,locato+dx+1,10);
            guidrawbc(3,10,80,20,locato);
            cehold();
          }
          else if(csx==1)
          { x2=dx; y2=dy;
            csx=0;
            itoa(x2,locato,10);
            dx=strlen(locato);
            locato[dx]=',';
            itoa(y2,locato+dx+1,10);
            guidrawbc(3,22,80,32,locato);
            cehold();
          }
        }

        else if(mousex>sw-11 && mousex<sw-1 && mousey>sh-11 && mousey<sh-1)
        { char hydrochloric[3];
          guidrawbc(sw-11,sh-11,sw-1,sh-1,itoa(zoom,hydrochloric,10));
          if(mousek&1) { if(zoom<3) zoom++; }
          else if(mousek&2) { if(zoom) zoom--; }
          scrollxon=0; scrollyon=0; scroffx=0; scroffy=0;
          ecxmax=(cwidth*(1<<zoom))+ecleft; ecymax=cheight*(1<<zoom);
          if(cheight*(1<<zoom)>(sh-11))
          { scroffy=0;
            scrollyon=1;
            ecymax=sh-11; }
          if((ecleft+(cwidth*(1<<zoom)))>(sw-11))
          { scroffx=0;
            scrollxon=1;
            ecxmax=sw-11; }
          if(scrollxon) { ecxmax-=(ecxmax-ecleft)%(1<<zoom); }
          if(scrollyon) { ecymax-=ecymax%(1<<zoom); }
          vesabuffer();
          cehold();
          eddrwscr();
          guisysgrey();
          drawed();
          locato[0]=0;
          if(csx==1)
          { int poo;
            itoa(x1,locato,10);
            poo=strlen(locato);
            locato[poo]=',';
            itoa(y1,locato+poo+1,10);
          }
          guidrawb(1,1,82,34,"Pick Corners");
          guidrawbc(3,10,80,20,locato);
          guidrawbc(3,22,80,32,"");
          vesabuffer();
        }
 
        else if(scrollxon && x>scrbx1 && x<scrbx2 && y>sh-11 && y<sh-1)
        { int bi,bk,bm,bn,bl,br,bx,bz;
          int moff;
          guidrawbc(scrbx1,sh-11,scrbx2,sh-1,"");
          bi=(ecxmax-ecleft)>>zoom;
          bk=(sw-ecleft)-31;
          bm=(bi*bk)/cwidth;
          bn=(scroffx*bk)/cwidth;
          bn+=ecleft+9;
          scrxmax=cwidth-((ecxmax-ecleft)>>zoom);
 
          bl=bn; br=bn+bm; bx=(sw-23)-bm; bz=ecleft+9;
          moff=x-scrbx1;
 
          vesabuffer();
          while(mousek)
          { msepoll();
            if(mousex!=mox)
            { hline(ecleft+9,sh-11,sw-23,gcols[5]);
              hline(ecleft+9,sh-1,sw-23,gcols[6]);
              box(ecleft+9,sh-10,sw-23,sh-2,gcols[4]);
              bl=mousex-moff;
              if(bl<bz) { bl=bz; moff=mousex-bl; }
              if(bl>bx) { bl=bx; moff=mousex-bl; }
              guidrawbc(bl,sh-11,bl+bm,sh-1,"");
              vesabuffer();
            }
          }
          guidrawb(bl,sh-11,bl+bm,sh-1,""); 
          scroffx=((bl-(ecleft+9))*cwidth)/bk;
          if(bl==bx) scroffx=scrxmax;
          else if(scroffx>scrxmax) scroffx=scrxmax;
          if(scroffx<0) scroffx=0;
          eddrwscr();
          guisysgrey();
          drawed();
          locato[0]=0;
          if(csx==1)
          { int poo;
            itoa(x1,locato,10);
            poo=strlen(locato);
            locato[poo]=',';
            itoa(y1,locato+poo+1,10);
          }
          guidrawb(1,1,82,34,"Pick Corners");
          guidrawbc(3,10,80,20,locato);
          guidrawbc(3,22,80,32,"");
          vesabuffer();
        }
        else if(scrollyon && y>scrby1 && y<scrby2 && x>sw-11 && x<sw-1)
        { int bi,bk,bm,bn,bl,br,bx,bz;
          int moff;
          guidrawbc(sw-11,scrby1,sw-1,scrby2,"");
          vesabuffer();
          bi=ecymax>>zoom;
          bk=sh-33;
          bm=(bi*bk)/cheight;
          bn=(scroffy*bk)/cheight;
          bn+=11;
          scrymax=cheight-(ecymax>>zoom);

          bl=bn; br=bn+bm; bx=(sh-23)-bm; bz=11;
          moff=y-scrby1;

          while(mousek)
          { msepoll();
            if(mousey!=moy)
            { vline(sw-11,11,sh-23,gcols[5]);
              vline(sw-1,11,sh-23,gcols[6]);
              box(sw-10,11,sw-2,sh-23,gcols[4]);
              bl=mousey-moff;
              if(bl<bz) { bl=bz; moff=mousey-bl; }
              if(bl>bx) { bl=bx; moff=mousey-bl; }
              guidrawbc(sw-11,bl,sw-1,bl+bm,"");
              vesabuffer();
            }
          }
          guidrawb(sw-11,bl,sw-1,bl+bm,"");
          scroffy=((bl-11)*cheight)/bk;
          if(bl==bx) scroffy=scrymax;
          else if(scroffy>scrymax) scroffy=scrymax;
          if(scroffy<0) scroffy=0;
          eddrwscr();
          guisysgrey();
          drawed();
          locato[0]=0;
          if(csx==1)
          { int poo;
            itoa(x1,locato,10);
            poo=strlen(locato);
            locato[poo]=',';
            itoa(y1,locato+poo+1,10);
          }
          guidrawb(1,1,82,34,"Pick Corners");
          guidrawbc(3,10,80,20,locato);
          guidrawbc(3,22,80,32,"");
          vesabuffer();
        }
        if(csx) goto mousecutloop;

        if(x2<x1) { int intex; intex=x2; x2=x1; x1=intex; }
        if(y2<y1) { int intey; intey=y2; y2=y1; y1=intey; }

        clipstats=2; /* shape */
        clipwidth=(x2-x1)+1;
        clipheight=(y2-y1)+1;

        for(x=x1;x<=x2;x++)
        { for(y=y1;y<=y2;y++)
          { clipboard[(x-x1)+((y-y1)*clipwidth)]
              = cdata[(edframe*cfrmsize)+x+(y*cwidth)];
        } }

        eddrwscr();
    return 0; }

    else if(x<41 && y>74 && y<84 && clipstats)
    { int i,j;
      guidrawbc( 1,74,41,84,"Paste");
      memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);
      if(clipstats==1)
      { for(j=0;j<cheight;j++) { for(i=0;i<cwidth;i++)
        { cdata[(edframe*cfrmsize)+i+(j*cwidth)]=clipboard[i+(j*cwidth)]; }}  
      } 
      else if (clipstats==2)
      { int x1,x2;
        cehold();
        vesabuffer();
        guisysgrey();
        drawed();

msepasteagain:

        memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
        msepoll();
        mox=mousex; moy=mousey;
            if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
            { int dx,dy;
              char nums[25];
              dx=mousex-ecleft;
              dy=mousey;
              dx=dx>>zoom;
              dy=dy>>zoom;
              dx+=scroffx;
              dy+=scroffy;
              itoa(dx,nums,10);
              nums[strlen(nums)+1]=0;
              nums[strlen(nums)]=',';
              itoa(dy,nums+strlen(nums),10);
              blitfont(3,113,nums,gcols[0]);
              for(j=0;j<clipheight;j++)
              { for(i=0;i<clipwidth;i++)
                { if((dx+i)<cwidth && (dy+j)<cheight)
                   cdata[(edframe*cfrmsize)+dx+i+((dy+j)*cwidth)]
                   = clipboard[i+(j*clipwidth)];
                }
              }
            }
        drawed();
        vesabuffer();
        mseblit(mousex,mousey);
        while(!kbhit() && !mousek)
        { msepoll();
          if(mousex!=mox || mousey!=moy)
          { msebolt(mox,moy);
            guidrawbc(1,111,82,120,""); // x,y box.
            if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
            { int dx,dy;
              char nums[25];
              dx=mousex-ecleft;
              dy=mousey;
              dx=dx>>zoom;
              dy=dy>>zoom;
              dx+=scroffx;
              dy+=scroffy;
              itoa(dx,nums,10);
              nums[strlen(nums)+1]=0;
              nums[strlen(nums)]=',';
              itoa(dy,nums+strlen(nums),10);
              blitfont(3,113,nums,gcols[0]);
              memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
              for(j=0;j<clipheight;j++)
              { for(i=0;i<clipwidth;i++)
                { if((dx+i)<cwidth && (dy+j)<cheight)
                   cdata[(edframe*cfrmsize)+dx+i+((dy+j)*cwidth)]
                   = clipboard[i+(j*clipwidth)];
                }
              }
            }
            else memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
            drawed();
            vesabuffer();
            mseblit(mousex,mousey);
          }
        }
        msebolt(mousex,mousey);
        x=mousex; y=mousey;

        while(kbhit()) { char poopy;
                         poopy=getch();
                         if(!poopy) getch();
                         else if (poopy==27)
                         { memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
                           eddrwscr(); return 0; }
                       }

        if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
        { int dx,dy;
          dx=mousex-ecleft;
          dy=mousey;
          dx=dx>>zoom;
          dy=dy>>zoom;
          dx+=scroffx;
          dy+=scroffy;
          goto msepastefin;
        }

        else if(mousex>sw-11 && mousex<sw-1 && mousey>sh-11 && mousey<sh-1)
        { char hydrochloric[3];
          guidrawbc(sw-11,sh-11,sw-1,sh-1,itoa(zoom,hydrochloric,10));
          if(mousek&1) { if(zoom<3) zoom++; }
          else if(mousek&2) { if(zoom) zoom--; }
          scrollxon=0; scrollyon=0; scroffx=0; scroffy=0;
          ecxmax=(cwidth*(1<<zoom))+ecleft; ecymax=cheight*(1<<zoom);
          if(cheight*(1<<zoom)>(sh-11))
          { scroffy=0;
            scrollyon=1;
            ecymax=sh-11; }
          if((ecleft+(cwidth*(1<<zoom)))>(sw-11))
          { scroffx=0;
            scrollxon=1;
            ecxmax=sw-11; }
          if(scrollxon) { ecxmax-=(ecxmax-ecleft)%(1<<zoom); }
          if(scrollyon) { ecymax-=ecymax%(1<<zoom); }
          vesabuffer();
          cehold();
          eddrwscr();
          guisysgrey();
          drawed();
          vesabuffer();
        }
 
        else if(scrollxon && x>scrbx1 && x<scrbx2 && y>sh-11 && y<sh-1)
        { int bi,bk,bm,bn,bl,br,bx,bz;
          int moff;
          guidrawbc(scrbx1,sh-11,scrbx2,sh-1,"");
          bi=(ecxmax-ecleft)>>zoom;
          bk=(sw-ecleft)-31;
          bm=(bi*bk)/cwidth;
          bn=(scroffx*bk)/cwidth;
          bn+=ecleft+9;
          scrxmax=cwidth-((ecxmax-ecleft)>>zoom);
 
          bl=bn; br=bn+bm; bx=(sw-23)-bm; bz=ecleft+9;
          moff=x-scrbx1;
 
          vesabuffer();
          while(mousek)
          { msepoll();
            if(mousex!=mox)
            { hline(ecleft+9,sh-11,sw-23,gcols[5]);
              hline(ecleft+9,sh-1,sw-23,gcols[6]);
              box(ecleft+9,sh-10,sw-23,sh-2,gcols[4]);
              bl=mousex-moff;
              if(bl<bz) { bl=bz; moff=mousex-bl; }
              if(bl>bx) { bl=bx; moff=mousex-bl; }
              guidrawbc(bl,sh-11,bl+bm,sh-1,"");
              vesabuffer();
            }
          }
          guidrawb(bl,sh-11,bl+bm,sh-1,""); 
          scroffx=((bl-(ecleft+9))*cwidth)/bk;
          if(bl==bx) scroffx=scrxmax;
          else if(scroffx>scrxmax) scroffx=scrxmax;
          if(scroffx<0) scroffx=0;
          eddrwscr();
          guisysgrey();
          drawed();
          vesabuffer();
        }
        else if(scrollyon && y>scrby1 && y<scrby2 && x>sw-11 && x<sw-1)
        { int bi,bk,bm,bn,bl,br,bx,bz;
          int moff;
          guidrawbc(sw-11,scrby1,sw-1,scrby2,"");
          vesabuffer();
          bi=ecymax>>zoom;
          bk=sh-33;
          bm=(bi*bk)/cheight;
          bn=(scroffy*bk)/cheight;
          bn+=11;
          scrymax=cheight-(ecymax>>zoom);

          bl=bn; br=bn+bm; bx=(sh-23)-bm; bz=11;
          moff=y-scrby1;

          while(mousek)
          { msepoll();
            if(mousey!=moy)
            { vline(sw-11,11,sh-23,gcols[5]);
              vline(sw-1,11,sh-23,gcols[6]);
              box(sw-10,11,sw-2,sh-23,gcols[4]);
              bl=mousey-moff;
              if(bl<bz) { bl=bz; moff=mousey-bl; }
              if(bl>bx) { bl=bx; moff=mousey-bl; }
              guidrawbc(sw-11,bl,sw-1,bl+bm,"");
              vesabuffer();
            }
          }
          guidrawb(sw-11,bl,sw-1,bl+bm,"");
          scroffy=((bl-11)*cheight)/bk;
          if(bl==bx) scroffy=scrymax;
          else if(scroffy>scrymax) scroffy=scrymax;
          if(scroffy<0) scroffy=0;
          eddrwscr();
          guisysgrey();
          drawed();
          vesabuffer();
        }
        else { eddrwscr(); return 0; }
        
        goto msepasteagain;

msepastefin:

        eddrwscr();
        cehold();
    return 0; }
      vesabuffer();
      cehold();
      drawed();
      vesabuffer();
    return 0; }
    else if(x>43 && y>74 && y<84 && clipstats)
    { int i,j; unsigned char m;
      guidrawbc(43,74,82,84,"Mask");
      memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);
      if(k&2) m=ecoll;
      else m=0;
      if(clipstats==1)
      { for(j=0;j<cheight;j++) { for(i=0;i<cwidth;i++)
        { if(clipboard[i+(j*cwidth)]!=m)
        { cdata[(edframe*cfrmsize)+i+(j*cwidth)]=clipboard[i+(j*cwidth)]; }
      }}}
      else if(clipstats==2)
      { int x1,x2;
        cehold();
        vesabuffer();
        guisysgrey();
        drawed();

msemaskagain:

        memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
        msepoll();
        mox=mousex; moy=mousey;
            if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
            { int dx,dy;
              char nums[25];
              dx=mousex-ecleft;
              dy=mousey;
              dx=dx>>zoom;
              dy=dy>>zoom;
              dx+=scroffx;
              dy+=scroffy;
              itoa(dx,nums,10);
              nums[strlen(nums)+1]=0;
              nums[strlen(nums)]=',';
              itoa(dy,nums+strlen(nums),10);
              blitfont(3,113,nums,gcols[0]);
              for(j=0;j<clipheight;j++)
              { for(i=0;i<clipwidth;i++)
                { if((dx+i)<cwidth && (dy+j)<cheight && clipboard[i+(j*clipwidth)]!=m)
                   cdata[(edframe*cfrmsize)+dx+i+((dy+j)*cwidth)]
                   = clipboard[i+(j*clipwidth)];
                }
              }
            }
        drawed();
        vesabuffer();
        mseblit(mousex,mousey);
        while(!kbhit() && !mousek)
        { msepoll();
          if(mousex!=mox || mousey!=moy)
          { msebolt(mox,moy);
            guidrawbc(1,111,82,120,""); // x,y box.
            if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
            { int dx,dy;
              char nums[25];
              dx=mousex-ecleft;
              dy=mousey;
              dx=dx>>zoom;
              dy=dy>>zoom;
              dx+=scroffx;
              dy+=scroffy;
              itoa(dx,nums,10);
              nums[strlen(nums)+1]=0;
              nums[strlen(nums)]=',';
              itoa(dy,nums+strlen(nums),10);
              blitfont(3,113,nums,gcols[0]);
              memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
              for(j=0;j<clipheight;j++)
              { for(i=0;i<clipwidth;i++)
                { if((dx+i)<cwidth && (dy+j)<cheight && clipboard[i+(j*clipwidth)]!=m)
                   cdata[(edframe*cfrmsize)+dx+i+((dy+j)*cwidth)]
                   = clipboard[i+(j*clipwidth)];
                }
              }
            }
            else memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
            drawed();
            vesabuffer();
            mseblit(mousex,mousey);
          }
        }
        msebolt(mousex,mousey);
        x=mousex; y=mousey;

        while(kbhit()) { char poopy;
                         poopy=getch();
                         if(!poopy) getch();
                         else if (poopy==27)
                         { memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
                           eddrwscr(); return 0; }
                       }

        if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
        { int dx,dy;
          dx=mousex-ecleft;
          dy=mousey;
          dx=dx>>zoom;
          dy=dy>>zoom;
          dx+=scroffx;
          dy+=scroffy;
          goto msemaskfin;
        }

        else if(mousex>sw-11 && mousex<sw-1 && mousey>sh-11 && mousey<sh-1)
        { char hydrochloric[3];
          guidrawbc(sw-11,sh-11,sw-1,sh-1,itoa(zoom,hydrochloric,10));
          if(mousek&1) { if(zoom<3) zoom++; }
          else if(mousek&2) { if(zoom) zoom--; }
          scrollxon=0; scrollyon=0; scroffx=0; scroffy=0;
          ecxmax=(cwidth*(1<<zoom))+ecleft; ecymax=cheight*(1<<zoom);
          if(cheight*(1<<zoom)>(sh-11))
          { scroffy=0;
            scrollyon=1;
            ecymax=sh-11; }
          if((ecleft+(cwidth*(1<<zoom)))>(sw-11))
          { scroffx=0;
            scrollxon=1;
            ecxmax=sw-11; }
          if(scrollxon) { ecxmax-=(ecxmax-ecleft)%(1<<zoom); }
          if(scrollyon) { ecymax-=ecymax%(1<<zoom); }
          vesabuffer();
          cehold();
          eddrwscr();
          guisysgrey();
          drawed();
          vesabuffer();
        }
 
        else if(scrollxon && x>scrbx1 && x<scrbx2 && y>sh-11 && y<sh-1)
        { int bi,bk,bm,bn,bl,br,bx,bz;
          int moff;
          guidrawbc(scrbx1,sh-11,scrbx2,sh-1,"");
          bi=(ecxmax-ecleft)>>zoom;
          bk=(sw-ecleft)-31;
          bm=(bi*bk)/cwidth;
          bn=(scroffx*bk)/cwidth;
          bn+=ecleft+9;
          scrxmax=cwidth-((ecxmax-ecleft)>>zoom);
 
          bl=bn; br=bn+bm; bx=(sw-23)-bm; bz=ecleft+9;
          moff=x-scrbx1;
 
          vesabuffer();
          while(mousek)
          { msepoll();
            if(mousex!=mox)
            { hline(ecleft+9,sh-11,sw-23,gcols[5]);
              hline(ecleft+9,sh-1,sw-23,gcols[6]);
              box(ecleft+9,sh-10,sw-23,sh-2,gcols[4]);
              bl=mousex-moff;
              if(bl<bz) { bl=bz; moff=mousex-bl; }
              if(bl>bx) { bl=bx; moff=mousex-bl; }
              guidrawbc(bl,sh-11,bl+bm,sh-1,"");
              vesabuffer();
            }
          }
          guidrawb(bl,sh-11,bl+bm,sh-1,""); 
          scroffx=((bl-(ecleft+9))*cwidth)/bk;
          if(bl==bx) scroffx=scrxmax;
          else if(scroffx>scrxmax) scroffx=scrxmax;
          if(scroffx<0) scroffx=0;
          eddrwscr();
          guisysgrey();
          drawed();
          vesabuffer();
        }
        else if(scrollyon && y>scrby1 && y<scrby2 && x>sw-11 && x<sw-1)
        { int bi,bk,bm,bn,bl,br,bx,bz;
          int moff;
          guidrawbc(sw-11,scrby1,sw-1,scrby2,"");
          vesabuffer();
          bi=ecymax>>zoom;
          bk=sh-33;
          bm=(bi*bk)/cheight;
          bn=(scroffy*bk)/cheight;
          bn+=11;
          scrymax=cheight-(ecymax>>zoom);

          bl=bn; br=bn+bm; bx=(sh-23)-bm; bz=11;
          moff=y-scrby1;

          while(mousek)
          { msepoll();
            if(mousey!=moy)
            { vline(sw-11,11,sh-23,gcols[5]);
              vline(sw-1,11,sh-23,gcols[6]);
              box(sw-10,11,sw-2,sh-23,gcols[4]);
              bl=mousey-moff;
              if(bl<bz) { bl=bz; moff=mousey-bl; }
              if(bl>bx) { bl=bx; moff=mousey-bl; }
              guidrawbc(sw-11,bl,sw-1,bl+bm,"");
              vesabuffer();
            }
          }
          guidrawb(sw-11,bl,sw-1,bl+bm,"");
          scroffy=((bl-11)*cheight)/bk;
          if(bl==bx) scroffy=scrymax;
          else if(scroffy>scrymax) scroffy=scrymax;
          if(scroffy<0) scroffy=0;
          eddrwscr();
          guisysgrey();
          drawed();
          vesabuffer();
        }
        else { eddrwscr(); return 0; }
        
        goto msemaskagain;

msemaskfin:

        eddrwscr();
        cehold();
    return 0; }
      vesabuffer();
      cehold();
      drawed();
      guidrawb(43,74,82,84,"Mask");
      vesabuffer();
    return 0; }
    else if(x>12 && x<71 && y>99 && y<109)
    { int i; guidrawbc(12,99,71,109,"Scripts");
      vesabuffer();
      cehold(); scripted();
      anifrm=cframes; aniprt=0; anitime=0; i=0;
      while(scrops[anistr].list[i]!='F' && scrops[anistr].list[i]!=255) i++;
      if(scrops[anistr].list[i]==255) anion=0;
      if(anion) { if(scrops[anistr].list[aniprt]=='F')
                  { anifrm=squant[anistr].quan[aniprt]; }
                  else if(scrops[anistr].list[aniprt]=='W')
                  { anitime=squant[anistr].quan[aniprt]; }
                  else if(scrops[anistr].list[aniprt]==255)
                  { aniprt=0; } aniprt++; drawani(); }
      eddrwscr();
    return 0; }
    else if(x>1 && x<82 && y>87 && y<97)
    { char strc[20]; int i;
      itoa(edframe,strc,10);
      i=strlen(strc); strc[i]='/'; i++;
      strc[i]=0; itoa(cframes,strc+i,10);
      guidrawbc(1,87,82,97,strc);
      vesabuffer();
      cehold();
      if(cwidth<(sw-5) && cheight<(sh-20)) pickframe();
      memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);
      eddrwscr();
    return 0; }
    else { cehold(); }
  return 0; }

  else if(x>sw-11 && x<sw-1 && y>sh-11 && y<sh-1)
  { char hydrochloric[3];
    guidrawbc(sw-11,sh-11,sw-1,sh-1,itoa(zoom,hydrochloric,10));
    if(k&1) { if(zoom<3) zoom++; }
    else if(k&2) { if(zoom) zoom--; }
    scrollxon=0; scrollyon=0; scroffx=0; scroffy=0;
    ecxmax=(cwidth*(1<<zoom))+ecleft; ecymax=cheight*(1<<zoom);
    if(cheight*(1<<zoom)>(sh-11))
    { scroffy=0;
      scrollyon=1;
      ecymax=sh-11; }
    if((ecleft+(cwidth*(1<<zoom)))>(sw-11))
    { scroffx=0;
      scrollxon=1;
      ecxmax=sw-11; }
    if(scrollxon) { ecxmax-=(ecxmax-ecleft)%(1<<zoom); }
    if(scrollyon) { ecymax-=ecymax%(1<<zoom); }
    vesabuffer();
    cehold();
    eddrwscr();
  }

  else if(x<64 && y>(sh-65)) // palette
  { int tx,ty;
    int tty;
    int xic,yic;
    char numstr[6];

    tty=sh-(4<<4);
    ty=y-tty;
    ty=ty>>2;
    tx=x>>2;
    tx=tx+(ty<<4);

    xic=ecoll&15; yic=ecoll>>4;
    hline(xic<<2,(yic<<2)+tty,(xic<<2)+3,ecoll);
    vline(xic<<2,(yic<<2)+tty,(yic<<2)+3+tty,ecoll);
    xic=ecolr&15; yic=ecolr>>4;
    hline(xic<<2,(yic<<2)+tty+3,(xic<<2)+3,ecolr);
    vline((xic<<2)+3,(yic<<2)+tty,(yic<<2)+3+tty,ecolr);

    if(k&1) ecoll=tx;
    if(k&2) ecolr=tx;

    xic=ecoll&15; yic=ecoll>>4;
    hline(xic<<2,(yic<<2)+tty,(xic<<2)+3,255);
    vline(xic<<2,(yic<<2)+tty,(yic<<2)+3+tty,255);
    xic=ecolr&15; yic=ecolr>>4;
    hline(xic<<2,(yic<<2)+tty+3,(xic<<2)+3,0);
    vline((xic<<2)+3,(yic<<2)+tty,(yic<<2)+3+tty,0);
    box( 1,tty-11,30,tty-3,ecoll);
    box(34,tty-11,63,tty-3,ecolr);
    blitfont(2,tty-10,itoa(ecoll,numstr,10),ecoll+4);
    blitfont(35,tty-10,itoa(ecolr,numstr,10),ecolr+4);
    vesabuffer();
    ceoff();
  return 0; }

  else if(x>=ecleft && x<ecxmax && y<ecymax)
  { int dx,dy,rx,ry,l;
    unsigned char eclr;
    char nums[25];
    l=1<<zoom;
    dx=x-ecleft;
    dy=y;
    dx=dx>>zoom;
    dy=dy>>zoom;
    rx=dx+(scrollxon*scroffx);
    ry=dy+(scrollyon*scroffy);
    if(shiftstats()&0x02) /* left shift = pick up color */
    { if(k&1) ecoll=cdata[(edframe*cfrmsize)+(ry*cwidth)+rx];
      if(k&2) ecolr=cdata[(edframe*cfrmsize)+(ry*cwidth)+rx];
      drawpal();
      vesabuffer();
      ceoff();
      return 0;}
    eclr=ecoll;
    if(k&2) eclr=ecolr;
    if(k&1) eclr=ecoll;
    memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize); /* pickup undo */
    if(drawmode)
    { fill(cdata+(edframe*cfrmsize),rx,ry,cwidth,cheight,
           eclr,cdata[(edframe*cfrmsize)+(ry*cwidth)+rx]);
      eddrwscr();
      cehold();
      return 0; }
    else
    { while(mousek && mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
      { l=1<<zoom;
        dx=mousex-ecleft;
        dy=mousey;
        dx=dx>>zoom;
        dy=dy>>zoom;
        rx=dx+(scrollxon*scroffx);
        ry=dy+(scrollyon*scroffy);
        itoa(dx,nums,10);
        nums[strlen(nums)+1]=0;
        nums[strlen(nums)]=',';
        itoa(dy,nums+strlen(nums),10);
        guidrawbc(1,111,82,120,""); // x,y box.
        blitfont(3,113,nums,gcols[0]);
        cdata[(edframe*cfrmsize)+(ry*cwidth)+rx]=eclr;
        box(ecleft+(dx*l),dy*l, ecleft+(dx*l)+l-1,(dy*l)+l-1,
            cdata[(edframe*cfrmsize)+(ry*cwidth)+rx]);    
        if(l>1 && gridon)
        { vline(ecleft+(dx*l),dy*l,(dy*l)+l-1,gcols[0]);
          hline(ecleft+(dx*l),dy*l,ecleft+(dx*l)+l-1,gcols[0]); }
        vesabuffer();
        ceoff();
      }
    return 0; }
  vesabuffer();
  ceoff(); }

  else if(scrollxon && x>scrbx1 && x<scrbx2 && y>sh-11 && y<sh-1)
  { int bi,bk,bm,bn,bl,br,bx,bz;
    int moff;
    guidrawbc(scrbx1,sh-11,scrbx2,sh-1,"");
    bi=(ecxmax-ecleft)>>zoom;
    bk=(sw-ecleft)-31;
    bm=(bi*bk)/cwidth;
    bn=(scroffx*bk)/cwidth;
    bn+=ecleft+9;
    scrxmax=cwidth-((ecxmax-ecleft)>>zoom);

    bl=bn; br=bn+bm; bx=(sw-23)-bm; bz=ecleft+9; /* left right max min */
    moff=x-scrbx1;

    vesabuffer();
    while(mousek)
    { msepoll();
      if(mousex!=mox)
      { hline(ecleft+9,sh-11,sw-23,gcols[5]);
        hline(ecleft+9,sh-1,sw-23,gcols[6]);
        box(ecleft+9,sh-10,sw-23,sh-2,gcols[4]);
        bl=mousex-moff;
        if(bl<bz) { bl=bz; moff=mousex-bl; } /* stop at maxes, nonstick */
        if(bl>bx) { bl=bx; moff=mousex-bl; }
        guidrawbc(bl,sh-11,bl+bm,sh-1,"");
        vesabuffer();
      }
    }
    guidrawb(bl,sh-11,bl+bm,sh-1,""); 
    scroffx=((bl-(ecleft+9))*cwidth)/bk;
    if(bl==bx) scroffx=scrxmax;
    else if(scroffx>scrxmax) scroffx=scrxmax;
    if(scroffx<0) scroffx=0;
    drawed();
    vesabuffer();
  }
  else if(scrollyon && y>scrby1 && y<scrby2 && x>sw-11 && x<sw-1)
  { int bi,bk,bm,bn,bl,br,bx,bz;
    int moff;
    guidrawbc(sw-11,scrby1,sw-1,scrby2,"");
    vesabuffer();
    bi=ecymax>>zoom;
    bk=sh-33;
    bm=(bi*bk)/cheight;
    bn=(scroffy*bk)/cheight;
    bn+=11;
    scrymax=cheight-(ecymax>>zoom);

    bl=bn; br=bn+bm; bx=(sh-23)-bm; bz=11;
    moff=y-scrby1;

    while(mousek)
    { msepoll();
      if(mousey!=moy)
      { vline(sw-11,11,sh-23,gcols[5]);
        vline(sw-1,11,sh-23,gcols[6]);
        box(sw-10,11,sw-2,sh-23,gcols[4]);
        bl=mousey-moff;
        if(bl<bz) { bl=bz; moff=mousey-bl; }
        if(bl>bx) { bl=bx; moff=mousey-bl; }
        guidrawbc(sw-11,bl,sw-1,bl+bm,"");
        vesabuffer();
      }
    }
    guidrawb(sw-11,bl,sw-1,bl+bm,"");
    scroffy=((bl-11)*cheight)/bk;
    if(bl==bx) scroffy=scrymax;
    else if(scroffy>scrymax) scroffy=scrymax;
    if(scroffy<0) scroffy=0;
    drawed();
    vesabuffer();
  }

  else if(!edmode && x>89 && x< ecleft-2 && y<cheight)
  { int i;
    char strnum[30]="Scr: ";
    if(k&1) { anistr--; if(anistr<0) {anistr=3; } }
    if(k&2) { anistr++; if(anistr>3) {anistr=0; } }
    itoa(anistr,strnum+5,10);
    guidrawb(12,99,71,109,strnum);
    anifrm=cframes; aniprt=0; anion=1; anitime=0;
    i=0;
    while(scrops[anistr].list[i]!='F' && scrops[anistr].list[i]!=255) i++;
    if(scrops[anistr].list[i]==255) anion=0;
    if(anion)
    { if(scrops[anistr].list[aniprt]=='F')
      { anifrm=squant[anistr].quan[aniprt]; }
      else if(scrops[anistr].list[aniprt]=='W')
      { anitime=squant[anistr].quan[aniprt]; }
      else if(scrops[anistr].list[aniprt]==255)
      { aniprt=0; }
      aniprt++;
    }
    drawani();
    cehold();
  }

  else
  { cehold(); }

return 0; }

void txfieldedit(int x1, int y1, int x2, int y2, char *tex, int len)
{ unsigned char c=0;
  char pos=0;
  guidrawbc(x1,y1,x2,y2,"");
  pos=strlen(tex);
  tex[pos]='_'; tex[pos+1]=0;
  blitfont(x1+2,y1+2,tex,gcols[0]);
  vesabuffer();
  cehold();
  while(c!=13)
  { c=getch();
    if(c==0) getch(); else if(c==8)
    { if(pos) { tex[pos]=0; pos--; tex[pos]='_'; }
      guidrawbc(x1,y1,x2,y2,""); blitfont(x1+2,y1+2,tex,gcols[0]); }
    else if((c>='0' && c<='9') || (c>='a' && c<='z') || (c>='A' && c<='Z')
                               || c=='-' || c=='~' || c=='.' || c=='_' )
    { if(pos<len) { tex[pos]=c; pos++; tex[pos]='_'; tex[pos+1]=0; }
      guidrawbc(x1,y1,x2,y2,""); blitfont(x1+2,y1+2,tex,gcols[0]); }
    else if(c==27) { c=13; } 
    vesabuffer();
  }
  tex[pos]=0;
return; }

#include <dos.h> /* time for the fileselector */
#define fswid 276
#define fshig 100
int fileselect(char *mask, int mode, char *filen)
{ int tx,ty,bx,by;
  char *fnms; struct find_t *ft; int fnm;
  int i,fdis,foff;
  char c;

  tx=(sw>>1)-(fswid>>1); ty=(sh>>1)-(fshig>>1);
  bx=tx+fswid; by=ty+fshig; guisysgrey();

  fnm=0;
  ft=malloc(sizeof(struct find_t));
  if(ft==NULL)
  { guidrawb(10,10,sw-11,20,"Out of memory, sorry. Press a key...");
    vesabuffer();
    if(!getch()) getch(); return 2; }
  fnms=malloc(13*256); /* Seeing maped, yeah, 256 seems a good number */
  if(fnms==NULL) { free(ft);
    guidrawb(10,10,sw-11,20,"Out of memory, sorry. Press a key...");
    vesabuffer();
    if(!getch()) getch(); return 2; }
  if(!_dos_findfirst(mask,0xff,ft))
  { memcpy(fnms+(fnm*13),ft->name,13); fnm++;
    while(!_dos_findnext(ft) && fnm<254)
    { memcpy(fnms+(fnm*13),ft->name,13); fnm++; }
  }
  free(ft);

  if(mode) { chrtype=0; }
  fdis=6*3; /* 3 columns, rows of six. */ foff=0;
  guidrawb(tx,ty,bx,by,mode?"Load a file":"Save a file");
  bfons(tx+4,ty+14,"File:");
  guidrawbc(tx+34,ty+12,tx+122,ty+22,filen);
  bfons(bx-101,ty+14,mode?"Load as:":"Save as:");
  if(mode) guidrawbc(bx-55,ty+12,bx-3,ty+22,chrtype?"V1 CHR":"AUTO");
  else guidrawbc(bx-55,ty+12,bx-3,ty+22,chrtype?"V1 CHR":"V2 CHR");
  guidrawbc(tx+3,ty+25,bx-3,by-14,"");
  for(i=0;i<fdis;i++)
  { if((i+foff)<fnm) { blitfont(tx+5+(80*(i/6)),ty+27+((i%6)*10),
                       fnms+((i+foff)*13),gcols[0]); } }
  guidrawb(tx+3,by-12,tx+13,by-2,"<");
  guidrawb(bx-13,by-12,bx-3,by-2,">");
  guidrawb(tx+102,by-12,tx+130,by-2,"Okay");
  guidrawb(tx+132,by-12,tx+169,by-2,"Cancel");

  filselagain:
  vesabuffer();
  msepoll(); mox=mousex; moy=mousey; mseblit(mousex,mousey);
  while(!kbhit() && !mousek)
  { msepoll(); if(mousex!=mox || mousey!=moy)
    { msebolt(mox,moy); mseblit(mousex,mousey); } }
  msebolt(mousex,mousey);

  if(mousek)
  { if(mousex>(tx+3) && mousex<(tx+13) && mousey>(by-12) && mousey<(by-2))
    { guidrawbc(tx+3,by-12,tx+13,by-2,"<");
      vesabuffer(); cehold();
      guidrawbc(tx+3,ty+25,bx-3,by-14,"");
      if(foff) foff--; for(i=0;i<fdis;i++)
      { if((i+foff)<fnm) { blitfont(tx+5+(80*(i/6)),ty+27+((i%6)*10),
                           fnms+((i+foff)*13),gcols[0]); } }
      guidrawb(tx+3,by-12,tx+13,by-2,"<");  }
    else if(mousex>(bx-13) && mousex<(bx-3) &&
            mousey>(by-12) && mousey<(by-2))
    { guidrawbc(bx-13,by-12,bx-3,by-2,">");
      vesabuffer();
      cehold();
      guidrawbc(tx+3,ty+25,bx-3,by-14,"");
      if(foff<(fnm-1)) foff++; for(i=0;i<fdis;i++)
      { if((i+foff)<fnm) { blitfont(tx+5+(80*(i/6)),ty+27+((i%6)*10),
                           fnms+((i+foff)*13),gcols[0]); } }
      guidrawb(bx-13,by-12,bx-3,by-2,">"); }
    else if (mousex>tx+3 && mousex<bx-3 && mousey>ty+25 && mousey<by-14)
    { int xob,yob,rob;
      vesabuffer();
      cehold();
      xob=(mousex-tx-3)/80; yob=(mousey-ty-25)/10;
      if(xob<0) xob=0; if(yob<0) yob=0;
      if(xob>2) xob=2; if(yob>5) yob=5;
      rob=foff+yob+(xob*6);
      if(rob<fnm) { strcpy(filen,fnms+(rob*13)); }
      guidrawbc(tx+34,ty+12,tx+122,ty+22,filen); }
    else if(mousex>tx+132 && mousex<tx+169 && mousey>by-12 && mousey<by-2)
    { guidrawbc(tx+132,by-12,tx+169,by-2,"Cancel");
      vesabuffer();
      cehold();
      free(fnms); return 1; }
    else if(mousex>tx+102 && mousex<tx+130 && mousey>by-12 && mousey<by-2)
    { guidrawbc(tx+102,by-12,tx+130,by-2,"Okay");
      vesabuffer();
      cehold(); goto filselen; }
    else if(mousex>bx-55 && mousex<bx-3 && mousey>ty+12 && mousey<ty+22)
    { vesabuffer();
      cehold(); chrtype=chrtype^1;
      if(mode) guidrawbc(bx-55,ty+12,bx-3,ty+22,chrtype?"V1 CHR":"AUTO");
      else guidrawbc(bx-55,ty+12,bx-3,ty+22,chrtype?"V1 CHR":"V2 CHR"); }
    else if(mousex>tx+34 && mousex<tx+122 && mousey>ty+12 && mousey<ty+22)
    { txfieldedit(tx+34,ty+12,tx+122,ty+22,filen,12);
      guidrawbc(tx+34,ty+12,tx+122,ty+22,filen); }
    else cehold();
  }
  vesabuffer();
  if(kbhit())
  { c=getch();
    if(c==27) { free(fnms); return 1; }
    else if(c==13)
    { goto filselen; }
    else if(c==0)
    { c=getch(); }
  }
  goto filselagain;
  filselen:

  free(fnms);
return 0; } /* return 2 on out of memory, 1 on cancel, 0 on file */

/* --- The editor --- */
char chreditor()
{ int quitout;
  int i,j,zx,zy;
  int anictime=0;

  mousek=0;
  if(!mseinit())
  { setpal(syspal);
    SDVideo();
    printf("Sorry, the mouse could not be found.\n");
    exit(0x01); }

  cfnamemax=(sw-42)/6;
  if(cfnamemax>74) cfnamemax=74;

  edframe=0; edmode=0;
  if(cheight>sh-5) { edmode=1; }
  if(cwidth>(sw/3)) { edmode=1; }
  ecleft=89+((cwidth+1)*(edmode^1));

  if(!zoomcfg)
  { zoom=0; zx=0; zy=0;
    for(i=0;i<4;i++)
    { if((cheight*(1<<i))<(sh-10)) zy=i; } /* always get max zoom. */
    for(i=0;i<4;i++)
    { if((ecleft+(cwidth*(1<<i)))<(sw-10)) zx=i; }
    if(zx>=zy) { zoom=zy; } else { zoom=zx; }
  }

  ecxmax=(cwidth*(1<<zoom))+ecleft;
  ecymax=cheight*(1<<zoom);

  if(cheight*(1<<zoom)>(sh-11)) /* large sprites need scrolling */
  { scroffy=0;
    scrollyon=1;
    ecymax=sh-11; }
  if((ecleft+(cwidth*(1<<zoom)))>(sw-11))
  { scroffx=0;
    scrollxon=1;
    ecxmax=sw-11; }

  if(scrollxon) { ecxmax-=(ecxmax-ecleft)%(1<<zoom); }
  if(scrollyon) { ecymax-=ecymax%(1<<zoom); }

  if(edmode==1) anion=0;
  initctimer();
  anistr=0; anifrm=0; aniprt=0; anion=1; anitime=0;

  if(anion)
  { i=0;
    while(scrops[anistr].list[i]!='F' && scrops[anistr].list[i]!=255) i++;
    if(scrops[anistr].list[i]==255) anion=0; }
  if(anion)
  { if(scrops[anistr].list[aniprt]=='F')
    { anifrm=squant[anistr].quan[aniprt]; }
    else if(scrops[anistr].list[aniprt]=='W')
    { anitime=squant[anistr].quan[aniprt]; }
    else if(scrops[anistr].list[aniprt]==255)
    { aniprt=0; }
    aniprt++;
  }
  anictime=ctimer();
  backuptime=ctimer();
  screensavertimer=ctimer();

  eddrwscr();

  quitout=0;
  while(!quitout)
  {
    msepoll();
    mox=mousex; moy=mousey;
    mseblit(mousex,mousey);
    screensavertimer=ctimer();
    while(!kbhit() && !mousek)
    { msepoll();
      if(mousex!=mox || mousey!=moy)
      { msebolt(mox,moy);
        guidrawbc(1,111,82,120,""); // x,y box.
        if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
        { int dx,dy;
          char nums[25];
          dx=mousex-ecleft;
          dy=mousey;
          dx=dx>>zoom;
          dy=dy>>zoom;
          dx+=scroffx;
          dy+=scroffy;
          itoa(dx,nums,10);
          nums[strlen(nums)+1]=0;
          nums[strlen(nums)]=',';
          itoa(dy,nums+strlen(nums),10);
          blitfont(3,113,nums,gcols[0]);
        }
        mseblit(mousex,mousey);
        screensavertimer=ctimer();
      }
      if((ctimer()-screensavertimer)>6000)
      { msebolt(mousex,mousey); v2cescreensaver();
        mseblit(mousex,mousey); screensavertimer=ctimer(); }
      if(anion)
      { if(anitime)
        { if((ctimer()-anictime))
          { anitime-=(ctimer()-anictime);
            if(anitime<0) anitime=0; 
            anictime=ctimer();
        } }
        else
        { if(scrops[anistr].list[aniprt]=='F')
          { anifrm=squant[anistr].quan[aniprt]; 
            msebolt(mousex,mousey);
            drawani(); 
            mseblit(mousex,mousey);
          }
          else if(scrops[anistr].list[aniprt]=='W')
          { anitime=squant[anistr].quan[aniprt]; }
          else if(scrops[anistr].list[aniprt]==255)
          { aniprt=-1; }
          aniprt++;
      } }
      if(backupon)
      { if((ctimer()-backuptime)>18000)
        { if(savechr(backupfile,0))
          { guisysgrey(); backupon=0;
 guidrawb(10,10,sw-11,20,"Invalid backup file.");
 guidrawb(10,22,sw-11,32,"Backup option turned off. Press a key...");
            if(!getch()) getch(); }
          backuptime=ctimer(); }
      }
    }
    msebolt(mousex,mousey);

    if(kbhit())
    { unsigned char k;
      k=getch();
      if(k==27) // Esc
      { if(!gpclosebox()) quitout=1;
        else { quitout=0; eddrwscr(); }
      }
      else if(k=='g' || k=='G')
      { gridon=gridon^1;
        eddrwscr();
      }
      else if(k=='a' || k=='A')
      { int i,j;
        cframes++;
        cdata=realloc(cdata,cwidth*cheight*(cframes+1));
        if(cdata==NULL)
        { free(clipboard);
          setpal(syspal);
          SDVideo();
          printf("Not enough memory. Aborting program.\n");
          exit(0x02); }
        if((edframe+2)<cframes)
        { memmove(cdata+((edframe+2)*cfrmsize),
                  cdata+((edframe+1)*cfrmsize),
                  cfrmsize*(cframes-edframe-1)); }
        memset(cdata+((edframe+1)*cfrmsize),0,cfrmsize);
        for(i=0;i<8;i++)
        { for(j=0;j<200;j++)
          { if(scrops[i].list[j]=='F')
            { if(squant[i].quan[j]>edframe)
              { squant[i].quan[j]++; }
            }
          }
        }
        eddrwscr();
      }
      else if(k=='k' || k=='K')
      { int ax,ay;
        memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize); /* pickup undo */
        for(ay=0;ay<cheight;ay++)
        { for(ax=0;ax<cwidth;ax++)
          { if(cdata[(edframe*cfrmsize)+(ay*cwidth)+ax]==ecoll)
            { cdata[(edframe*cfrmsize)+(ay*cwidth)+ax]=ecolr; }
            else if(cdata[(edframe*cfrmsize)+(ay*cwidth)+ax]==ecolr)
            { cdata[(edframe*cfrmsize)+(ay*cwidth)+ax]=ecoll; }
          }
        }
        eddrwscr();
      }
      else if(k=='l' || k=='L')
      { int ax,ay;
        memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize); /* pickup undo */
        for(ay=0;ay<cheight;ay++)
        { for(ax=0;ax<cwidth;ax++)
          { if(cdata[(edframe*cfrmsize)+(ay*cwidth)+ax]==ecolr)
            { cdata[(edframe*cfrmsize)+(ay*cwidth)+ax]=ecoll; }
          }
        }
        eddrwscr();
      }
      else if(k=='r' || k=='R') { eddrwscr(); }
      else if(k=='t' || k=='T') { v2cescreensaver(); }
      else if(k=='-')
      { if(edframe) edframe--;
        memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);
        eddrwscr(); }
      else if(k=='+')
      { if(edframe<(cframes-1)) edframe++;
        memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);
        eddrwscr(); }
      else if(k=='z' || k=='Z')
      { if(zoom) zoom--;
        scrollxon=0; scrollyon=0; scroffx=0; scroffy=0;
        ecxmax=(cwidth*(1<<zoom))+ecleft; ecymax=cheight*(1<<zoom);
        if(cheight*(1<<zoom)>(sh-11))
        { scroffy=0;
          scrollyon=1;
          ecymax=sh-11; }
        if((ecleft+(cwidth*(1<<zoom)))>(sw-11))
        { scroffx=0;
          scrollxon=1;
          ecxmax=sw-11; }
        if(scrollxon) { ecxmax-=(ecxmax-ecleft)%(1<<zoom); }
        if(scrollyon) { ecymax-=ecymax%(1<<zoom); }
        eddrwscr();
      }
      else if(k=='s' || k=='S')
      { invon=invon^1;
        if(invon) { setcol(0,63,63,63); setcol(255,0,0,0); }
        else { setcol(0,0,0,0); setcol(255,63,63,63); }
      }
      else if(k=='x' || k=='X')
      { if(zoom<3) zoom++;
        scrollxon=0; scrollyon=0; scroffx=0; scroffy=0;
        ecxmax=(cwidth*(1<<zoom))+ecleft; ecymax=cheight*(1<<zoom);
        if(cheight*(1<<zoom)>(sh-11))
        { scroffy=0;
          scrollyon=1;
          ecymax=sh-11; }
        if((ecleft+(cwidth*(1<<zoom)))>(sw-11))
        { scroffx=0;
          scrollxon=1;
          ecxmax=sw-11; }
        if(scrollxon) { ecxmax-=(ecxmax-ecleft)%(1<<zoom); }
        if(scrollyon) { ecymax-=ecymax%(1<<zoom); }
        eddrwscr();
      }
      else if(k=='d' || k=='D')
      { drawmode=drawmode^1;
        guidrawb(42,13,82,23,drawmode?"Fill":"Draw"); }
      else if(k=='[') { ecoll--; drawpal(); }
      else if(k==']') { ecoll++; drawpal(); }
      else if(k=='{') { ecolr--; drawpal(); }
      else if(k=='}') { ecolr++; drawpal(); }
      else if(k=='H' || k=='h')
      { int i,j;
        memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);
        for(j=0;j<cheight;j++)
        { for(i=0;i<cwidth;i++)
        { tempchr[((cwidth-1)-i)+(cwidth*j)] =
          cdata[(edframe*cfrmsize)+(cwidth*j)+i]; } }
        for(j=0;j<cheight;j++)
        { for(i=0;i<cwidth;i++)
        { cdata[(edframe*cfrmsize)+(cwidth*j)+i]=tempchr[i+(cwidth*j)]; } }
        drawed();
        vesabuffer();
      }
      else if(k=='V' || k=='v')
      { int i,j;
        memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);
        for(j=0;j<cheight;j++)
        { for(i=0;i<cwidth;i++)
        { tempchr[i+(cwidth*((cheight-1)-j))] =
          cdata[(edframe*cfrmsize)+(cwidth*j)+i]; } }
        for(j=0;j<cheight;j++)
        { for(i=0;i<cwidth;i++)
        { cdata[(edframe*cfrmsize)+(cwidth*j)+i]=tempchr[i+(cwidth*j)]; } }
        drawed();
        vesabuffer();
      }
      else if(k=='o' || k=='O')
      { int x1,x2,y1,y2,csx,x,y;
        char locato[20];
        guisysgrey();
        drawed();
        guidrawb(1,1,82,34,"Pick Corners");
        guidrawbc(3,10,80,20,"");
        guidrawbc(3,22,80,32,"");

        vesabuffer();
        csx=2; // Counts times through mouse loop.
keypresscutloop:
        msepoll();
        mox=mousex; moy=mousey;
        mseblit(mousex,mousey);
        while(!kbhit() && !mousek)
        { msepoll();
          if(mousex!=mox || mousey!=moy)
          { msebolt(mox,moy);
            guidrawbc(1,111,82,120,""); // x,y box.
            if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
            { int dx,dy;
              char nums[25];
              dx=mousex-ecleft;
              dy=mousey;
              dx=dx>>zoom;
              dy=dy>>zoom;
              dx+=scroffx;
              dy+=scroffy;
              itoa(dx,nums,10);
              nums[strlen(nums)+1]=0;
              nums[strlen(nums)]=',';
              itoa(dy,nums+strlen(nums),10);
              blitfont(3,113,nums,gcols[0]);
            }
            mseblit(mousex,mousey);
          }
        }
        msebolt(mousex,mousey);
        x=mousex; y=mousey;

        while(kbhit()) { getch(); }

        if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
        { int dx,dy;
          dx=mousex-ecleft;
          dy=mousey;
          dx=dx>>zoom;
          dy=dy>>zoom;
          dx+=scroffx;
          dy+=scroffy;
          if(csx==2)
          { x1=dx; y1=dy;
            csx=1;
            itoa(x1,locato,10);
            dx=strlen(locato);
            locato[dx]=',';
            itoa(y1,locato+dx+1,10);
            guidrawbc(3,10,80,20,locato);
            cehold();
          }
          else if(csx==1)
          { x2=dx; y2=dy;
            csx=0;
            itoa(x2,locato,10);
            dx=strlen(locato);
            locato[dx]=',';
            itoa(y2,locato+dx+1,10);
            guidrawbc(3,22,80,32,locato);
            cehold();
          }
        }

        else if(mousex>sw-11 && mousex<sw-1 && mousey>sh-11 && mousey<sh-1)
        { char hydrochloric[3];
          guidrawbc(sw-11,sh-11,sw-1,sh-1,itoa(zoom,hydrochloric,10));
          if(mousek&1) { if(zoom<3) zoom++; }
          else if(mousek&2) { if(zoom) zoom--; }
          scrollxon=0; scrollyon=0; scroffx=0; scroffy=0;
          ecxmax=(cwidth*(1<<zoom))+ecleft; ecymax=cheight*(1<<zoom);
          if(cheight*(1<<zoom)>(sh-11))
          { scroffy=0;
            scrollyon=1;
            ecymax=sh-11; }
          if((ecleft+(cwidth*(1<<zoom)))>(sw-11))
          { scroffx=0;
            scrollxon=1;
            ecxmax=sw-11; }
          if(scrollxon) { ecxmax-=(ecxmax-ecleft)%(1<<zoom); }
          if(scrollyon) { ecymax-=ecymax%(1<<zoom); }
          vesabuffer();
          cehold();
          eddrwscr();
          guisysgrey();
          drawed();
          locato[0]=0;
          if(csx==1)
          { int poo;
            itoa(x1,locato,10);
            poo=strlen(locato);
            locato[poo]=',';
            itoa(y1,locato+poo+1,10);
          }
          guidrawb(1,1,82,34,"Pick Corners");
          guidrawbc(3,10,80,20,locato);
          guidrawbc(3,22,80,32,"");
          vesabuffer();
        }
 
        else if(scrollxon && x>scrbx1 && x<scrbx2 && y>sh-11 && y<sh-1)
        { int bi,bk,bm,bn,bl,br,bx,bz;
          int moff;
          guidrawbc(scrbx1,sh-11,scrbx2,sh-1,"");
          bi=(ecxmax-ecleft)>>zoom;
          bk=(sw-ecleft)-31;
          bm=(bi*bk)/cwidth;
          bn=(scroffx*bk)/cwidth;
          bn+=ecleft+9;
          scrxmax=cwidth-((ecxmax-ecleft)>>zoom);
 
          bl=bn; br=bn+bm; bx=(sw-23)-bm; bz=ecleft+9;
          moff=x-scrbx1;
 
          vesabuffer();
          while(mousek)
          { msepoll();
            if(mousex!=mox)
            { hline(ecleft+9,sh-11,sw-23,gcols[5]);
              hline(ecleft+9,sh-1,sw-23,gcols[6]);
              box(ecleft+9,sh-10,sw-23,sh-2,gcols[4]);
              bl=mousex-moff;
              if(bl<bz) { bl=bz; moff=mousex-bl; }
              if(bl>bx) { bl=bx; moff=mousex-bl; }
              guidrawbc(bl,sh-11,bl+bm,sh-1,"");
              vesabuffer();
            }
          }
          guidrawb(bl,sh-11,bl+bm,sh-1,""); 
          scroffx=((bl-(ecleft+9))*cwidth)/bk;
          if(bl==bx) scroffx=scrxmax;
          else if(scroffx>scrxmax) scroffx=scrxmax;
          if(scroffx<0) scroffx=0;
          eddrwscr();
          guisysgrey();
          drawed();
          locato[0]=0;
          if(csx==1)
          { int poo;
            itoa(x1,locato,10);
            poo=strlen(locato);
            locato[poo]=',';
            itoa(y1,locato+poo+1,10);
          }
          guidrawb(1,1,82,34,"Pick Corners");
          guidrawbc(3,10,80,20,locato);
          guidrawbc(3,22,80,32,"");
          vesabuffer();
        }
        else if(scrollyon && y>scrby1 && y<scrby2 && x>sw-11 && x<sw-1)
        { int bi,bk,bm,bn,bl,br,bx,bz;
          int moff;
          guidrawbc(sw-11,scrby1,sw-1,scrby2,"");
          vesabuffer();
          bi=ecymax>>zoom;
          bk=sh-33;
          bm=(bi*bk)/cheight;
          bn=(scroffy*bk)/cheight;
          bn+=11;
          scrymax=cheight-(ecymax>>zoom);

          bl=bn; br=bn+bm; bx=(sh-23)-bm; bz=11;
          moff=y-scrby1;

          while(mousek)
          { msepoll();
            if(mousey!=moy)
            { vline(sw-11,11,sh-23,gcols[5]);
              vline(sw-1,11,sh-23,gcols[6]);
              box(sw-10,11,sw-2,sh-23,gcols[4]);
              bl=mousey-moff;
              if(bl<bz) { bl=bz; moff=mousey-bl; }
              if(bl>bx) { bl=bx; moff=mousey-bl; }
              guidrawbc(sw-11,bl,sw-1,bl+bm,"");
              vesabuffer();
            }
          }
          guidrawb(sw-11,bl,sw-1,bl+bm,"");
          scroffy=((bl-11)*cheight)/bk;
          if(bl==bx) scroffy=scrymax;
          else if(scroffy>scrymax) scroffy=scrymax;
          if(scroffy<0) scroffy=0;
          eddrwscr();
          guisysgrey();
          drawed();
          locato[0]=0;
          if(csx==1)
          { int poo;
            itoa(x1,locato,10);
            poo=strlen(locato);
            locato[poo]=',';
            itoa(y1,locato+poo+1,10);
          }
          guidrawb(1,1,82,34,"Pick Corners");
          guidrawbc(3,10,80,20,locato);
          guidrawbc(3,22,80,32,"");
          vesabuffer();
        }
        if(csx) goto keypresscutloop;

// handle the values here.
        if(x2<x1) { int intex; intex=x2; x2=x1; x1=intex; }
        if(y2<y1) { int intey; intey=y2; y2=y1; y1=intey; }

        clipstats=2; /* shape */
        clipwidth=x2-x1+1;
        clipheight=y2-y1+1;

        for(x=x1;x<=x2;x++)
        { for(y=y1;y<=y2;y++)
          { clipboard[(x-x1)+((y-y1)*clipwidth)]
              = cdata[(edframe*cfrmsize)+x+(y*cwidth)];
        } }

        eddrwscr();
      }
      else if(k=='f' || k=='F')
      { int i,j;
        for(j=0;j<cheight;j++) { for(i=0;i<cwidth;i++)
        { clipboard[i+(j*cwidth)]=cdata[(edframe*cfrmsize)+i+(j*cwidth)]; }}
        clipstats=1; /* frame */
 //       guidrawb(1,50,82,60,clipstattxt[clipstats]);
        guidrawb( 1,74,41,84,"Paste");
        guidrawb(43,74,82,84,"Mask");
        vesabuffer();
      }
      else if((k=='p' || k=='P') && clipstats==1)
      { int i,j;
        memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);
        for(j=0;j<cheight;j++) { for(i=0;i<cwidth;i++)
        { cdata[(edframe*cfrmsize)+i+(j*cwidth)]=clipboard[i+(j*cwidth)]; }}  
        drawed();
        vesabuffer();
      }
      else if((k=='p' || k=='P') && clipstats==2)
      { int i,j,x,y;
        vesabuffer();
        guisysgrey();
        drawed();
        memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);

keypasteagain:

        memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
        msepoll();
        mox=mousex; moy=mousey;
            if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
            { int dx,dy;
              char nums[25];
              dx=mousex-ecleft;
              dy=mousey;
              dx=dx>>zoom;
              dy=dy>>zoom;
              dx+=scroffx;
              dy+=scroffy;
              itoa(dx,nums,10);
              nums[strlen(nums)+1]=0;
              nums[strlen(nums)]=',';
              itoa(dy,nums+strlen(nums),10);
              blitfont(3,113,nums,gcols[0]);
              for(j=0;j<clipheight;j++)
              { for(i=0;i<clipwidth;i++)
                { if((dx+i)<cwidth && (dy+j)<cheight)
                   cdata[(edframe*cfrmsize)+dx+i+((dy+j)*cwidth)]
                   = clipboard[i+(j*clipwidth)];
                }
              }
            }
        drawed();
        vesabuffer();
        mseblit(mousex,mousey);
        while(!kbhit() && !mousek)
        { msepoll();
          if(mousex!=mox || mousey!=moy)
          { msebolt(mox,moy);
            guidrawbc(1,111,82,120,""); // x,y box.
            if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
            { int dx,dy;
              char nums[25];
              dx=mousex-ecleft;
              dy=mousey;
              dx=dx>>zoom;
              dy=dy>>zoom;
              dx+=scroffx;
              dy+=scroffy;
              itoa(dx,nums,10);
              nums[strlen(nums)+1]=0;
              nums[strlen(nums)]=',';
              itoa(dy,nums+strlen(nums),10);
              blitfont(3,113,nums,gcols[0]);
              memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
              for(j=0;j<clipheight;j++)
              { for(i=0;i<clipwidth;i++)
                { if((dx+i)<cwidth && (dy+j)<cheight)
                   cdata[(edframe*cfrmsize)+dx+i+((dy+j)*cwidth)]
                   = clipboard[i+(j*clipwidth)];
                }
              }
            }
            else memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
            drawed();
            vesabuffer();
            mseblit(mousex,mousey);
          }
        }
        msebolt(mousex,mousey);
        x=mousex; y=mousey;

        while(kbhit()) { char poopy;
                         poopy=getch();
                         if(!poopy) getch();
                         else if (poopy==27)
                         { memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
                           eddrwscr(); goto keypastefin; }
                       }

        if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
        { int dx,dy;
          dx=mousex-ecleft;
          dy=mousey;
          dx=dx>>zoom;
          dy=dy>>zoom;
          dx+=scroffx;
          dy+=scroffy;
          goto keypastefin;
        }

        else if(mousex>sw-11 && mousex<sw-1 && mousey>sh-11 && mousey<sh-1)
        { char hydrochloric[3];
          guidrawbc(sw-11,sh-11,sw-1,sh-1,itoa(zoom,hydrochloric,10));
          if(mousek&1) { if(zoom<3) zoom++; }
          else if(mousek&2) { if(zoom) zoom--; }
          scrollxon=0; scrollyon=0; scroffx=0; scroffy=0;
          ecxmax=(cwidth*(1<<zoom))+ecleft; ecymax=cheight*(1<<zoom);
          if(cheight*(1<<zoom)>(sh-11))
          { scroffy=0;
            scrollyon=1;
            ecymax=sh-11; }
          if((ecleft+(cwidth*(1<<zoom)))>(sw-11))
          { scroffx=0;
            scrollxon=1;
            ecxmax=sw-11; }
          if(scrollxon) { ecxmax-=(ecxmax-ecleft)%(1<<zoom); }
          if(scrollyon) { ecymax-=ecymax%(1<<zoom); }
          vesabuffer();
          cehold();
          eddrwscr();
          guisysgrey();
          drawed();
          vesabuffer();
        }
 
        else if(scrollxon && x>scrbx1 && x<scrbx2 && y>sh-11 && y<sh-1)
        { int bi,bk,bm,bn,bl,br,bx,bz;
          int moff;
          guidrawbc(scrbx1,sh-11,scrbx2,sh-1,"");
          bi=(ecxmax-ecleft)>>zoom;
          bk=(sw-ecleft)-31;
          bm=(bi*bk)/cwidth;
          bn=(scroffx*bk)/cwidth;
          bn+=ecleft+9;
          scrxmax=cwidth-((ecxmax-ecleft)>>zoom);
 
          bl=bn; br=bn+bm; bx=(sw-23)-bm; bz=ecleft+9;
          moff=x-scrbx1;
 
          vesabuffer();
          while(mousek)
          { msepoll();
            if(mousex!=mox)
            { hline(ecleft+9,sh-11,sw-23,gcols[5]);
              hline(ecleft+9,sh-1,sw-23,gcols[6]);
              box(ecleft+9,sh-10,sw-23,sh-2,gcols[4]);
              bl=mousex-moff;
              if(bl<bz) { bl=bz; moff=mousex-bl; }
              if(bl>bx) { bl=bx; moff=mousex-bl; }
              guidrawbc(bl,sh-11,bl+bm,sh-1,"");
              vesabuffer();
            }
          }
          guidrawb(bl,sh-11,bl+bm,sh-1,""); 
          scroffx=((bl-(ecleft+9))*cwidth)/bk;
          if(bl==bx) scroffx=scrxmax;
          else if(scroffx>scrxmax) scroffx=scrxmax;
          if(scroffx<0) scroffx=0;
          eddrwscr();
          guisysgrey();
          drawed();
          vesabuffer();
        }
        else if(scrollyon && y>scrby1 && y<scrby2 && x>sw-11 && x<sw-1)
        { int bi,bk,bm,bn,bl,br,bx,bz;
          int moff;
          guidrawbc(sw-11,scrby1,sw-1,scrby2,"");
          vesabuffer();
          bi=ecymax>>zoom;
          bk=sh-33;
          bm=(bi*bk)/cheight;
          bn=(scroffy*bk)/cheight;
          bn+=11;
          scrymax=cheight-(ecymax>>zoom);

          bl=bn; br=bn+bm; bx=(sh-23)-bm; bz=11;
          moff=y-scrby1;

          while(mousek)
          { msepoll();
            if(mousey!=moy)
            { vline(sw-11,11,sh-23,gcols[5]);
              vline(sw-1,11,sh-23,gcols[6]);
              box(sw-10,11,sw-2,sh-23,gcols[4]);
              bl=mousey-moff;
              if(bl<bz) { bl=bz; moff=mousey-bl; }
              if(bl>bx) { bl=bx; moff=mousey-bl; }
              guidrawbc(sw-11,bl,sw-1,bl+bm,"");
              vesabuffer();
            }
          }
          guidrawb(sw-11,bl,sw-1,bl+bm,"");
          scroffy=((bl-11)*cheight)/bk;
          if(bl==bx) scroffy=scrymax;
          else if(scroffy>scrymax) scroffy=scrymax;
          if(scroffy<0) scroffy=0;
          eddrwscr();
          guisysgrey();
          drawed();
          vesabuffer();
        }
        else { eddrwscr(); goto keypastefin; }
        
        goto keypasteagain;

keypastefin:

        eddrwscr();
        cehold();
      }
      else if((k=='m' || k=='M') && clipstats==1)
      { int i,j; unsigned char m;
        memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);
        if(k=='M') m=ecoll;
        else m=0;
        for(j=0;j<cheight;j++) { for(i=0;i<cwidth;i++)
        { if(clipboard[i+(j*cwidth)]!=m)
        { cdata[(edframe*cfrmsize)+i+(j*cwidth)]=clipboard[i+(j*cwidth)]; }
        }}
        drawed();
        vesabuffer();
      }
      else if((k=='m' || k=='M') && clipstats==2)
      { int i,j,x,y; unsigned char m;
        if(k=='M') m=ecoll;
        else m=0;
        vesabuffer();
        guisysgrey();
        drawed();
        memcpy(undo,cdata+(edframe*cfrmsize),cfrmsize);

keymaskagain:

        memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
        msepoll();
        mox=mousex; moy=mousey;
            if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
            { int dx,dy;
              char nums[25];
              dx=mousex-ecleft;
              dy=mousey;
              dx=dx>>zoom;
              dy=dy>>zoom;
              dx+=scroffx;
              dy+=scroffy;
              itoa(dx,nums,10);
              nums[strlen(nums)+1]=0;
              nums[strlen(nums)]=',';
              itoa(dy,nums+strlen(nums),10);
              blitfont(3,113,nums,gcols[0]);
              for(j=0;j<clipheight;j++)
              { for(i=0;i<clipwidth;i++)
                { if((dx+i)<cwidth && (dy+j)<cheight && clipboard[i+(j*clipwidth)]!=m)
                   cdata[(edframe*cfrmsize)+dx+i+((dy+j)*cwidth)]
                   = clipboard[i+(j*clipwidth)];
                }
              }
            }
        drawed();
        vesabuffer();
        mseblit(mousex,mousey);
        while(!kbhit() && !mousek)
        { msepoll();
          if(mousex!=mox || mousey!=moy)
          { msebolt(mox,moy);
            guidrawbc(1,111,82,120,""); // x,y box.
            if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
            { int dx,dy;
              char nums[25];
              dx=mousex-ecleft;
              dy=mousey;
              dx=dx>>zoom;
              dy=dy>>zoom;
              dx+=scroffx;
              dy+=scroffy;
              itoa(dx,nums,10);
              nums[strlen(nums)+1]=0;
              nums[strlen(nums)]=',';
              itoa(dy,nums+strlen(nums),10);
              blitfont(3,113,nums,gcols[0]);
              memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
              for(j=0;j<clipheight;j++)
              { for(i=0;i<clipwidth;i++)
                { if((dx+i)<cwidth && (dy+j)<cheight && clipboard[i+(j*clipwidth)]!=m)
                   cdata[(edframe*cfrmsize)+dx+i+((dy+j)*cwidth)]
                   = clipboard[i+(j*clipwidth)];
                }
              }
            }
            else memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
            drawed();
            vesabuffer();
            mseblit(mousex,mousey);
          }
        }
        msebolt(mousex,mousey);
        x=mousex; y=mousey;

        while(kbhit()) { char poopy;
                         poopy=getch();
                         if(!poopy) getch();
                         else if (poopy==27)
                         { memcpy(cdata+(edframe*cfrmsize),undo,cfrmsize);
                           eddrwscr(); goto keymaskfin; }
                       }

        if(mousex>=ecleft && mousex<ecxmax && mousey<ecymax)
        { int dx,dy;
          dx=mousex-ecleft;
          dy=mousey;
          dx=dx>>zoom;
          dy=dy>>zoom;
          dx+=scroffx;
          dy+=scroffy;
          goto keymaskfin;
        }

        else if(mousex>sw-11 && mousex<sw-1 && mousey>sh-11 && mousey<sh-1)
        { char hydrochloric[3];
          guidrawbc(sw-11,sh-11,sw-1,sh-1,itoa(zoom,hydrochloric,10));
          if(mousek&1) { if(zoom<3) zoom++; }
          else if(mousek&2) { if(zoom) zoom--; }
          scrollxon=0; scrollyon=0; scroffx=0; scroffy=0;
          ecxmax=(cwidth*(1<<zoom))+ecleft; ecymax=cheight*(1<<zoom);
          if(cheight*(1<<zoom)>(sh-11))
          { scroffy=0;
            scrollyon=1;
            ecymax=sh-11; }
          if((ecleft+(cwidth*(1<<zoom)))>(sw-11))
          { scroffx=0;
            scrollxon=1;
            ecxmax=sw-11; }
          if(scrollxon) { ecxmax-=(ecxmax-ecleft)%(1<<zoom); }
          if(scrollyon) { ecymax-=ecymax%(1<<zoom); }
          vesabuffer();
          cehold();
          eddrwscr();
          guisysgrey();
          drawed();
          vesabuffer();
        }
 
        else if(scrollxon && x>scrbx1 && x<scrbx2 && y>sh-11 && y<sh-1)
        { int bi,bk,bm,bn,bl,br,bx,bz;
          int moff;
          guidrawbc(scrbx1,sh-11,scrbx2,sh-1,"");
          bi=(ecxmax-ecleft)>>zoom;
          bk=(sw-ecleft)-31;
          bm=(bi*bk)/cwidth;
          bn=(scroffx*bk)/cwidth;
          bn+=ecleft+9;
          scrxmax=cwidth-((ecxmax-ecleft)>>zoom);
 
          bl=bn; br=bn+bm; bx=(sw-23)-bm; bz=ecleft+9;
          moff=x-scrbx1;
 
          vesabuffer();
          while(mousek)
          { msepoll();
            if(mousex!=mox)
            { hline(ecleft+9,sh-11,sw-23,gcols[5]);
              hline(ecleft+9,sh-1,sw-23,gcols[6]);
              box(ecleft+9,sh-10,sw-23,sh-2,gcols[4]);
              bl=mousex-moff;
              if(bl<bz) { bl=bz; moff=mousex-bl; }
              if(bl>bx) { bl=bx; moff=mousex-bl; }
              guidrawbc(bl,sh-11,bl+bm,sh-1,"");
              vesabuffer();
            }
          }
          guidrawb(bl,sh-11,bl+bm,sh-1,""); 
          scroffx=((bl-(ecleft+9))*cwidth)/bk;
          if(bl==bx) scroffx=scrxmax;
          else if(scroffx>scrxmax) scroffx=scrxmax;
          if(scroffx<0) scroffx=0;
          eddrwscr();
          guisysgrey();
          drawed();
          vesabuffer();
        }
        else if(scrollyon && y>scrby1 && y<scrby2 && x>sw-11 && x<sw-1)
        { int bi,bk,bm,bn,bl,br,bx,bz;
          int moff;
          guidrawbc(sw-11,scrby1,sw-1,scrby2,"");
          vesabuffer();
          bi=ecymax>>zoom;
          bk=sh-33;
          bm=(bi*bk)/cheight;
          bn=(scroffy*bk)/cheight;
          bn+=11;
          scrymax=cheight-(ecymax>>zoom);

          bl=bn; br=bn+bm; bx=(sh-23)-bm; bz=11;
          moff=y-scrby1;

          while(mousek)
          { msepoll();
            if(mousey!=moy)
            { vline(sw-11,11,sh-23,gcols[5]);
              vline(sw-1,11,sh-23,gcols[6]);
              box(sw-10,11,sw-2,sh-23,gcols[4]);
              bl=mousey-moff;
              if(bl<bz) { bl=bz; moff=mousey-bl; }
              if(bl>bx) { bl=bx; moff=mousey-bl; }
              guidrawbc(sw-11,bl,sw-1,bl+bm,"");
              vesabuffer();
            }
          }
          guidrawb(sw-11,bl,sw-1,bl+bm,"");
          scroffy=((bl-11)*cheight)/bk;
          if(bl==bx) scroffy=scrymax;
          else if(scroffy>scrymax) scroffy=scrymax;
          if(scroffy<0) scroffy=0;
          eddrwscr();
          guisysgrey();
          drawed();
          vesabuffer();
        }
        else { eddrwscr(); goto keymaskfin; }
        
        goto keymaskagain;

keymaskfin:

        eddrwscr();
        cehold();
      }
      else if(k==0) // Extended ASCII
      { k=getch();
        if(k==75) { if(scrollxon) { if(scroffx) scroffx--; drawed(); vesabuffer();} }
        else if(k==77)
        { if(scrollxon) { if(scroffx<scrxmax) scroffx++; drawed(); vesabuffer();} }
        else if(k==72) { if(scrollyon) { if(scroffy) scroffy--; drawed(); vesabuffer();}}
        else if(k==80)
        { if(scrollyon) { if(scroffy<scrymax) scroffy++; drawed(); vesabuffer();} }
        else if(k==68) // F10
        { if(!gpclosebox()) quitout=1; else { quitout=0; eddrwscr(); } }
        else if(k==65) // F7
        { gpexportpcx(); eddrwscr(); }
        else if(k==66) // F8
        { mseblit(mousex,mousey);
          if(vmd==VMD_13H)
          { savepcx("v2chr.pcx",vid,320,200,edpal); }
          msebolt(mousex,mousey);
          eddrwscr(); }
        else if(k==62)
        { char fnm[20]; char cttemp;
          strcpy(fnm,cfront); cttemp=chrtype;
          if(!fileselect("*.CHR",1,fnm))
          { int oldwidth,oldheight; oldwidth=cwidth; oldheight=cheight;
            if(!loadchr(fnm))
            { strcpy(cfront,fnm);
              if(oldwidth!=cwidth || oldheight!=cheight)
              { free(clipboard); clipstats=0;
                clipboard=malloc(cfrmsize*3);
                if(clipboard==NULL)
                { free(cdata); setpal(syspal);  SDVideo();
             printf("Not enough memory. Aborting program.\n"); exit(0x02);}
              tempchr=clipboard+cfrmsize;
              undo=tempchr+cfrmsize; }
              memcpy(undo,cdata,cfrmsize); /* preload the first frame */
            }
            else { guisysgrey();
  guidrawb(10,10,sw-11,20,"Unable to load specified file. Press a key..."); 
            if(!getch()) getch(); }}
          eddrwscr(); chrtype=cttemp; 
        }
        else if(k==63)
        { char fnm[20]; chrtype=0;
          strcpy(fnm,cfront);
          if(savechr(fnm,chrtype))
          { guisysgrey();
            guidrawb(10,10,sw-11,20,"Unable to save file. Press a key..."); 
            if(!getch()) getch(); eddrwscr(); }
        }
        else if(k==64)
        { char fnm[20]; chrtype=0;
          strcpy(fnm,cfront);
          if(!fileselect("*.CHR",0,fnm))
          { if(!savechr(fnm,chrtype))
            { strcpy(cfront,fnm); }
            else { guisysgrey();
             guidrawb(10,10,sw-11,20,"Unable to save file. Press a key..."); 
              if(!getch()) getch(); }
          }
        eddrwscr(); }
        k=0;
      }
    }
    if(mousek)
    { if(handleclick(mousek,mousex,mousey)) { quitout=1; } }
  }

return 0x00; }

/* End of file, first edition: Brad Smith 1999 */