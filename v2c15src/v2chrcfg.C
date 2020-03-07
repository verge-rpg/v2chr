/* V2 CHR graphic editor, module: V2CHRCFG.C */
/* Parses V2CHR.CFG */
/* Brad Smith January 30th 1999 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{ int frames;
  int height;
  int width;
  int hotspotx;
  int hotspoty;
  int hotspotwidth;
  int hotspotheight;
} chrtemplt;

extern char stpvmd;
extern char backupon;
extern char backupfile[76];
extern unsigned char gcols[9];
extern chrtemplt chrtdef;
extern unsigned char zoom;
extern char zoomcfg;
extern char gridon;

extern unsigned char edpal[768];
#include "vergepal.h"
#include "vidmouse.h"
extern int loadpcx(char *filename, unsigned char *bm,int w,int h,unsigned char *pal);

char msefile[15];

unsigned char matchcolour(char r,char g,char b,unsigned char *pal)
{ unsigned char p;
  int i,j,k; k=(64*3)+5;
  for(i=0;i<256;i++)
  { j=abs(r-pal[i*3]); j+=abs(g-pal[(i*3)+1]); j+=abs(b-pal[(i*3)+2]);
    if(j<k) { p=i; k=j; } }
return p; }

void parseconfig()
{ FILE *f;
  char p;
  char *buff;

  memcpy(edpal,vergepal,768); /* load up the default palette */

  f=fopen("v2chr.cfg","rb");
  if(f==NULL) return;
  buff=malloc(1024);
  if(buff==NULL) return;

  while(!feof(f))
  { buff[0]=0;
    fscanf(f,"%s",buff);
 newf:
    if(!strcmp(buff,"#"))
    { p=0;
      while(!feof(f) && p!=10 && p!=13)
      { p=fgetc(f); }
    }
    else if(!stricmp(buff,"VIDEO"))
    { fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        stpvmd=atoi(buff);
      }
      else goto newf;
    }
    else if(!stricmp(buff,"SIMPLEMOUSECOL"))
    { int i,smc;
      fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        smc=atoi(buff);
        for(i=0;i<64;i++) { if(msecursors[i+64]) msecursors[i+64]=smc; }
      }
      else goto newf;
    }
    else if(!stricmp(buff,"BACKUP"))
    { fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        if(!stricmp(buff,"ON"))
        { backupon=1; }
        else if(!stricmp(buff,"OFF"))
        { backupon=0; }
        else goto newf;
      }
      else goto newf;
    }
    else if(!stricmp(buff,"SIMPLEMOUSE"))
    { fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        if(!stricmp(buff,"ON"))
        { msecurs=msecursors+64; }
//        else if(!stricmp(buff,"OFF"))
//        { msecurs=msecursors; }
        else goto newf; }
      else goto newf;
    }
    else if(!stricmp(buff,"MOUSE"))
    { fscanf(f,"%s",buff);
      if(!stricmp(buff,"="))
      { fscanf(f,"%s",buff);
        if(buff[0]=='"')
        { strcpy(msefile,buff+1);
          msefile[strlen(msefile)-1]=0;
          if(!loadpcx(msefile,msecursors+128,8,8,NULL))
          { msecurs=msecursors+128; }
        }
        else goto newf;
      }
      else goto newf;
    }
    else if(!stricmp(buff,"BACKUPFILE"))
    { fscanf(f,"%s",buff);
      if(!stricmp(buff,"="))
      { fscanf(f,"%s",buff);
        if(buff[0]=='"')
        { strcpy(backupfile,buff+1);
          backupfile[strlen(backupfile)-1]=0;
        }
        else goto newf;
      }
      else goto newf;
    }
    else if(!stricmp(buff,"PALETTE"))
    { fscanf(f,"%s",buff);
      if(!stricmp(buff,"="))
      { fscanf(f,"%s",buff);
        if(buff[0]=='"')
        { FILE *pfil; int i;
          buff[strlen(buff)-1]=0;
          pfil=fopen(buff+1,"rb");
          if(pfil!=NULL)
          { for(i=0;i<768;i++)
            { if(!feof(pfil)) edpal[i]=fgetc(pfil); }
            fclose(pfil);
            for(i=0;i<9;i++)
            { gcols[i]=
              matchcolour(vergepal[gcols[i]*3],vergepal[(gcols[i]*3)+1],
                          vergepal[(gcols[i]*3)+2],edpal); }
          }
        }
        else goto newf;
      }
      else goto newf;
    }
    else if(!strnicmp(buff,"COL",3))
    { char cch=9;
      if(!stricmp(buff+3,"TEXT")) cch=0;
      if(!stricmp(buff+3,"DTEX")) cch=1;
      if(!stricmp(buff+3,"HIGH")) cch=2;
      if(!stricmp(buff+3,"FACE")) cch=3;
      if(!stricmp(buff+3,"DISB")) cch=4;
      if(!stricmp(buff+3,"SHAD")) cch=5;
      if(!stricmp(buff+3,"LIGH")) cch=6;
      if(!stricmp(buff+3,"FEAL")) cch=7;
      if(!stricmp(buff+3,"FEAS")) cch=8;
      if(cch<9)
      { fscanf(f,"%s",buff);
        if(!strcmp(buff,"="))
        { fscanf(f,"%s",buff);
          gcols[cch]=atoi(buff);
        }
        else goto newf;
      }
    }
    else if(!stricmp(buff,"FRAMES"))
    { fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        chrtdef.frames=atoi(buff);
      }
      else goto newf;
    }
    else if(!stricmp(buff,"WIDTH"))
    { fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        chrtdef.width=atoi(buff);
      }
      else goto newf;
    }
    else if(!stricmp(buff,"HEIGHT"))
    { fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        chrtdef.height=atoi(buff);
      }
      else goto newf;
    }
    else if(!stricmp(buff,"HOTX"))
    { fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        chrtdef.hotspotx=atoi(buff);
      }
      else goto newf;
    }
    else if(!stricmp(buff,"HOTY"))
    { fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        chrtdef.hotspoty=atoi(buff);
      }
      else goto newf;
    }
    else if(!stricmp(buff,"HOTWIDTH"))
    { fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        chrtdef.hotspotwidth=atoi(buff);
      }
      else goto newf;
    }
    else if(!stricmp(buff,"HOTHEIGHT"))
    { fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        chrtdef.hotspotheight=atoi(buff);
      }
      else goto newf;
    }
    else if(!stricmp(buff,"GRID"))
    { fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        if(!stricmp(buff,"ON"))
        { gridon=1; }
        else if(!stricmp(buff,"OFF"))
        { gridon=0; }
        else goto newf;
      }
      else goto newf;
    }
    else if(!stricmp(buff,"ZOOM"))
    { fscanf(f,"%s",buff);
      if(!strcmp(buff,"="))
      { fscanf(f,"%s",buff);
        zoom=atoi(buff);
        zoomcfg=1;
        if(zoom>3) { zoom=0; zoomcfg=0; }
      }
      else goto newf;
    }
  }

  fclose(f);

  if(stpvmd>3 || stpvmd<1) /* Invalid video selection */
  { stpvmd=1; }

  free(buff);

return; }

/* End of module. First edition: January 30th 1999 Brad Smith */
