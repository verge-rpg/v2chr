/* NEWCHAOS PCX.C module adapted for the V2 CHR editor */
/* Original, and edited versions by Brad Smith */

#include <stdio.h>
#include <stdlib.h>

/* loads a PCX into a buffer */
int loadpcx(char *filename, unsigned char *bm, int w, int h, unsigned char *pal)
{ FILE *f;
  unsigned short int xmin,xmax,pwid;
  unsigned short int ymin,ymax,phig;
  unsigned char bits;
  unsigned char encode;
  unsigned long int current;
  unsigned char curdat;
  unsigned char datleft;
  unsigned char *pbuffer;
  unsigned long int px,py;
  unsigned short int linelen;
  unsigned short int curline;
  unsigned short int i,j;

  f=fopen(filename,"rb");
  if(f==NULL) { return 1; }

  fgetc(f); /* manufacturer */
  fgetc(f); /* version */
  encode=fgetc(f);
  bits=fgetc(f);
  fread(&xmin,2,1,f);
  fread(&ymin,2,1,f);
  fread(&xmax,2,1,f);
  fread(&ymax,2,1,f);
  fseek(f,4,SEEK_CUR); /* xres, yres */
  fseek(f,48,SEEK_CUR); /* I shan't be needing an EGA palette */
  fgetc(f); /* Reserved? */
  fgetc(f); /* planes */
  fread(&linelen,2,1,f);
  fseek(f,2,SEEK_CUR); /* greyscale/not */
  fseek(f,58,SEEK_CUR); /* Z-Soft called it "filler" */
  fseek(f,128,SEEK_SET); /* Just in case */

  if(bits!=8) { fclose(f); return 2; } /* Image isn't 256 color. */
  if(encode!=1) { fclose(f); return 3; } /* Image isn't RLE. */

  pwid=xmax-xmin+1;
  phig=ymax-ymin+1;

  pbuffer=malloc(pwid*phig+1);
  if(pbuffer==NULL) { fclose(f); return 4; } /* Out of memory. */

  for(curline=0;curline<phig;curline++)
  { current=pwid*curline;
    i=0;
    do
    { curdat=fgetc(f);
      if((curdat&192)==192)
      { datleft=curdat&63;
        curdat=fgetc(f);
        for(j=0;j<datleft;j++)
        { pbuffer[current+i+j]=curdat; }
        i+=datleft;
      }
      else
      { pbuffer[current+i]=curdat; i++; }
    } while (i<linelen);
  }

  for(px=0;px<pwid;px++)
  { for(py=0;py<phig;py++)
    { if(px<w && py<h)
      { bm[(py*w)+px]=pbuffer[px+(py*pwid)]; }
    }
  } /* Copy buffer into data */
  free(pbuffer);

  if(pal==NULL) { fclose(f); return 0; } /* Palette not requested. */
  fseek(f,-769,SEEK_END);
  if(fgetc(f)!=12)
  { fclose(f); return 5; } /* No palette. */

  for(current=0; current<768; current++)
  { pal[current]=fgetc(f)>>2; }

  fclose(f);

return 0; }

/* saves a PCX */
int savepcx(unsigned char *filename, unsigned char *bm, int w, int h, unsigned char *pal)
{ FILE *f;
  unsigned short int tc;
  unsigned int cline;
  unsigned int cx;
  unsigned char crun;
  unsigned char cp;
  unsigned int bpl;
  unsigned char *p;

  f=fopen(filename,"wb");
  if(f==NULL) { return 1; } /* Invalid filename */

  fputc(10,f); /* Manuf: ZSoft PCX */
  fputc(5,f); /* Version 5 (PCX 3) */
  fputc(1,f); /* Encoding: 1 (RLE) */
  fputc(8,f); /* Bits: 8 (256color) */

  fputc(0,f); fputc(0,f); fputc(0,f); fputc(0,f); /* Xmin / Ymin */
  tc=w-1; fwrite(&tc,2,1,f); /* Xmax */
  tc=h-1; fwrite(&tc,2,1,f); /* Ymax */
  tc=320; fwrite(&tc,2,1,f); /* Xres */
  tc=200; fwrite(&tc,2,1,f); /* Yres */
  for(cx=0;cx<48;cx++) { fputc(0,f); } /* Empty EGA palette */
  fputc(0,f); /* reserved */
  fputc(1,f); /* planes */
  tc=w;
  if(tc&1) tc++;
  bpl=tc;
  fwrite(&tc,2,1,f); /* Bytes per line */
  tc=1; fwrite(&tc,2,1,f); /* Color palette */
  for(cx=0;cx<58;cx++) { fputc(0,f); } /* filler */

  cx=0;
  while(cx<h)
  { int i;
    p=bm+(w*cx);
    i=0;
    while (i<w)
    {   cp=p[i++];
        crun=1;
        while (crun<(unsigned) 63 && i<w && cp==p[i])
        { crun++; i++; }
        if (crun>1 || (cp & 0xC0) != 0) fputc(0xC0+crun,f);
        fputc(cp,f);
    }
    if(w&1) fputc(0,f); /* Padding for odd sizes */
    cx++;
  }
  fputc(0,f);

  fputc(12,f); /* Means "YES there is a palette" */
  for(cx=0;cx<768;cx++)
  { fputc(pal[cx]<<2,f); } /* Write palette */

  fclose(f);

return 0;}


/*
 HEADER PARSING THAT'S ONLY USED IN THE TESTING

  unsigned char manuf;
  unsigned char ver;
  unsigned short int xres,yres;
  unsigned char planes;
  unsigned char palette;

  printf("Manufacturer: %d\n",manuf);
  printf("Version: %d\n",ver);
  printf("Encoding: %d\n",encode);
  printf("Bits per Pixel: %d\n",bits);
  printf("Xmin, Xmax, Xc: %d, %d, %d\n",xmin,xmax,(xmax-xmin+1));
  printf("Ymin, Ymax, Yc: %d, %d, %d\n",ymin,ymax,(ymax-ymin+1));
  printf("Original resolution X, Y: %d, %d\n",xres,yres);
  printf("Color Planes: %d\n",planes);
  printf("Bits per Line: %d\n",bpline);
  printf("Palette Info: %d\n",palette);

  Also, a cheap way of writing a PCX would be to write '193' 'colour' for
  every single value.
*/
