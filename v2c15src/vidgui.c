// NewChaos Editor Shell GUI graphic routines
// Modified January 1999 to fit into the V2 CHR graphical editor
// Original and modified, Brad Smith 1999

#include "video.h"
#include "font.h"

extern unsigned char gcols[9];

void guisysgrey()
{ int i,j;
  for(i=0;i<sh;i++)
  { if(i&1)
    { for(j=0;j<sw;j++)
      { if(j&1) plot(j,i,gcols[1]); }
    }
    else
    { for(j=0;j<sw;j++)
      { if(!(j&1)) plot(j,i,gcols[1]); }
    }
  }
return; }

void guidrawb(int x1, int y1, int x2, int y2, char* text)
{ box(x1,y1,x2,y2,gcols[3]);
  hline(x1,y1,x2,gcols[6]);
  hline(x1,y2,x2,gcols[5]);
  vline(x1,y1,y2,gcols[6]);
  vline(x2,y1,y2,gcols[5]);
  plot(x1,y1,gcols[7]);
  plot(x2,y2,gcols[8]);
  plot(x1,y2,gcols[3]);
  plot(x2,y1,gcols[3]);
  blitfont(x1+3,y1+3,text,gcols[1]);
  blitfont(x1+2,y1+2,text,gcols[0]);
return; }

void guidrawbc(int x1, int y1, int x2, int y2, char* text)
{ box(x1,y1,x2,y2,gcols[4]);
  hline(x1,y1,x2,gcols[5]);
  hline(x1,y2,x2,gcols[6]);
  vline(x1,y1,y2,gcols[5]);
  vline(x2,y1,y2,gcols[6]);
  plot(x1,y1,gcols[8]);
  plot(x2,y2,gcols[7]);
  plot(x1,y2,gcols[3]);
  plot(x2,y1,gcols[3]);
  blitfont(x1+3,y1+3,text,gcols[0]);
return; }

void guidrawbd(int x1, int y1, int x2, int y2, char* text)
{ box(x1,y1,x2,y2,gcols[4]);
  hline(x1,y1,x2,gcols[5]);
  hline(x1,y2,x2,gcols[6]);
  vline(x1,y1,y2,gcols[5]);
  vline(x2,y1,y2,gcols[6]);
  plot(x1,y1,gcols[8]);
  plot(x2,y2,gcols[7]);
  plot(x1,y2,gcols[3]);
  plot(x2,y1,gcols[3]);
  blitfont(x1+2,y1+2,text,gcols[1]);
return; }

// End of file. First modified edition: Brad Smith January 31st 1999
