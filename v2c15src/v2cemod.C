/* Verge 2 Graphical CHR editor modules */

#define gptver     "v1.5"
#define gptrelease "05/22/00"

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
extern unsigned long int cheezytimerstart;
extern int timerinterval;
extern void initctimer();
extern int ctimer();
#include <i86.h>
extern unsigned char shiftstats();

#include "video.h"
#include "vergepal.h"
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
extern unsigned char syspal[768];

extern char stpvmd;
extern char backupon;
extern char backupfile[76];
extern int backuptime;
extern unsigned char gcols[9];
extern chrtemplt chrtdef;

extern unsigned char *cdata;
extern int cdtalloc, cdtused;
extern int cfrmsize;
extern int cframes,cheight,cwidth,chotx,choty,chotw,choth;
extern scriptop scrops[8];
extern scriptqn squant[8];
extern char cfname[75];
extern char *cfront; /* points to 13 character filename */
extern char chrtype; /* v1 or v2 type, 0=v2 1=v1 */
extern int cfup,cfdown,cfleft,cfright;

extern char edmode; extern int edframe;
extern int anistr; extern int anifrm;
extern int aniprt; extern int anitime;
extern int anion;
extern unsigned char ecoll;
extern unsigned char ecolr;
extern char gridon;
extern int cfnamemax;

extern int ecleft; /* left side of CHR edit box */
extern int ecxmax; extern int ecymax; // bottom right corner
extern char scrollxon;
extern char scrollyon;
extern int scroffx;
extern int scroffy;
extern int scrbx1,scrbx2; /* sides of the x scrollbutton */
extern int scrby1,scrby2; /* top and bottom of the y scrollbutton */
extern int scrxmax,scrymax;

extern unsigned char zoom; // goes up to 8 (zoom=3 because it's <<)
extern char zoomcfg;
extern char invon; // whether or not screen colours (0/255) are inverted.

extern char clipstats;
//static char *clipstattxt[3]={"Empty","Frame","Shape"};
extern unsigned char *clipboard;
extern unsigned char *tempchr;
extern unsigned char *undo;

extern void bfons(int x, int y, char* c);
#define nboxwid 220
#define nboxhig 80
extern void ncfieldedit(int x1, int y1, int x2, int y2,
                        char *tex, int len, int n);
extern unsigned short int v1ccnum[8][16];

#define gpcbw 140
#define gpcbh 22

char gpclosebox()
{ int tx,ty,bx,by,xc;
  unsigned char gpp;

  tx=(sw>>1)-(gpcbw>>1)-1;
  ty=(sh>>1)-(gpcbh>>1)-1;
  bx=tx+gpcbw;
  by=ty+gpcbh;
  xc=(sw>>1)-1;

  guisysgrey();

  guidrawb(tx,ty,bx,by,"");
  gpasbfc(xc+1,ty+3,"Quit without saving?",gcols[1]);
  gpasbfc(xc,ty+2,  "Quit without saving?",gcols[0]);

  guidrawb(xc-19,ty+10,xc+2,ty+20,"Yes");
  guidrawb(xc+4,ty+10,xc+19,ty+20,"No");

 regpcb:
  vesabuffer();
  msepoll();
  mox=mousex; moy=mousey;
  mseblit(mousex,mousey);
  while(!kbhit() && !mousek)
  { msepoll();
    if(mousex!=mox || mousey!=moy)
    { msebolt(mox,moy);
      mseblit(mousex,mousey);
  } }
  msebolt(mousex,mousey);

  if(kbhit())
  { gpp=getch();
    if(!gpp)
    { getch(); }
    else
    { if(gpp=='y' || gpp=='Y')
      { guidrawbc(xc-19,ty+10,xc+2,ty+20,"Yes");
        return 0; }
      if(gpp=='n' || gpp=='N')
      { guidrawbc(xc+4,ty+10,xc+19,ty+20,"No");
        return 1; }
    }
    vesabuffer();
  }
  if(mousek)
  { if(mousey>ty+10 && mousey<ty+20)
    { if(mousex>xc-19 && mousex<xc+2)
      { guidrawbc(xc-19,ty+10,xc+2,ty+20,"Yes");
        vesabuffer();
        cehold();
        return 0; }
      else if(mousex>xc+4 && mousex<xc+19)
      { guidrawbc(xc+4,ty+10,xc+19,ty+20,"No");
        vesabuffer();
        cehold();
        return 1; }
    }
    vesabuffer();
    cehold();
  }
 goto regpcb;
return 1; }

