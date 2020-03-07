/* Verge 2 CHR graphic editor header: VIDEO.H */
/* Palette, Screen Setting, and Drawing       */
/*   Brad Smith January 1999                  */

#ifndef VIDEO
#define VIDEO

extern unsigned int sw;
extern unsigned int sh;
extern unsigned int swd;
extern unsigned char* vid;
extern unsigned char vmd;

extern void (*plot)(int x,int y, unsigned char c);
extern void (*hline)(int x1,int y,int x2, unsigned char c);
extern void (*vline)(int x,int y1,int y2, unsigned char c);
extern void (*box)(int x1,int y1,int x2,int y2, unsigned char c);
extern void (*cls)();
extern unsigned char (*getpix)(int x, int y);

extern void SDVideo();

#define VMD_13H    1 // 320x200 chained
#define VMD_MXL    2 // 320x240 unchained
#define VMD_VESA   3 // 640x480 vesa

extern void InitVideo(char mode);

extern void setpal(unsigned char *pal);
extern void getpal(unsigned char *pal);
extern void setcol(unsigned char c, unsigned char r, unsigned char g, unsigned char b);
extern void getcol(unsigned char c, unsigned char *r, unsigned char *g, unsigned char *b);

extern void vesabuffer();
extern void vesarealplot(int x, int y, unsigned char c);

#endif

/* End of header - First edition completed January 30th 1999, Brad Smith */
