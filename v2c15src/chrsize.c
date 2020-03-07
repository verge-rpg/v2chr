/* V2 CHR Resizing Program, Brad Smith 2000 */

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{ int ladd,radd,tadd,badd;
  FILE *fi,*fo;
  unsigned char *dat,*com;
  unsigned long int comsize,totalsize,ncomsize,p,q,l;
  int frames,width,height,framesize;
  int nwidth,nheight,ntotalsize,nframesize;
  int i,j,k,r,x,y;
  unsigned char c;
  unsigned short s;

  printf(" V2 CHR Resizing Program, Brad Smith 2000 \n");
  printf("==========================================\n");
  if(argc<3)
  { printf("Usage: CHRSIZE infile outfile arguments\n");
    printf("Arguments: [+|-][l|r|t|b][#]\n");
    printf("Ex. CHRSIZE in.chr out.chr -l5 +t8\n");
    printf(" (adds 8 lines to the top, takes 5 away\n");
    printf(" from the left)\n");
    return 1; }

  fi=fopen(argv[1],"rb");
  if(fi==NULL)
  { printf("Unable to open %s. Aborting.\n",argv[1]);
    return 2; }
  printf("Source: %s\n",argv[1]);
  fo=fopen(argv[2],"wb");
  if(fo==NULL)
  { fclose(fi);
    printf("Unable to open %s. Aborting.\n",argv[2]);
    return 3; }
  printf("Destination: %s\n",argv[2]);

  ladd=0; radd=0; tadd=0; badd=0;

  for(i=3;i<argc;i++)
  { if(strlen(argv[i])<3) { printf("%s : invalid argument.\n",argv[i]); }
    else if(argv[i][0]=='-')
    {
      if(argv[i][1]=='l')
      { ladd=0-atoi(argv[i]+2); }
      else if(argv[i][1]=='r')
      { radd=0-atoi(argv[i]+2); }
      else if(argv[i][1]=='t')
      { tadd=0-atoi(argv[i]+2); }
      else if(argv[i][1]=='b')
      { badd=0-atoi(argv[i]+2); }
      else if(argv[i][1]=='L')
      { ladd=0-atoi(argv[i]+2); }
      else if(argv[i][1]=='R')
      { radd=0-atoi(argv[i]+2); }
      else if(argv[i][1]=='T')
      { tadd=0-atoi(argv[i]+2); }
      else if(argv[i][1]=='B')
      { badd=0-atoi(argv[i]+2); }
      else { printf("%s : invalid argument.\n",argv[i]); }
    }
    else if(argv[i][0]=='+')
    {
      if(argv[i][1]=='l')
      { ladd=atoi(argv[i]+2); }
      else if(argv[i][1]=='r')
      { radd=atoi(argv[i]+2); }
      else if(argv[i][1]=='t')
      { tadd=atoi(argv[i]+2); }
      else if(argv[i][1]=='b')
      { badd=atoi(argv[i]+2); }
      else if(argv[i][1]=='L')
      { ladd=atoi(argv[i]+2); }
      else if(argv[i][1]=='R')
      { radd=atoi(argv[i]+2); }
      else if(argv[i][1]=='T')
      { tadd=atoi(argv[i]+2); }
      else if(argv[i][1]=='B')
      { badd=atoi(argv[i]+2); }
      else { printf("%s : invalid argument.\n",argv[i]); }
    }
    else { printf("%s : invalid argument.\n",argv[i]); }
  }

  printf("Adding %d columns to the left.\n",ladd);
  printf("Adding %d columns to the right.\n",radd);
  printf("Adding %d rows to the top.\n",tadd);
  printf("Adding %d rows to the bottom.\n",badd);

  printf("Processing...\n");

  c=fgetc(fi);
  if(c!=2)
  { fclose(fi); fclose(fo);
    printf("%s is not a valid V2 CHR file.\n",argv[1]); }
  fputc(2,fo);

  // Header
  fread(&s,1,2,fi); width=s; s+=ladd+radd; fwrite(&s,1,2,fo);
  fread(&s,1,2,fi); height=s; s+=tadd+badd; fwrite(&s,1,2,fo);
  framesize=width*height;
  fread(&s,1,2,fi); fwrite(&s,1,2,fo);
  fread(&s,1,2,fi); fwrite(&s,1,2,fo);
  fread(&s,1,2,fi); fwrite(&s,1,2,fo);
  fread(&s,1,2,fi); fwrite(&s,1,2,fo);
  fread(&s,1,2,fi); frames=s; fwrite(&s,1,2,fo);

  if(width+ladd+radd<1)
  { fclose(fi); fclose(fo);
    printf("Invalid final CHR width.\n");
    return 4; }
  if(height+tadd+badd<1)
  { fclose(fi); fclose(fo);
    printf("Invalid final CHR height.\n");
    return 4; }

  // Read the compressed size from fi.
  fread(&comsize,1,4,fi);
  totalsize=frames*width*height;

  com=malloc(comsize+1);
  if(com==NULL)
  { fclose(fi); fclose(fo);
    printf("Out of memory.\n");
    return 5; }
  dat=malloc(totalsize+1);
  if(dat==NULL)
  { free(com); fclose(fi); fclose(fo);
    printf("Out of memory.\n");
    return 5; }

  // load the compressed data.
  fread(com,1,comsize,fi);

  p=0; r=0; c=0; q=0;
  while(p<comsize || q<totalsize)
  { if(!r)
    { if(com[p]==0xFF)
      { p++; r=com[p]; p++; c=com[p]; p++; }
      else { r=1; c=com[p]; p++; }
    }
    else { r--; if(q<totalsize) { dat[q]=c; q++; } }
  }

  free(com);

  nwidth=width+ladd+radd;
  nheight=height+tadd+badd;
  nframesize=nwidth*nheight;
  ntotalsize=nframesize*frames;

  com=calloc(ntotalsize+1,1); // com is now the data for the new chr.
  if(com==NULL)
  { free(dat); fclose(fi); fclose(fo);
    printf("Out of memory.\n");
    return 5; }

  for(i=0;i<frames;i++)
  { for(k=0;k<nheight;k++)
    { for(j=0;j<nwidth;j++)
      { if(j>=ladd && j<(nwidth-radd) && k>=tadd && k<(nheight-badd))
        { x=j-ladd; y=k-tadd;
          com[(i*nframesize)+j+(k*nwidth)]=dat[x+(y*width)+(i*framesize)]; }
      }
    }
  }

  free(dat); // the compressed -could- become too large if there's a lot
  dat=malloc(ntotalsize+1024); // of 255s.
  if(dat==NULL)
  { free(com); fclose(fi); fclose(fo);
    printf("Out of memory.\n");
    return 5; }

  q=0; p=0; r=0; c=0;
  do
  { c=com[q]; q++; r=1;
    while(r<254 && q<ntotalsize && q==com[q])
    { r++; q++; }
    if(r==2 && c!=0xFF)
    { dat[p]=c; p++; }
    if(r==3 && c!=0xFF)
    { dat[p]=c; dat[p+1]=c; p++; }
    if(r>3 || c==0xFF)
    { dat[p]=0xFF; dat[p+1]=r; p+=2; }
    dat[p]=c; p++;
  } while(q<ntotalsize);
  ncomsize=p; // data compressed

  free(com);

  fwrite(&ncomsize,1,4,fo);
  fwrite(dat,1,ncomsize,fo);
  free(dat); // data written

  // back to copying from file to file.
  fread(&l,1,4,fi); fwrite(&l,1,4,fo);
  fread(&l,1,4,fi); fwrite(&l,1,4,fo);
  fread(&l,1,4,fi); fwrite(&l,1,4,fo);
  fread(&l,1,4,fi); fwrite(&l,1,4,fo);
  for(i=0;i<4;i++)
  { fread(&s,1,4,fi); fwrite(&s,1,4,fo);
    for(j=0;j<s;j++) { c=fgetc(fi); putc(c,fo); }
  }

  printf("Done.\n");
  fclose(fi);
  fclose(fo);

return 0; }