void gpnewchr()
{ char ncv[16];
  int i,j;
  int tx,ty;
  chrtemplt nchr;
  unsigned char c;
  unsigned char *newdat;
  char ntype=chrtype;

  memcpy(&nchr,&chrtdef,sizeof(chrtemplt));

  tx=(sw>>1)-(nboxwid>>1);
  ty=(sh>>1)-(nboxhig>>1);

  guisysgrey();
  guidrawb(tx,ty,tx+nboxwid,ty+nboxhig,"Create New CHR");
  guidrawb(tx+nboxwid-42,ty+nboxhig-12,tx+nboxwid-2,ty+nboxhig-2,"Cancel");
  guidrawb(tx+nboxwid-84,ty+nboxhig-12,tx+nboxwid-44,ty+nboxhig-2,"Okay");
  bfons(tx+12,ty+14,"Frames:");
  guidrawbc(tx+54,ty+12,tx+98,ty+22,itoa(nchr.frames,ncv,10));
  bfons(tx+12,ty+26," Width:");
  guidrawbc(tx+54,ty+24,tx+98,ty+34,itoa(nchr.width,ncv,10));
  bfons(tx+12,ty+38,"Height:");
  guidrawbc(tx+54,ty+36,tx+98,ty+46,itoa(nchr.height,ncv,10));
  bfons(tx+12,ty+50,"  Type:");
  guidrawbc(tx+54,ty+48,tx+98,ty+58,ntype?"V1 CHR":"V2 CHR");
  bfons(tx+112,ty+14,"HotspotX:");
  guidrawbc(tx+166,ty+12,tx+210,ty+22,itoa(nchr.hotspotx,ncv,10));
  bfons(tx+112,ty+26,"HotspotY:");
  guidrawbc(tx+166,ty+24,tx+210,ty+34,itoa(nchr.hotspoty,ncv,10));
  bfons(tx+112,ty+38,"HotWidth:");
  guidrawbc(tx+166,ty+36,tx+210,ty+46,itoa(nchr.hotspotwidth,ncv,10));
  bfons(tx+106,ty+50,"HotHeight:");
  guidrawbc(tx+166,ty+48,tx+210,ty+58,itoa(nchr.hotspotheight,ncv,10));

  nexnchrmse:
  vesabuffer();
  msepoll();
  mox=mousex; moy=mousey;
  mseblit(mousex,mousey);
  while(!kbhit() && !mousek)
  { msepoll();
    if(mousex!=mox || mousey!=moy)
    { msebolt(mox,moy);
      mseblit(mousex,mousey);
  } }
  msebolt(mousex,mousey);

  if(mousek)
  { int mx,my;
    mx=mousex-tx;
    my=mousey-ty;
    if(mousex<ty || mousey<ty) { cehold(); }
    else if(mx>54 && mx<98 && my>48 && my<58)
    { ntype=ntype^1;
      guidrawbc(tx+54,ty+48,tx+98,ty+58,ntype?"V1 CHR":"V2 CHR");
      vesabuffer();
      cehold(); }
    else if(mx>54 && mx<98 && my>12 && my<46)
    { int topt,ton;
      topt=my-13;
      ton=topt%12;
      if(ton<9)
      { topt=topt/12;
        if(topt==0)
        { ncfieldedit(tx+54,ty+12,tx+98,ty+22,ncv,4,nchr.frames);
          nchr.frames=atoi(ncv);
          if(nchr.frames<1) nchr.frames=1;
          if(nchr.frames>1024) nchr.frames=1024;
          guidrawbc(tx+54,ty+12,tx+98,ty+22,itoa(nchr.frames,ncv,10)); }
        if(topt==1)
        { ncfieldedit(tx+54,ty+24,tx+98,ty+34,ncv,3,nchr.width);
          nchr.width=atoi(ncv);
          if(nchr.width<1) nchr.width=1;
          if(nchr.width>500) nchr.width=500;
          guidrawbc(tx+54,ty+24,tx+98,ty+34,itoa(nchr.width,ncv,10)); }
        if(topt==2)
        { ncfieldedit(tx+54,ty+36,tx+98,ty+46,ncv,3,nchr.height);
          nchr.height=atoi(ncv);
          if(nchr.height<1) nchr.height=1;
          if(nchr.height>500) nchr.height=500;
          guidrawbc(tx+54,ty+36,tx+98,ty+46,itoa(nchr.height,ncv,10)); }
      } else { cehold(); }
      vesabuffer();
    }
    else if(mx>166 && mx<210 && my>12 && my<58)
    { int topt,ton;
      topt=my-13;
      ton=topt%12;
      if(ton<9)
      { topt=topt/12;
        if(topt==0)
        { ncfieldedit(tx+166,ty+12,tx+210,ty+22,ncv,4,nchr.hotspotx);
          nchr.hotspotx=atoi(ncv);
          if(nchr.hotspotx<-999) nchr.hotspotx=-999;
          if(nchr.hotspotx>9999) nchr.hotspotx=9999;
          guidrawbc(tx+166,ty+12,tx+210,ty+22,itoa(nchr.hotspotx,ncv,10)); }
        if(topt==1)
        { ncfieldedit(tx+166,ty+24,tx+210,ty+34,ncv,4,nchr.hotspoty);
          nchr.hotspoty=atoi(ncv);
          if(nchr.hotspoty<-999) nchr.hotspoty=-999;
          if(nchr.hotspoty>9999) nchr.hotspoty=9999;
          guidrawbc(tx+166,ty+24,tx+210,ty+34,itoa(nchr.hotspoty,ncv,10)); }
        if(topt==2)
        { ncfieldedit(tx+166,ty+36,tx+210,ty+46,ncv,4,nchr.hotspotwidth);
          nchr.hotspotwidth=atoi(ncv);
          if(nchr.hotspotwidth<-999) nchr.hotspotwidth=-999;
          if(nchr.hotspotwidth>9999) nchr.hotspotwidth=9999;
    guidrawbc(tx+166,ty+36,tx+210,ty+46,itoa(nchr.hotspotwidth,ncv,10)); }
        if(topt==3)
        { ncfieldedit(tx+166,ty+48,tx+210,ty+58,ncv,4,nchr.hotspotheight);
          nchr.hotspotheight=atoi(ncv);
          if(nchr.hotspotheight<-999) nchr.hotspotheight=-999;
          if(nchr.hotspotheight>9999) nchr.hotspotheight=9999;
   guidrawbc(tx+166,ty+48,tx+210,ty+58,itoa(nchr.hotspotheight,ncv,10)); }
      } else { cehold(); }
      vesabuffer();
    }
    else if( mx>(nboxwid-42) && mx<(nboxwid-2) &&
             my>(nboxhig-12) && my<(nboxhig-2) )
    { guidrawbc(tx+nboxwid-42,ty+nboxhig-12,tx+nboxwid-2,
                ty+nboxhig-2,"Cancel");
      vesabuffer();
      cehold();
      return; }
    else if(mx>(nboxwid-84) && mx<(nboxwid-44) &&
            my>(nboxhig-12) && my<(nboxhig-2) )
    { guidrawbc(tx+nboxwid-84,ty+nboxhig-12,tx+nboxwid-44,
                ty+nboxhig-2,"Okay");
      vesabuffer();
      cehold();
      goto finnchr; }
    else { cehold(); }
  }
  if(kbhit())
  { c=getch();
    if(c==27) return;
    else if(c==13) goto finnchr;
    else if(c==0) getch();
  }
  goto nexnchrmse;
  finnchr:

  if(ntype==1) /* change values to v1 chr */
  { if(nchr.frames>30) nchr.frames=30;
    nchr.width=16; nchr.height=32; nchr.hotspotx=0; nchr.hotspoty=16;
    nchr.hotspotwidth=16; nchr.hotspotheight=16; }

  if((nchr.width*nchr.height*nchr.frames)>20971520)
  { cls();
    guisysgrey();
    bfons(10,10,"Sorry, your intended CHR would take up far too");
    bfons(10,20,"much memory. Please reduce your dimenstions or");
    bfons(10,30,"frames and try again. Press a key...");
    vesabuffer();
    if(!getch()) getch();
    return; }
  newdat=calloc(nchr.width*nchr.height*(nchr.frames+1),1);
  if(newdat==NULL)
  { eddrwscr();
    guisysgrey();
    guidrawb(10,10,sw-11,20,"Sorry, out of memory. Press a key...");
    vesabuffer();
    if(!getch()) getch();
    return; }
  free(cdata);

  if(nchr.width!=cwidth || nchr.height!=cheight)
  { free(clipboard);
    clipboard=malloc((nchr.width*nchr.height)*3);
    if(clipboard==NULL)
    { free(cdata);
      free(newdat);
      setpal(syspal);
      SDVideo();
      printf("Not enough memory. Aborting program.\n");
      exit(0x02); }
    if((nchr.width!=cwidth)||(nchr.height!=cheight)) { clipstats=0; }
    clipstats=0;
  }

  cfrmsize=nchr.width*nchr.height;
  tempchr=clipboard+cfrmsize;
  undo=tempchr+cfrmsize;
  cframes=nchr.frames; cheight=nchr.height; cwidth=nchr.width;
  chotx=nchr.hotspotx; choty=nchr.hotspoty;
  chotw=nchr.hotspotwidth; choth=nchr.hotspotheight;
  cdata=newdat; /* the hands finally change */
  cdtalloc=(cframes+1)*cfrmsize+1;
  cdtused=cframes*cfrmsize;
  for(i=0;i<8;i++)
  { scrops[i].list[0]=255;
    squant[i].quan[0]=0;   }
  if(ntype==1)
  { strcpy(scrops[0].list,"FWFWFWFWFWFWFWFW");
    strcpy(scrops[1].list,"FWFWFWFWFWFWFWFW");
    strcpy(scrops[2].list,"FWFWFWFWFWFWFWFW");
    strcpy(scrops[3].list,"FWFWFWFWFWFWFWFW");
    strcpy(scrops[4].list,"FW");  strcpy(scrops[5].list,"FW");
    strcpy(scrops[6].list,"FW"); strcpy(scrops[7].list,"FW");
    for(i=0;i<4;i++) { scrops[i].list[16]=255; }
    for(i=4;i<8;i++) { scrops[i].list[2]=255; }
    for(i=0; i<8; i++) { memcpy(&squant[i].quan,&v1ccnum[i],32); }
    cfleft=15; cfright=10; cfup=5; cfdown=0;
  }

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

  anistr=0;
  anifrm=0;
  aniprt=0;
  anion=1;
  anitime=0;
  if(edmode==1) anion=0;

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

  memcpy(undo,cdata,cfrmsize); /* preload the first frame */
  vesabuffer();

return; }

