MolScript v2.1.2
================

    Copyright (C) 1997-1998 Per J. Kraulis

The MolScript program is used to produce publication-quality images of
macromolecular 3D structures. 

It has for a long time been a standard tool in the biomolecular
structural sciences. This is reflected by the fact that the paper
describing MolScript appears as number 82 in the list of the feature article
["The Top 100 Papers. Nature explores the most-cited research of all
time"](http://www.nature.com/news/the-top-100-papers-1.16224) by
Richard Van Noorden, Brendan Maher & Regina Nuzzo published [30 Oct
2014](http://www.nature.com/nature/journal/v514/n7524/index.html).

Open Source
-----------

I have for a long time intended to make MolScript Open Source, but
never got around to it. The Nature Top-100-list did the trick of
pushing me into action. MolScript is now available under the MIT
license from this GitHub repository.

Version 2.1.2
-------------

The first version of MolScript (written in Fortran 77) was released in
1991, and its current version (2.1.2, written in C) in 1998.

Please be aware that no changes have been made to the code since
1998. In particular, the Makefile for the executable including OpenGL
support is not up to scratch. It needs updating. If anyone is willing to
help, I would much appreciate it.

I have tested the so-called Makefile.basic. It works. I have also
verified that the [Raster3D software
(v3.0)](http://skuld.bmsc.washington.edu/raster3d/html/raster3d.html)
still works with MolScript.

Future plans
------------

I have very little time to work on this currently. Other projects are
more pressing. But here are some possible items for a roadmap for
future development of MolScript:

* Fix the OpenGL implementation, with a working Makefile.

* Prepare a proper Debian package for easier installation.

* Add a proper interactive interface to the OpenGL implementation. The
  script language is nowadays a user-hostile interface.

* Write an implementation to produce [X3D](http://www.web3d.org/), the
  successor to the VRML format for 3D objects on the web..

* Make a [WebGL](http://en.wikipedia.org/wiki/WebGL) implementation

* Set up a web service producing images from input scripts using MolScript.


Reference
---------

    MOLSCRIPT: a program to produce both detailed and schematic plots of protein structures
    J. Appl. Cryst. (1991) 24, 946-950
    [DOI:10.1107/S0021889891004399](http://dx.doi.org/10.1107/S0021889891004399)
    [at J. Appl. Cryst. web site](http://scripts.iucr.org/cgi-bin/paper?S0021889891004399)

