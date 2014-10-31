# Makefile
#
# MolScript v2.1 example images
#
# Per Kraulis
#  24-Jul-1998  first attempts
#  31-Oct-2014  output PNG from Raster3d (render); skip JPEG and RGB

#------------------------------------------------------------
MOLSCRIPT = ../molscript
MOLAUTO = ../molauto

RASTER3D = render

LARGE = -size 600 600
SMALL = -size 180 180 -accum 4

DOC = ../doc/images

#------------------------------------------------------------
.SUFFIXES: .in .ps .wrl .rgb .eps .sgi .jpg .png .gif

.in.ps:
	$(MOLSCRIPT) -postscript -in $< -out $@

.in.wrl:
	$(MOLSCRIPT) -vrml -in $< -out $@

.in.png:
	$(MOLSCRIPT) -raster3d $(LARGE) -in $< |  $(RASTER3D) -png $@

.in.eps:
	$(MOLSCRIPT) -eps $(LARGE) -in $< -out $@

.in.sgi:
	$(MOLSCRIPT) -sgi $(LARGE) -in $< -out $@

.in.jpg:
	$(MOLSCRIPT) -jpeg $(LARGE) -in $< -out $@

.in.gif:
	$(MOLSCRIPT) -gif $(LARGE) -in $< -out $@

#------------------------------------------------------------
complete: molauto postscript vrml raster3d eps sgi jpeg png gif

basic: molauto postscript vrml raster3d

clean:
	rm -f core *~ *.ps *.wrl *.wrl.gz *.sgi *.eps *.rgb *.jpg *.png *.gif

#------------------------------------------------------------
molauto: ras_molauto.in ras_molauto_nice.in

ras_molauto.in: ras.pdb
	$(MOLAUTO) $? > $@

ras_molauto_nice.in: ras.pdb
	$(MOLAUTO) -nice -cpk $? > $@

#------------------------------------------------------------
postscript: ras_std.ps ras_rmsd.ps ras_cyl.ps \
            ras_molauto.ps ras_molauto_nice.ps

#------------------------------------------------------------
vrml: ras_std.wrl ras_std_gz.wrl ras_rmsd.wrl ras_shiny.wrl \
      ras_vrml.wrl ras_min_vrml.wrl ras_min_vrml_gz.wrl \
      ras_cyl.wrl ras_cyl_gz.wrl \
      ras_gdp.wrl ras_gdp_gz.wrl \
      ras_molauto.wrl ras_molauto_gz.wrl \
      ras_vrml_lod.wrl ras_vrml_lod_gz.wrl \
      ras_molauto_nice.wrl ras_molauto_nice_gz.wrl

ras_std_gz.wrl: ras_std.wrl
	gzip -c $? > $@

ras_min_vrml_gz.wrl: ras_min_vrml.wrl
	gzip -c $? > $@

ras_cyl_gz.wrl: ras_cyl.wrl
	gzip -c $? > $@

ras_gdp_gz.wrl: ras_gdp.wrl
	gzip -c $? > $@

ras_vrml_lod_gz.wrl: ras_vrml_lod.wrl
	gzip -c $? > $@

ras_molauto_gz.wrl: ras_molauto.wrl
	gzip -c $? > $@

ras_molauto_nice_gz.wrl: ras_molauto_nice.wrl
	gzip -c $? > $@

#------------------------------------------------------------
raster3d: ras_std.png ras_rmsd.png ras_gdp.png

#------------------------------------------------------------
eps: ras_std.eps

#------------------------------------------------------------
sgi: ras_std.sgi ras_rmsd.sgi ras_shiny.sgi

#------------------------------------------------------------
jpeg: ras_std.jpg ras_std_small.jpg \
      ras_rmsd.jpg ras_rmsd_small.jpg \
      ras_shiny.jpg ras_shiny_small.jpg \
      ras_cyl.jpg ras_cyl_small.jpg \
      ras_gdp.jpg ras_gdp_small.jpg \
      ras_molauto.jpg ras_molauto_small.jpg \
      ras_molauto_nice.jpg ras_molauto_nice_small.jpg 

ras_std_small.jpg: ras_std.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in ras_std.in -out $@

ras_rmsd_small.jpg: ras_rmsd.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

ras_shiny_small.jpg: ras_shiny.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

ras_cyl_small.jpg: ras_cyl.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

ras_gdp_small.jpg: ras_gdp.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

ras_molauto_small.jpg: ras_molauto.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

ras_molauto_nice_small.jpg: ras_molauto_nice.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

#------------------------------------------------------------
png: ras_std.png ras_shiny.png

#------------------------------------------------------------
gif: ras_std.gif ras_cyl.gif

#------------------------------------------------------------
doc: $(DOC)/ras_std.jpg \
     $(DOC)/ras_rmsd.jpg \
     $(DOC)/ras_shiny.jpg \
     $(DOC)/ras_gdp_balls.jpg \
     $(DOC)/ras_gdp_bonds.jpg \
     $(DOC)/ras_gdp_cpk.jpg \
     $(DOC)/ras_coil.jpg \
     $(DOC)/ras_cyl.jpg \
     $(DOC)/ras_turn.jpg \
     $(DOC)/ras_trace.jpg 

$(DOC)/ras_std.jpg: ras_std.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

$(DOC)/ras_rmsd.jpg: ras_rmsd.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

$(DOC)/ras_shiny.jpg: ras_shiny.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

$(DOC)/ras_gdp_balls.jpg: ras_gdp_balls.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

$(DOC)/ras_gdp_bonds.jpg: ras_gdp_bonds.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

$(DOC)/ras_gdp_cpk.jpg: ras_gdp_cpk.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

$(DOC)/ras_coil.jpg: ras_coil.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

$(DOC)/ras_cyl.jpg: ras_cyl.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

$(DOC)/ras_turn.jpg: ras_turn.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@

$(DOC)/ras_trace.jpg: ras_trace.in
	$(MOLSCRIPT) -jpeg $(SMALL) -in $? -out $@