extern unsigned char edpal[768];
static char* exscrtx[4]={"lscript=","rscript=","uscript=","dscript="};

void gpexportpcx()
{ int tx,ty,bx,by;
  char pcxn[15]="DEFAULT.PCX";
  char makn[15]="DEFAULT.MAK";
  char k,okcan;

  okcan=0;

  guisysgrey();
  tx=(sw>>1)-90;
  ty=(sh>>1)-24;
  bx=tx+180;
  by=ty+48;

  guidrawb(tx,ty,bx,by,"Export PCX/MAK");
  bfons(tx+10,ty+12,"PCX Filename:");
  bfons(tx+10,ty+24,"MAK Filename:");
  guidrawbc(tx+90,ty+12,tx+178,ty+22,pcxn);
  guidrawbc(tx+90,ty+24,tx+178,ty+34,makn);
  guidrawb(tx+55,ty+36,tx+84,ty+46,"Okay");
  guidrawb(tx+86,ty+36,tx+126,ty+46,"Cancel");

 nexpcx:
  vesabuffer();
  k=0; msepoll(); mox=mousex; moy=mousey; mseblit(mousex,mousey);
  while(!kbhit() && !mousek)
  { msepoll(); if(mousex!=mox || mousey!=moy)
    { msebolt(mox,moy);  mseblit(mousex,mousey); }
  } msebolt(mousex,mousey);
  if(kbhit())
  { char k=getch();
    if(k==13) { okcan=1; goto enpcx; }
    if(k==27) { okcan=0; goto enpcx; }
    else if (!k) { getch(); } }
  if(mousek)
  { int mx,my;
    mx=mousex; my=mousey;
    mx-=tx; my-=ty;
    if(mx>90 && mx<178 && my>12 && my<22)
    { txfieldedit(tx+90,ty+12,tx+178,ty+22,pcxn,12);
      guidrawbc(tx+90,ty+12,tx+178,ty+22,pcxn); }
    if(mx>90 && mx<178 && my>24 && my<34)
    { txfieldedit(tx+90,ty+24,tx+178,ty+34,makn,12);
      guidrawbc(tx+90,ty+24,tx+178,ty+34,makn); }
    else if(mx>55 && mx<84 && my>36 && my<46)
    { guidrawbc(tx+55,ty+36,tx+84,ty+46,"Okay");
      vesabuffer();
      cehold(); okcan=1; goto enpcx; }
    else if(mx>86 && mx<126 && my>36 && my<46)
    { guidrawbc(tx+86,ty+36,tx+126,ty+46,"Cancel");
      vesabuffer();
      cehold(); okcan=0; goto enpcx; }
    else cehold();
    vesabuffer();
  }
  goto nexpcx;
 enpcx:

  if(okcan)
  { unsigned char* tbff;
    int pcol,prow,pwid,phig;
    unsigned char* tbp;
    FILE *mf;
    int i,j,l,m;
    char mbf[1024];

    guisysgrey();
    vesabuffer();
    if(cwidth>318) { pcol=1; pwid=cwidth+2; }
    else { pwid=320; pcol=320/(cwidth+1); } /* keep small chrs to 320x# */
    prow=(cframes/pcol); /* This rounds down. */
    if(cframes%pcol) prow++; /* Just make sure. */
    phig=prow*(cheight+1)+1;
    if(phig<200) phig=200; /* again, keep the small ones to #x200 */

    tbff=NULL;
    tbff=malloc(pwid*phig);
    if(tbff==NULL)
    { guidrawb(10,10,sw-11,20,"Out of memory, export aborted. Press a key.");
      getch();
      return; }
    memset(tbff,255,pwid*phig); /* Fills the space. */

    for(i=0;i<prow;i++)
    { for(j=0;j<pcol;j++)
      { if(((i*pcol)+j)<cframes)
        { tbp=cdata+(((i*pcol)+j)*cfrmsize);
          for(l=0;l<cheight;l++)
          { memcpy(tbff+(pwid*((i*(cheight+1))+1+l))+((cwidth+1)*j)+1,tbp,cwidth);
            tbp+=cwidth; }
        }
    } }

    if(savepcx(pcxn,tbff,pwid,phig,edpal))
    { free(tbff);
      guidrawb(10,10,sw-11,20,"Could not save PCX. Press a key.");
      vesabuffer();
      getch();
      return; }
    free(tbff);

    mf=NULL; mf=fopen(makn,"wt");
    if(mf==NULL)
    { guidrawb(10,10,sw-11,20,"PCX saved, but not the makefile. Press a key.");
      vesabuffer();
      getch();
      return; }

    fputs("/*         Character Makefile        */\n",mf);
    fputs("/* Generated by Rane's V2 CHR editor */\n",mf);
    fputs("\n",mf);

    strcpy(mbf,"pcx_name=");
    if(strcmp(pcxn+strlen(pcxn)-4,".PCX"))
    { guidrawb(10,10,sw-11,20,"PCX must have a '.PCX' extension. Press a key.");
      fclose(mf); getch(); return; }
    strcpy(mbf+strlen(mbf),pcxn);
    mbf[strlen(mbf)-4]=0;
    fputs(mbf,mf); fputs(";\n",mf);
    strcpy(mbf,"chr_name=");
    strcpy(mbf+strlen(mbf),pcxn);
    mbf[strlen(mbf)-4]=0;
    fputs(mbf,mf); fputs(";\n",mf);

    strcpy(mbf,"frame_w=");
    itoa(cwidth,mbf+strlen(mbf),10);
    fputs(mbf,mf); fputs(";\n",mf);
    strcpy(mbf,"frame_h=");
    itoa(cheight,mbf+strlen(mbf),10);
    fputs(mbf,mf); fputs(";\n",mf);

    strcpy(mbf,"hot_x=");
    itoa(chotx,mbf+strlen(mbf),10);
    fputs(mbf,mf); fputs(";\n",mf);
    strcpy(mbf,"hot_y=");
    itoa(choty,mbf+strlen(mbf),10);
    fputs(mbf,mf); fputs(";\n",mf);
    strcpy(mbf,"hot_w=");
    itoa(chotw,mbf+strlen(mbf),10);
    fputs(mbf,mf); fputs(";\n",mf);
    strcpy(mbf,"hot_h=");
    itoa(choth,mbf+strlen(mbf),10);
    fputs(mbf,mf); fputs(";\n",mf);

    strcpy(mbf,"per_row=");
    itoa(pcol,mbf+strlen(mbf),10);
    fputs(mbf,mf); fputs(";\n",mf);
    strcpy(mbf,"total_frames=");
    itoa(cframes,mbf+strlen(mbf),10);
    fputs(mbf,mf); fputs(";\n",mf);

    strcpy(mbf,"lidle=");
    itoa(cfleft,mbf+strlen(mbf),10);
    fputs(mbf,mf); fputs(";\n",mf);
    strcpy(mbf,"ridle=");
    itoa(cfright,mbf+strlen(mbf),10);
    fputs(mbf,mf); fputs(";\n",mf);
    strcpy(mbf,"uidle=");
    itoa(cfup,mbf+strlen(mbf),10);
    fputs(mbf,mf); fputs(";\n",mf);
    strcpy(mbf,"didle=");
    itoa(cfdown,mbf+strlen(mbf),10);
    fputs(mbf,mf); fputs(";\n",mf);

    for(i=0;i<4;i++)
    { char* scrtx;
      int scrlen;
      strcpy(mbf,exscrtx[i]);
      scrtx=mbf+strlen(mbf);
      scrlen=0; scrtx[0]=0; l=0;
      for(j=0;j<200;j++)
      { if(scrops[i].list[j]==255) { scrlen=j; j=201; break; } }
      for(j=0;j<scrlen;j++)
      { scrtx[l]=scrops[i].list[j]; l++; /* Always F or W */
        itoa(squant[i].quan[j],scrtx+l,10);
        l+=strlen(scrtx+l); }
      scrlen=strlen(scrtx)+1;
      fputs(mbf,mf); fputs(";\n",mf);
    }
    fputs("\n",mf);
    fputs("/* End of generated file. */\n",mf);

    fclose(mf);    
    guidrawb(10,10,sw-11,20,"PCX/MAK export successful. Press a key.");
    vesabuffer();
    getch();
  return; }
  else return;
return; }

