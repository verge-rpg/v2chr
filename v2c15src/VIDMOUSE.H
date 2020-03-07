/* V2 CHR graphic editor header: VIDMOUSE.C */
/* DOS Interrupt driven Mouse driver interface */
/* Brad Smith 1999 */

#ifndef VIDMOUSE
#define VIDMOUSE

extern void (*mseblit)(int x, int y);
extern void (*msebolt)(int x, int y);
extern int mousex,mousey,mousek;
extern int mox,moy,mok;
extern unsigned char mousebox[64];
extern unsigned char *msecurs;
extern unsigned char msecursors[128];
extern void msedrawfuncs(char mode);
extern int mseinit();
extern void msepoll();

#endif

/* End of header. First edition completed January 30th 1999 */
