/* sgi_image.c

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

#include "sgi_image.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <boolean.h>


#define VERBATIM 0
#define RLE 1


/*------------------------------------------------------------*/
sgi_image *
sgiimg_create (void)
{
  sgi_image *new;

  new = malloc (sizeof (sgi_image));
  new->xsize = 500;
  new->ysize = 500;
  new->zsize = 3;		/* RGB by default */
  new->dimension = 3;
  new->storage = VERBATIM;
  new->bpc = 1;
  new->pinmin = 0;
  new->pinmax = 255;
  new->name[0] = '\0';

  new->file = NULL;
  new->start_table = NULL;
  new->length_table = NULL;

  return new;
}


/*------------------------------------------------------------*/
void
sgiimg_delete (sgi_image *img)
{
  assert (img);

  if (img->file) sgiimg_file_close (img);

  free (img);
}

/*------------------------------------------------------------*/
void
sgiimg_set_size (sgi_image *img, int width, int height)
{
  assert (img);
  assert (img->file == NULL);
  assert (width > 0);
  assert (width < 8191);
  assert (height > 0);
  assert (height < 8191);

  img->xsize = width;
  img->ysize = height;
  if (img->storage == RLE) sgiimg_set_rle (img);
}


/*------------------------------------------------------------*/
void
sgiimg_set_verbatim (sgi_image *img)
{
  assert (img);
  assert (img->file == NULL);

  img->storage = VERBATIM;
  if (img->start_table) {
    free (img->start_table);
    img->start_table = NULL;
    free (img->length_table);
    img->length_table = NULL;
  }
}


/*------------------------------------------------------------*/
void
sgiimg_set_rle (sgi_image *img)
{
  assert (img);
  assert (img->file == NULL);

  img->storage = RLE;
  if (img->start_table) {
    free (img->start_table);
    free (img->length_table);
  }
  img->tablen = img->ysize * img->zsize;
  img->start_table = calloc (img->tablen, sizeof (long));
  img->length_table = calloc (img->tablen, sizeof (long));
}


/*------------------------------------------------------------*/
void
sgiimg_set_char (sgi_image *img)
{
  assert (img);
  assert (img->file == NULL);

  img->bpc = 1;
}


/*------------------------------------------------------------*/
void
sgiimg_set_short (sgi_image *img)
{
  assert (img);
  assert (img->file == NULL);

  img->bpc = 2;
}


/*------------------------------------------------------------*/
void
sgiimg_set_bw (sgi_image *img)
{
  assert (img);
  assert (img->file == NULL);

  img->zsize = 1;
  img->dimension = 2;
  if (img->storage == RLE) sgiimg_set_rle (img);
}


/*------------------------------------------------------------*/
void
sgiimg_set_rgb (sgi_image *img)
{
  assert (img);
  assert (img->file == NULL);

  img->zsize = 3;
  img->dimension = 3;
  if (img->storage == RLE) sgiimg_set_rle (img);
}


/*------------------------------------------------------------*/
void
sgiimg_set_rgba (sgi_image *img)
{
  assert (img);
  assert (img->file == NULL);

  img->zsize = 4;
  img->dimension = 3;
  if (img->storage == RLE) sgiimg_set_rle (img);
}


/*------------------------------------------------------------*/
void
sgiimg_set_minmax (sgi_image *img, long min, long max)
{
  assert (img);
  assert (img->file == NULL);
  assert (min >= 0);
  assert (max > 0);
  assert (max <= 65535);
  assert (min < max);

  img->pinmin = min;
  img->pinmax = max;
}


/*------------------------------------------------------------*/
void
sgiimg_set_name (sgi_image *img, char *str)
{
  assert (img);
  assert (img->file == NULL);
  assert (str);

  strncpy (img->name, str, 79);
  img->name[79] = '\0';
}


/*------------------------------------------------------------*/
int
sgiimg_file_create (sgi_image *img, char *filename)
{
  assert (img);
  assert (img->file == NULL);
  assert (filename);
  assert (*filename);

  img->file = fopen (filename, "wb");
  if (img->file == NULL) return FALSE;

  if (sgiimg_file_init (img)) {
    return TRUE;
  } else {
    fclose (img->file);
    img->file = NULL;
    return FALSE;
  }
}