#define gpcsw 200
#define gpcsh 110
void gpchrstat()
{ int tx,ty,bx,by;
  char ncv[16];

  tx=(sw>>1)-(gpcsw>>1); bx=tx+gpcsw;
  ty=(sh>>1)-(gpcsh>>1); by=ty+gpcsh;
  guisysgrey();
  guidrawb(tx,ty,bx,by,"CHR Statistics");
  guidrawb(bx-31,by-14,bx-4,by-4,"Okay");
  guidrawb(tx+4,ty+14,bx-4,ty+50,"Idle Frames");
  bfons(tx+8,ty+27,"   Up"); bfons(tx+8,ty+39," Down");
  bfons(tx+100,ty+27," Left"); bfons(tx+100,ty+39,"Right");
  guidrawbc(tx+40,ty+25,tx+90,ty+35,itoa(cfup,ncv,10));
  guidrawbc(tx+40,ty+37,tx+90,ty+47,itoa(cfdown,ncv,10));
  guidrawbc(tx+132,ty+25,tx+182,ty+35,itoa(cfleft,ncv,10));
  guidrawbc(tx+132,ty+37,tx+182,ty+47,itoa(cfright,ncv,10));
  guidrawb(tx+4,ty+53,bx-4,ty+92,"Hotspot");
  bfons(tx+32,ty+66,"X"); bfons(tx+32,ty+78,"Y");
  guidrawbc(tx+40,ty+64,tx+90,ty+74,itoa(chotx,ncv,10));
  guidrawbc(tx+40,ty+76,tx+90,ty+86,itoa(choty,ncv,10));
  bfons(tx+100,ty+66,"Width"); bfons(tx+94,ty+78,"Height");
  guidrawbc(tx+132,ty+64,tx+182,ty+74,itoa(chotw,ncv,10));
  guidrawbc(tx+132,ty+76,tx+182,ty+86,itoa(choth,ncv,10));

  nexcstatmse:
  vesabuffer();
  msepoll(); mox=mousex; moy=mousey;
  mseblit(mousex,mousey); while(!kbhit() && !mousek)
  { msepoll(); if(mousex!=mox || mousey!=moy)
  { msebolt(mox,moy);  mseblit(mousex,mousey); } } msebolt(mousex,mousey);
  if(kbhit())
  { char k=getch(); if(k==13 || k==27) { goto encstatmse; }
    else if (!k) { getch(); } }
  if(mousek)
  { int mx,my; mx=mousex; my=mousey; mx-=tx; my-=ty;
    if(mx<0 || my<0 || mx>gpcsw || my>gpcsh) { cehold(); goto nexcstatmse; }
    else if(mx>40 && mx<90 && my>25 && my<35)
    { ncfieldedit(tx+40,ty+25,tx+90,ty+35,ncv,4,cfup);
      cfup=atoi(ncv); if(cfup<0) cfup=0; if(cfup>=cframes) cfup=cframes-1;
      guidrawbc(tx+40,ty+25,tx+90,ty+35,itoa(cfup,ncv,10)); }
    else if(mx>40 && mx<90 && my>37 && my<47)
    { ncfieldedit(tx+40,ty+37,tx+90,ty+47,ncv,4,cfdown);
      cfdown=atoi(ncv); if(cfdown<0) cfdown=0;
      if(cfdown>=cframes) cfdown=cframes-1;
      guidrawbc(tx+40,ty+37,tx+90,ty+47,itoa(cfdown,ncv,10)); }
    else if(mx>132 && mx<182 && my>25 && my<35)
    { ncfieldedit(tx+132,ty+25,tx+182,ty+35,ncv,4,cfleft);
      cfleft=atoi(ncv); if(cfleft<0) cfleft=0;
      if(cfleft>=cframes) cfleft=cframes-1;
      guidrawbc(tx+132,ty+25,tx+182,ty+35,itoa(cfleft,ncv,10)); }
    else if(mx>132 && mx<182 && my>37 && my<47)
    { ncfieldedit(tx+132,ty+37,tx+182,ty+47,ncv,4,cfright);
      cfright=atoi(ncv); if(cfright<0) cfright=0;
      if(cfright>=cframes) cfright=cframes-1;
      guidrawbc(tx+132,ty+37,tx+182,ty+47,itoa(cfright,ncv,10)); }

    else if(mx>40 && mx<90 && my>64 && my<74)
    { ncfieldedit(tx+40,ty+64,tx+90,ty+74,ncv,4,chotx);
      chotx=atoi(ncv); if(chotx<-999) chotx=-999; if(chotx>9999) chotx=9999;
      guidrawbc(tx+40,ty+64,tx+90,ty+74,itoa(chotx,ncv,10)); }
    else if(mx>40 && mx<90 && my>76 && my<86)
    { ncfieldedit(tx+40,ty+76,tx+90,ty+86,ncv,4,choty);
      choty=atoi(ncv); if(choty<-999) choty=-999; if(choty>9999) choty=9999;
      guidrawbc(tx+40,ty+76,tx+90,ty+86,itoa(choty,ncv,10)); }
    else if(mx>132 && mx<182 && my>64 && my<74)
    { ncfieldedit(tx+132,ty+64,tx+182,ty+74,ncv,4,chotw);
      chotw=atoi(ncv); if(chotw<-999) chotw=-999; if(chotw>9999) chotw=9999;
      guidrawbc(tx+132,ty+64,tx+182,ty+74,itoa(chotw,ncv,10)); }
    else if(mx>132 && mx<182 && my>76 && my<86)
    { ncfieldedit(tx+132,ty+76,tx+182,ty+86,ncv,4,choth);
      choth=atoi(ncv); if(choth<-999) choth=-999; if(choth>9999) choth=9999;
      guidrawbc(tx+132,ty+76,tx+182,ty+86,itoa(choth,ncv,10)); }

    else if(mousex>bx-31 && mousex<bx-4 && mousey>by-15 && mousey<by-4)
    { guidrawbc(bx-31,by-14,bx-4,by-4,"Okay");
      vesabuffer();
      cehold(); guidrawb(bx-31,by-14,bx-4,by-4,"Okay");
      goto encstatmse; }
    else cehold();
    vesabuffer();
  }
  goto nexcstatmse;
 encstatmse:
return; }

