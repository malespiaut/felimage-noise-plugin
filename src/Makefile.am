## Process this file with automake to produce Makefile.in

bin_PROGRAMS = felimage-noise

bindir = $(GIMP_LIBDIR)/plug-ins

felimage_noise_SOURCES = \
	basis.c         \
	cell_3d.c       \
	cell_4d.c       \
	cell_5d.c       \
	interface.c	\
	lnoise_3d.c     \
	lnoise_4d.c     \
	lnoise_5d.c     \
	loadconf.c	\
	main.c		\
	poisson.c	\
	random.c	\
	render.c	\
	saveconf.c	\
	snoise_3d.c     \
	snoise_4d.c     \
	snoise_5d.c     
	
include_HEADERS = \
	basis.h		\
	calibration.h   \
	cell.h		\
	cell_int.h	\
	link_icon.h	\
	lnoise.h	\
	lnoise_int.h	\
	loadsaveconf.h	\
	main.h		\
	poisson.h	\
	random.h	\
	render.h	\
	snoise.h	\
	snoise_int.h	

dist_data_DATA=\
	MakeCalibrate
	


AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DDATADIR=\""$(DATADIR)"\"

INCLUDES =\
	-I$(top_srcdir)		\
	@GIMP_CFLAGS@		\
	-I$(includedir)

LDADD = $(GIMP_LIBS)