/*------------------------------------------------------------*/
int
sgiimg_file_init (sgi_image *img)
{
  short magic = 474;
  char dummy = 0;
  long colormap = 0;
  int slot;

  assert (img);
  assert (img->file != NULL);

  if (fwrite (&magic, 2, 1, img->file) < 1) return FALSE;

  fwrite (&(img->storage), 1, 1, img->file);
  fwrite (&(img->bpc), 1, 1, img->file);
  fwrite (&(img->dimension), 2, 1, img->file);
  fwrite (&(img->xsize), 2, 1, img->file);
  fwrite (&(img->ysize), 2, 1, img->file);
  fwrite (&(img->zsize), 2, 1, img->file);
  fwrite (&(img->pinmin), 4, 1, img->file);
  fwrite (&(img->pinmax), 4, 1, img->file);
  for (slot = 0; slot < 4; slot++) fwrite (&dummy, 1, 1, img->file);
  fwrite (&(img->name[0]), 1, 80, img->file);
  fwrite (&colormap, 4, 1, img->file);
  for (slot = 0; slot < 404; slot++) fwrite (&dummy, 1, 1, img->file);

  if (img->storage == RLE) {
    fwrite (img->start_table, 4, img->tablen, img->file);
    fwrite (img->length_table, 4, img->tablen, img->file);
  }

  img->current_channel = 0;
  img->current_rownum = 0;

  return TRUE;
}


/*------------------------------------------------------------*/
int
sgiimg_file_open (sgi_image *img, char *filename)
{
  assert (img);
  assert (img->file == NULL);
  assert (filename);
  assert (*filename);

  img->file = fopen (filename, "rb");

  img->current_channel = 1;
  img->current_rownum = 0;

  return (img->file != NULL);
}


/*------------------------------------------------------------*/
void
sgiimg_file_close (sgi_image *img)
{
  assert (img);
  assert (img->file != NULL);

  if (img->storage == RLE) {
    fseek (img->file, 512, SEEK_SET);
    fwrite (img->start_table, 4, img->tablen, img->file);
    fwrite (img->length_table, 4, img->tablen, img->file);
  }
  fclose (img->file);
  img->file = NULL;
}


/*------------------------------------------------------------*/
int
sgiimg_write_next_char_row (sgi_image *img, unsigned char *row)
{
  assert (img);
  assert (img->file != NULL);
  assert (img->bpc == 1);
  assert ((img->storage == VERBATIM) || (img->storage == RLE));
  assert (row);
  assert (img->current_rownum < img->ysize);
  assert (img->current_channel < img->zsize);

  if (img->storage == VERBATIM) {
    if (fwrite (row, 1, img->xsize, img->file) < img->xsize) return FALSE;

  } else {			/* RLE */
    unsigned char indicator, pixel, len;
    int slot1, slot2, equality;
    int pos = img->current_rownum + img->ysize * img->current_channel;
    long length = 0;

    img->start_table[pos] = ftell (img->file);

    for (slot1 = 0; slot1 < img->xsize; slot1 += len) {

      pixel = row[slot1];
      slot2 = slot1 + 1;

      if (slot2 < img->xsize) {
	equality = (pixel == row[slot2]);
	for (len = 2, slot2++;
	     (len < 0x7f) && (slot2 < img->xsize);
	     len++, slot2++) {
	  if ((pixel == row[slot2]) != equality) break;
	}

	indicator = len;
	if (equality) {		/* copies of pixel; high-order bit == 0 */
	  if (fwrite (&indicator, 1, 1, img->file) < 1) return FALSE;
	  fwrite (&pixel, 1, 1, img->file);
	  length += 2;
	} else {		/* stretch of pixels; high-order bit == 1 */
	  indicator |= 0x80;
	  if (fwrite (&indicator, 1, 1, img->file) < 1) return FALSE;
	  fwrite (row + slot1, 1, (size_t) len, img->file);
	  length += 1 + len;
	}
	

      } else {			/* special case: one single char left */
	len = 1;
	indicator = 0x81;
	fwrite (&indicator, 1, 1, img->file);
	fwrite (&pixel, 1, (size_t) len, img->file);
	length += 2;
      }
    }

    indicator = 0x80;		/* end of row: count == 0 */
    fwrite (&indicator, 1, 1, img->file);
    length++;

    img->length_table[pos] = length;
  }

  img->current_rownum++;
  if (img->current_rownum >= img->ysize) {
    img->current_channel++;
    img->current_rownum = 0;
  }

  return TRUE;
}