static char *scrnames[8] = { "Left","Right","Up","Down",
                             "Idle-L","Idle-R","Idle-U","Idle-D" };
static char *scrtyp[4] = { "","End","Frame","Wait" };
#define scrb 100
void scrdani()
{ int i,j;
  if(anifrm<cframes)
  { for(j=0;j<cheight;j++) { for(i=0;i<cwidth;i++)
    { plot(i+scrb+3,j,cdata[(anifrm*cfrmsize)+(j*cwidth)+i]); }}
  }
  else { box(scrb+3,0,scrb+2+cwidth,cheight-1,gcols[4]); }
  vesabuffer();
return; }
void scripted()
{ int screm,scoff,scrlen,scodisp;
  int anictime=0;
  int i,j;
  char nstr[10];

  anifrm=cframes; aniprt=0;
  anitime=0; scoff=0;
  screm=1; scrlen=0;
  for(i=0;i<200;i++)
  { if(scrops[anistr].list[i]==255) { scrlen=i+1; i=201; break; } }
  scodisp=(sh/10)-4; /* how many script elements are displayed.*/
  cls();
  guidrawb(0,0,scrb,sh-1,"Script Editor");
  guidrawb(2,10,28,20,"Exit");
  guidrawb(30,10,40,20,"<");
  guidrawb(scrb-12,10,scrb-2,20,">");
  guidrawb(42,10,scrb-14,20,scrnames[anistr]);
  guidrawbd(scrb-12,(scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
  if((scoff+scodisp)<scrlen) guidrawb(scrb-12,
     (scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
  guidrawb(scrb-12,30,scrb-2,39,"Þ");
  vesabuffer();

  for(i=0;i<scodisp;i++)
  { j=0; if(scrops[anistr].list[i+scoff]=='F') j=2;
    if(scrops[anistr].list[i+scoff]=='W') j=3;
    if(scrops[anistr].list[i+scoff]==255) j=1;
    if((i+scoff)>=scrlen) j=0;
    guidrawbc(3,(i+3)*10,36,((i+4)*10)-1,"");
    guidrawbc(38,(i+3)*10,scrb-14,((i+4)*10)-1,"");
    blitfont(5,((i+3)*10)+2,scrtyp[j],gcols[0]);
    if(j>1)
    { blitfont(40,((i+3)*10)+2,itoa(squant[anistr].quan[i+scoff],nstr,10),
               gcols[0]); } }

  if((cwidth>(sw-(scrb+5))) || (cheight>sh))
  { bfons(scrb+5,(sh>>1)-3,"CHR is too large to display."); screm=0; }
  if(screm)
  { if(scrops[anistr].list[aniprt]=='F')
    { anifrm=squant[anistr].quan[aniprt]; }
    else if(scrops[anistr].list[aniprt]=='W')
    { anitime=squant[anistr].quan[aniprt]; }
    else if(scrops[anistr].list[aniprt]==255) { aniprt=0; } aniprt++; 
    if(cheight<sh) { guidrawb(scrb+2,0,scrb+3+cwidth,cheight,""); }
    else if(cheight==sh) { guidrawb(scrb+2,0,scrb+2+cwidth,sh-1,""); }
    if(screm) { scrdani(); } }

  anictime=ctimer();
  scredblah:
  vesabuffer();
  msepoll();
  mox=mousex; moy=mousey;
  mseblit(mousex,mousey);
  while(!kbhit() && !mousek)
  { msepoll(); if(mousex!=mox || mousey!=moy)
    { msebolt(mox,moy);  mseblit(mousex,mousey);  }
    if(screm) { if(anitime) { if((ctimer()-anictime))
          { anitime-=(ctimer()-anictime); if(anitime<0) anitime=0; 
            anictime=ctimer(); } }
        else { if(scrops[anistr].list[aniprt]=='F')
          { anifrm=squant[anistr].quan[aniprt]; 
  if(scrlen>1) {msebolt(mousex,mousey); if(screm) { scrdani(); }
                mseblit(mousex,mousey);}}
          else if(scrops[anistr].list[aniprt]=='W')
          { anitime=squant[anistr].quan[aniprt]; }
          else if(scrops[anistr].list[aniprt]==255) { aniprt=-1; }
        aniprt++; } } /* compacterized for my convenienece.*/ }
  msebolt(mousex,mousey);

  if(mousek)
  { if(mousey>10 && mousey<20) /* top bar */
    { if(mousex>2 && mousex<28)
      { guidrawbc(2,10,28,20,"Exit"); vesabuffer(); cehold(); return; }
      else if(mousex>30 && mousex<40)
      { guidrawbc(30,10,40,20,"<");
        anistr--; if(anistr<0) anistr=3;
        aniprt=0; anifrm=cframes; anitime=0; scoff=0; scrlen=0;
        for(i=0;i<200;i++)
        { if(scrops[anistr].list[i]==255) { scrlen=i+1; i=201; break; } }
        if(screm) { scrdani(); }
        vesabuffer();
        cehold();
        for(i=0;i<scodisp;i++)
        { j=0; if(scrops[anistr].list[i+scoff]=='F') j=2;
          if(scrops[anistr].list[i+scoff]=='W') j=3;
          if(scrops[anistr].list[i+scoff]==255) j=1;
          if((i+scoff)>=scrlen) j=0;
          guidrawbc(3,(i+3)*10,36,((i+4)*10)-1,"");
          guidrawbc(38,(i+3)*10,scrb-14,((i+4)*10)-1,"");
          blitfont(5,((i+3)*10)+2,scrtyp[j],gcols[0]); if(j>1)
     { blitfont(40,((i+3)*10)+2,itoa(squant[anistr].quan[i+scoff],nstr,10),
       gcols[0]); } }
        guidrawbd(scrb-12,(scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
        if((scoff+scodisp)<scrlen) guidrawb(scrb-12,
          (scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
        guidrawb(30,10,40,20,"<");
        guidrawb(42,10,scrb-14,20,scrnames[anistr]); }
      else if(mousex>(scrb-12) && mousex<(scrb-2))
      { guidrawbc(scrb-12,10,scrb-2,20,">");
        anistr++; if(anistr>3) anistr=0;
        aniprt=0; anifrm=cframes; anitime=0; scoff=0; scrlen=0;
        for(i=0;i<200;i++)
        { if(scrops[anistr].list[i]==255) { scrlen=i+1; i=201; break; } }
        if(screm) { scrdani(); }
        vesabuffer();
        cehold();
        for(i=0;i<scodisp;i++)
        { j=0; if(scrops[anistr].list[i+scoff]=='F') j=2;
          if(scrops[anistr].list[i+scoff]=='W') j=3;
          if(scrops[anistr].list[i+scoff]==255) j=1;
          if((i+scoff)>=scrlen) j=0;
          guidrawbc(3,(i+3)*10,36,((i+4)*10)-1,"");
          guidrawbc(38,(i+3)*10,scrb-14,((i+4)*10)-1,"");
          blitfont(5,((i+3)*10)+2,scrtyp[j],gcols[0]); if(j>1)
     { blitfont(40,((i+3)*10)+2,itoa(squant[anistr].quan[i+scoff],nstr,10),
       gcols[0]); } }
        guidrawbd(scrb-12,(scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
        if((scoff+scodisp)<scrlen) guidrawb(scrb-12,
           (scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
        guidrawb(scrb-12,10,scrb-2,20,">");
        guidrawb(42,10,scrb-14,20,scrnames[anistr]); }
      else cehold();
      vesabuffer();
    }
    else if(mousex>scrb-12 && mousex<scrb-2 && mousey>30 && mousey<39)
    { guidrawbc(scrb-12,30,scrb-2,39,"Þ"); if(scoff) scoff--;
      vesabuffer();
      cehold();
      for(i=0;i<scodisp;i++)
      { j=0; if(scrops[anistr].list[i+scoff]=='F') j=2;
        if(scrops[anistr].list[i+scoff]=='W') j=3;
        if(scrops[anistr].list[i+scoff]==255) j=1;
        if((i+scoff)>=scrlen) j=0;
        guidrawbc(3,(i+3)*10,36,((i+4)*10)-1,"");
        guidrawbc(38,(i+3)*10,scrb-14,((i+4)*10)-1,"");
        blitfont(5,((i+3)*10)+2,scrtyp[j],gcols[0]); if(j>1)
     { blitfont(40,((i+3)*10)+2,itoa(squant[anistr].quan[i+scoff],nstr,10),
                gcols[0]); } }
       guidrawbd(scrb-12,(scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
       if((scoff+scodisp)<scrlen) guidrawb(scrb-12,
          (scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
       guidrawb(scrb-12,30,scrb-2,39,"Þ"); }
    else if(mousex>scrb-12 && mousex<scrb-2 &&
            mousey>((scodisp+2)*10) && mousey<(((scodisp+3)*10)-1))
    { guidrawbc(scrb-12,(scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
      if((scoff+scodisp)<scrlen) { scoff++; }
      vesabuffer();
      cehold();
      for(i=0;i<scodisp;i++)
      { j=0; if(scrops[anistr].list[i+scoff]=='F') j=2;
        if(scrops[anistr].list[i+scoff]=='W') j=3;
        if(scrops[anistr].list[i+scoff]==255) j=1;
        if((i+scoff)>=scrlen) j=0;
        guidrawbc(3,(i+3)*10,36,((i+4)*10)-1,"");
        guidrawbc(38,(i+3)*10,scrb-14,((i+4)*10)-1,"");
        blitfont(5,((i+3)*10)+2,scrtyp[j],gcols[0]); if(j>1)
     { blitfont(40,((i+3)*10)+2,itoa(squant[anistr].quan[i+scoff],nstr,10),
                gcols[0]); } }
      guidrawbd(scrb-12,(scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
      if((scoff+scodisp)<scrlen)
      guidrawb(scrb-12,(scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
    }
    else if(mousex>3 && mousex<36 && mousey>30 &&
            mousey<(((scodisp+3)*10)-1))
    { int ob,rob; char gk=mousek;
      ob=mousey-31; rob=ob%10;
      vesabuffer();
      cehold();
      if(rob<8)
      { ob=ob/10; rob=ob+scoff;
       if(rob<=scrlen)
       {if(scrops[anistr].list[rob]=='F' && (gk&1))
        { scrops[anistr].list[rob]='W'; }
        else if(scrops[anistr].list[rob]=='W' && (gk&1))
        { scrops[anistr].list[rob]='F'; 
          if(squant[anistr].quan[rob]>=cframes)
          squant[anistr].quan[rob]=cframes-1;
        }
        else if(scrops[anistr].list[rob]==255)
        { if(scrlen<199) { scrops[anistr].list[rob]='F';
                           if(rob&1) scrops[anistr].list[rob]='W';
                           squant[anistr].quan[rob]=0;
                           scrops[anistr].list[rob+1]=255; 
                           scrlen++; }
          if(scrlen==2) /* MODIFICATION: Keeps new scripts from flicker. */
          { scrops[anistr].list[1]='W';
            squant[anistr].quan[1]=20;
            scrops[anistr].list[2]=255;
            scrlen++; }
        }
        else if((scrops[anistr].list[rob]=='F' ||
                 scrops[anistr].list[rob]=='W') && (gk&2))
        { scrops[anistr].list[rob]=255; scrlen=rob+1; }
        for(i=0;i<scodisp;i++)
        { j=0; if(scrops[anistr].list[i+scoff]=='F') j=2;
          if(scrops[anistr].list[i+scoff]=='W') j=3;
          if(scrops[anistr].list[i+scoff]==255) j=1;
          if((i+scoff)>=scrlen) j=0;
          guidrawbc(3,(i+3)*10,36,((i+4)*10)-1,"");
          guidrawbc(38,(i+3)*10,scrb-14,((i+4)*10)-1,"");
          blitfont(5,((i+3)*10)+2,scrtyp[j],gcols[0]); if(j>1)
          { blitfont(40,((i+3)*10)+2,itoa(squant[anistr].quan[i+scoff],
                     nstr,10),gcols[0]); }}
        guidrawbd(scrb-12,(scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
        if((scoff+scodisp)<scrlen)
        guidrawb(scrb-12,(scodisp+2)*10,scrb-2,((scodisp+3)*10)-1,"ß");
      } }
      vesabuffer();
    }
    else if(mousex>38 && mousex<(scrb-14) && mousey>30 &&
            mousey<(((scodisp+3)*10)-1))
    { int ob,rob; char gk=mousek;
      ob=mousey-31; rob=ob%10;
      vesabuffer();
      cehold();
      if(rob<8)
      { ob=ob/10; rob=ob+scoff;
        if(rob<(scrlen-1))
        { ncfieldedit(38,(3+ob)*10,scrb-14,((4+ob)*10)-1,nstr,4,
                                        squant[anistr].quan[rob]);
          vesabuffer();
          squant[anistr].quan[rob]=atoi(nstr);
          if(squant[anistr].quan[rob]<0) squant[anistr].quan[rob]=0;
          if(scrops[anistr].list[rob]=='F')
          {  if(squant[anistr].quan[rob]>=cframes)
             squant[anistr].quan[rob]=cframes-1; }
          else
          {  if(squant[anistr].quan[rob]>9999)
             squant[anistr].quan[rob]=9999; }
        }
        for(i=0;i<scodisp;i++)
        { j=0; if(scrops[anistr].list[i+scoff]=='F') j=2;
          if(scrops[anistr].list[i+scoff]=='W') j=3;
          if(scrops[anistr].list[i+scoff]==255) j=1;
          if((i+scoff)>=scrlen) j=0;
          guidrawbc(38,(i+3)*10,scrb-14,((i+4)*10)-1,"");
          if(j>1)
          { blitfont(40,((i+3)*10)+2,itoa(squant[anistr].quan[i+scoff],
                     nstr,10),gcols[0]); }}
      } 
      vesabuffer();
    }
    else cehold();
  }
  if (kbhit())
  { unsigned char c;
    c=getch();
    if(c==27 || c==13) { return; }
    else if(c==0) getch();
  }
  goto scredblah;
  bfons(5,5,"This is a bug. E-mail me how it happened please."); 
  vesabuffer();
  getch();
return; }

void pickframe()
{ char strc[10];
  int pfrm=0;
  int i,j,k,l,xo,yo;
  unsigned char* cof;
  int doff,drow,dcol;
  int dh=cheight+1;
  int dw=cwidth+1;
  int dsx,dsy,dfx,dfy;

  doff=0;
  dcol=(sw-5)/dw;
  drow=(sh-15)/dh;
  dsx=1; dsy=13; dfx=(dcol*dw)-1; dfy=(drow*dh)+11;
  cls();

 repfd:
  itoa(pfrm,strc,10);
  guidrawb(0,0,40,10,strc);
  guidrawb(42,0,52,10,"<");
  if(doff<(cframes-1)) guidrawb(sw-11,0,sw-1,10,">");
  else guidrawbd(sw-11,0,sw-1,10,">");
  guidrawb(0,12,(dcol*dw)+1,(drow*dh)+13,"");
  for(i=0;i<drow;i++)
  { for(j=0;j<dcol;j++)
    { if((doff+(j*drow)+i)<cframes)
      { xo=(j*dw)+1; yo=(i*dh)+13;
        cof=cdata+((doff+(j*drow)+i)*cfrmsize);
        for(l=0;l<cheight;l++)
        { for(k=0;k<cwidth;k++) { plot(xo+k,yo+l,cof[0]); cof++; } }
  } } }

 repfm:
  vesabuffer();
  msepoll();
  mox=mousex; moy=mousey;
  mseblit(mousex,mousey);
  while(!kbhit() && !mousek)
  { msepoll();
    if(mousex!=mox || mousey!=moy)
    { msebolt(mox,moy);
      guidrawb(0,0,40,10,"");
      if(mousex>=dsx && mousey>=dsy && mousex<=dfx && mousey<=dfy)
      { int rx,ry,rf;
        if((((mousex-dsx)%dw)!=dw-1) && (((mousey-dsy)%dh)!=dh-1))
        { rx=(mousex-dsx)/dw; ry=(mousey-dsy)/dh; rf=(rx*drow)+ry+doff;
          if(rf<cframes){ itoa(rf,strc,10); guidrawb(0,0,40,10,strc);
          vesabuffer(); }
      } }
      mseblit(mousex,mousey);
  } }
  msebolt(mousex,mousey);

  if(mousek)
  { if(mousex>42 && mousex<52 && mousey && mousey<10)
    { guidrawbc(42,0,52,10,"<");
      vesabuffer();
      cehold();
      if(doff) { doff-=drow;
                 if(doff<0) doff=0; }
    goto repfd; }
    if(mousex>sw-11 && mousex<sw-1 && mousey && mousey<10 && doff<(cframes-1))
    { guidrawbc(sw-11,0,sw-1,10,">");
      vesabuffer();
      cehold();
      doff+=drow;
      if(doff>=cframes)
      { doff=(cframes-1)-((cframes-1)%drow); }
    goto repfd; }
    else cehold();
    if(mousex>=dsx && mousey>=dsy && mousex<=dfx && mousey<=dfy)
    { int rx,ry,rf;
      if((((mousex-dsx)%dw)!=dw-1) && (((mousey-dsy)%dh)!=dh-1))
      { rx=(mousex-dsx)/dw; ry=(mousey-dsy)/dh; rf=(rx*drow)+ry+doff;
        if(rf<cframes){ edframe=rf; return; }
      }
    vesabuffer();
    }
  }
  if(kbhit()) 
  { char c;
    c=getch();
    if(c==27) return;
    if(c==0) { getch(); }
  }
 goto repfm;

return; }

void v2cescreensaver()
{ int i,j; unsigned int k;
  char opal[768]; char anipl[1536];
  char *tenipl;
  int r,b,g;
  int tim,ttim; int poff;

  getpal(opal);
  srand(ctimer());
moress:
  poff=0;
  ttim=ctimer();
  r=rand()&63; g=rand()&63; b=rand()&63;
  tenipl=anipl;
  for(i=0;i<64;i++)
  { tenipl[i*3]=(r*i)>>6; tenipl[(i*3)+1]=(g*i)>>6;
    tenipl[(i*3)+2]=(b*i)>>6; tenipl[(127-i)*3]=(r*i)>>6;
    tenipl[((127-i)*3)+1]=(g*i)>>6; tenipl[((127-i)*3)+2]=(b*i)>>6; }
  r=rand()&63; g=rand()&63; b=rand()&63;
  tenipl+=(128*3); for(i=0;i<64;i++)
  { tenipl[i*3]=(r*i)>>6; tenipl[(i*3)+1]=(g*i)>>6;
    tenipl[(i*3)+2]=(b*i)>>6; tenipl[(127-i)*3]=(r*i)>>6;
    tenipl[((127-i)*3)+1]=(g*i)>>6; tenipl[((127-i)*3)+2]=(b*i)>>6; }
  anipl[0]=0; anipl[1]=0; anipl[2]=0;
  cls();
  setpal(anipl); memcpy(anipl+768,anipl,768); tenipl=anipl; poff=0;
  k=rand()&255; if(k==0) k=255;
  plot(0,0,k); k=getpix(0,0);
  for(i=1;i<sw;i++) { if(rand()&1) k++; else k--;
                      if(k==0) k=255; plot(i,0,k); }
  plot(0,1,getpix(0,0)+1);
  for(j=1;j<sh;j++)
  { for(i=0;i<sw;i++)
    { if(j==1 && i==0) i++;
      k=getpix(i-1,j); k+=getpix(i,j-1); k=k>>1;
      if(rand()&1) k--; else k++; if(k==0) k=255;
      plot(i,j,k); } }
  vesabuffer();
  msepoll(); tim=ctimer(); mox=mousex; moy=mousey;
  while(!mousek && !kbhit() && mox==mousex && moy==mousey)
  { msepoll(); 
    if((ctimer()-tim)>5)
    { tenipl[0]=r; tenipl[1]=g; tenipl[2]=b;
      poff+=3; if(poff==768) poff=0;  tenipl=anipl+poff;
      r=tenipl[0]; g=tenipl[1]; b=tenipl[2];
      memset(tenipl,0,3);
      setpal(tenipl); tim=ctimer(); }
    if(ctimer()-ttim>1500) { goto moress; }
  }
  if(kbhit()) { if(!getch()) getch(); }
  cls(); setpal(opal);
  eddrwscr();
return; }

#define secretxt "Initiate the secret large breasted stuff? Y/N"
void secret(); // Ooh sheecret schtuff

void gpabout()
{ int tx,ty,txc;

  guisysgrey();

  tx=(sw>>1)-71;
  ty=(sh>>1)-25;
  txc=tx+70;

  guidrawb(tx,ty,tx+140,ty+50,"");
  gpasbfc(txc+1,ty+3,"About",gcols[1]);
  gpasbfc(txc+1,ty+14,"V2 CHR Graphic Editor",gcols[1]);
  gpasbfc(txc+1,ty+22,gptver,gcols[1]);
  gpasbfc(txc+1,ty+30,"Brad Smith",gcols[1]);
  gpasbfc(txc+1,ty+38,gptrelease,gcols[1]);
  gpasbfc(txc,ty+2,"About",gcols[0]);
  gpasbfc(txc,ty+13,"V2 CHR Graphic Editor",gcols[0]);
  gpasbfc(txc,ty+21,gptver,gcols[0]);
  gpasbfc(txc,ty+29,"Brad Smith",gcols[0]);
  gpasbfc(txc,ty+37,gptrelease,gcols[0]);

  vesabuffer();
  msepoll();
  mox=mousex; moy=mousey;
  mseblit(mousex,mousey);
  while(!kbhit() && !mousek)
  { msepoll();
    if(mousex!=mox || mousey!=moy)
    { msebolt(mox,moy);
      mseblit(mousex,mousey);
  } }
  msebolt(mousex,mousey);

  if(mousek)
  { if((mousex<10 && mousey>(sh-5)) || (mousex==tx && mousey==ty+50))
    { cehold();
      if(shiftstats()&0x04) /* if control is pressed */
      { char c; guisysgrey();
        guidrawb(10,10,sw-11,20,secretxt);
        vesabuffer();
        c=getch(); if(c=='y' || c=='Y') { secret(); cls(); }
      }
    }
    else cehold();
  }
  if(kbhit()) { if(!getch()) getch(); }

  eddrwscr();
return; }

/* Oh hoh... there's something special left over... hmm... */
#include "secret.h" // comment this line when not needed.
void secret()
{ int i,j,k,t,c;
  char opal[768];
  cls();
  getpal(opal);
  setpal(secpal);
 blitfont((sw>>1)-138,1,"Hey hey all you mammophiles, the moment you've",1);
 blitfont((sw>>1)-99,10,"all been waiting for has arrived!",1);
  vesabuffer();
  c=(sw-125)-1; t=ctimer();
  for(k=20;k<200;k++)
  { for(j=20;j<k;j++) { for(i=0;i<125;i++)
    { plot(c+i,(sh-k)+j,secretp[(j*125)+i]);
      plot(125-i,(sh-k)+j,secretp[(j*125)+i]); } }
      vesabuffer();
      while((ctimer()-t)<10) { i=i; };
    t=ctimer(); if(kbhit()) break; }
  if(kbhit()) { if(!getch()) getch(); }
  blitfont((sw>>1)-24,130,"Breasts!",1);
  blitfont((sw>>1)-111,sh-11,"Show's over. Press a key to finish...",1);
  vesabuffer();
  if(!getch()) getch(); cls(); setpal(opal);
return; }