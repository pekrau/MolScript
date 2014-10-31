/* sgi_image.h

   SGI (aka RGB) format image file i/o.

   clib v1.1

   This is based on the specification of the SGI image file format:
   http://reality.sgi.com/grafica/sgiimage.html

   Copyright (C) 1997-1998 Per Kraulis
    26-Sep-1997  started
    28-Sep-1997  write char routine implemented

   to do:
   - short routines not implemented
   - read routines not implemented
   - RLE needs optimizing: check if row actually gets compressed
*/

#ifndef	SGI_IMAGE_H
#define	SGI_IMAGE_H

#include <stdio.h>

typedef struct {
  unsigned short xsize, ysize, zsize;
  char storage, bpc;
  unsigned short dimension;
  long pinmin, pinmax;
  char name[80];

  FILE *file;
  int current_rownum, current_channel;
  int tablen;
  long *start_table, *length_table;
} sgi_image;

sgi_image *sgiimg_create (void);
void sgiimg_delete (sgi_image *img);

void sgiimg_set_size (sgi_image *img, int width, int height);
void sgiimg_set_verbatim (sgi_image *img);
void sgiimg_set_rle (sgi_image *img);
void sgiimg_set_char (sgi_image *img);
void sgiimg_set_short (sgi_image *img);
void sgiimg_set_bw (sgi_image *img);
void sgiimg_set_rgb (sgi_image *img);
void sgiimg_set_rgba (sgi_image *img);
void sgiimg_set_minmax (sgi_image *img, long min, long max);
void sgiimg_set_name (sgi_image *img, char *str);

int sgiimg_file_create (sgi_image *img, char *filename);
int sgiimg_file_init (sgi_image *img);
int sgiimg_file_open (sgi_image *img, char *filename);
void sgiimg_file_close (sgi_image *img);

int sgiimg_write_next_char_row (sgi_image *img, unsigned char *row);

#endif
