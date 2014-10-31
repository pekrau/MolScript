/*
   OpenGL bitmap character and string output.

   This is an optimized version of the GLUT library procedure
   glutBitmapCharacter. It assumes that the correct graphics state
   (as regards the pixelstore modes) has been set. The required values
   for the pixelstore modes are the default values (defined in the OpenGL
   specification), *except* for glPixelStorei (GL_UNPACK_ALIGNMENT, 1).

   This package relies on GLUT version 3.6.

   clib v1.1

   Copyright (C) 1998 Per Kraulis
     6-Feb-1998  first attempts
*/

#include "ogl_bitmap_character.h"

/* public ====================
#include <GL/glut.h>
==================== public */

#include <assert.h>

#include <../lib/glut/glutbitmap.h>


/*------------------------------------------------------------*/
void
ogl_bitmap_character (void *font, int c)
     /*
       Output the character using the given GLUT bitmap font.
       The font must be a GLUTbitmapFont.
       The raster position must be defined.
     */
{
  const BitmapCharRec *ch;
  BitmapFontPtr fontinfo;

  /* pre */
  assert (font);

#if defined(WIN32)
  fontinfo = (BitmapFontPtr) __glutFont(font);
#else
  fontinfo = (BitmapFontPtr) font;
#endif

  if (c < fontinfo->first ||
    c >= fontinfo->first + fontinfo->num_chars)
    return;
  ch = fontinfo->ch[c - fontinfo->first];
  if (ch) {
    glBitmap(ch->width, ch->height, ch->xorig, ch->yorig,
      ch->advance, 0, ch->bitmap);
  }
}


/*------------------------------------------------------------*/
void
ogl_bitmap_string (void *font, char *str)
     /*
       Output the string using the given GLUT bitmap font.
       The font must be a GLUTbitmapFont.
       The raster position must be defined.
     */
{
  /* pre */
  assert (font);
  assert (str);

  for ( ; *str; str++) ogl_bitmap_character (font, *str);
}
