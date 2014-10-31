# Makefile
#
# clib for MolScript v2.1
#
#      NOTE:
# This Makefile cannot be used on its own. It must be used
# indirectly from the Makefile in the molscript directory.
#
# Per Kraulis
#  12-Aug-1998  reorganized for v2.1

#------------------------------------------------------------
# Command execution shell.
SHELL = /bin/sh

# Original clib directory.
ORIGCLIBDIR = $(HOME)/clib

# Command 'cp' flags.
# -p = copy also time-stamps
CPFLAGS = -p

#------------------------------------------------------------
CSRC = args.c str_utils.c dynstring.c err.c indent.c \
       vector3.c matrix3.c quaternion.c body3d.c extent3d.c \
       io_utils.c colour.c key_value.c named_data.c double_hash.c \
       hermite_curve.c element_lookup.c aa_lookup.c mol3d.c mol3d_init.c \
       mol3d_io.c mol3d_utils.c mol3d_chain.c mol3d_secstruc.c \
       sgi_image.c vrml.c ogl_utils.c ogl_body.c ogl_bitmap_character.c

HSRC = args.h str_utils.h dynstring.h err.h indent.h boolean.h \
       vector3.h matrix3.h quaternion.h body3d.h extent3d.h angle.h \
       io_utils.h colour.h key_value.h named_data.h double_hash.h \
       hermite_curve.h element_lookup.h aa_lookup.h mol3d.h mol3d_init.h \
       mol3d_io.h mol3d_utils.h mol3d_chain.h mol3d_secstruc.h \
       sgi_image.h vrml.h ogl_utils.h ogl_body.h ogl_bitmap_character.h

#------------------------------------------------------------
src:
	for file in $(CSRC) $(HSRC); do \
	  cp $(CPFLAGS) $(ORIGCLIBDIR)/$$file . ; \
	done
	rm -f clib.a


#------------------------------------------------------------
OBJ = args.o str_utils.o dynstring.o err.o indent.o \
      vector3.o matrix3.o quaternion.o body3d.o extent3d.o \
      io_utils.o colour.o key_value.o named_data.o double_hash.o \
      hermite_curve.o element_lookup.o aa_lookup.o mol3d.o mol3d_init.o \
      mol3d_io.o mol3d_utils.o mol3d_chain.o mol3d_secstruc.o \
      sgi_image.o vrml.o $(OPENGLCLIBOBJ)

#------------------------------------------------------------
clib.a: $(OBJ)
	ar -cru clib.a $(OBJ)

clean:
	rm -f clib.a *.o

veryclean:
	rm -f *.h *.c; make clean
