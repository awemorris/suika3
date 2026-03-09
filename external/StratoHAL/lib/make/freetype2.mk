OBJS = \
	ftadvanc.o \
	ftbase.o \
	ftbbox.o \
	ftbdf.o \
	ftbitmap.o \
	ftcalc.o \
	ftcid.o \
	ftcolor.o \
	ftdbgmem.o \
	ftdebug.o \
	fterrors.o \
	ftfntfmt.o \
	ftfstype.o \
	ftgasp.o \
	ftgloadr.o \
	ftglyph.o \
	ftgxval.o \
	fthash.o \
	ftinit.o \
	ftlcdfil.o \
	ftmac.o \
	ftmm.o \
	ftobjs.o \
	ftotval.o \
	ftoutln.o \
	ftpatent.o \
	ftpfr.o \
	ftpsprop.o \
	ftrfork.o \
	ftsnames.o \
	ftstream.o \
	ftstroke.o \
	ftsynth.o \
	ftsystem.o \
	fttrigon.o \
	fttype1.o \
	ftutil.o \
	ftwinfnt.o \
	md5.o \
	bdf.o \
	bdfdrivr.o \
	bdflib.o \
	ftbzip2.o \
	ftcache.o \
	ftcbasic.o \
	ftccache.o \
	ftccmap.o \
	ftcglyph.o \
	ftcimage.o \
	ftcmanag.o \
	ftcmru.o \
	ftcsbits.o \
	cff.o \
	cffcmap.o \
	cffdrivr.o \
	cffgload.o \
	cffload.o \
	cffobjs.o \
	cffparse.o \
	cidgload.o \
	cidload.o \
	cidobjs.o \
	cidparse.o \
	cidriver.o \
	type1cid.o \
	otvalid.o \
	otvbase.o \
	otvcommn.o \
	otvgdef.o \
	otvgpos.o \
	otvgsub.o \
	otvjstf.o \
	otvmath.o \
	otvmod.o \
	pfr.o \
	pfrcmap.o \
	pfrdrivr.o \
	pfrgload.o \
	pfrload.o \
	pfrobjs.o \
	pfrsbit.o \
	afmparse.o \
	cffdecode.o \
	psarrst.o \
	psaux.o \
	psauxmod.o \
	psblues.o \
	psconv.o \
	pserror.o \
	psfont.o \
	psft.o \
	pshints.o \
	psintrp.o \
	psobjs.o \
	psread.o \
	psstack.o \
	pshalgo.o \
	pshglob.o \
	pshinter.o \
	pshmod.o \
	pshrec.o \
	psmodule.o \
	psnames.o \
	ftraster.o \
	ftbsdf.o \
	ftsdf.o \
	ftsdfcommon.o \
	ftsdfrend.o \
	pngshim.o \
	sfdriver.o \
	sfobjs.o \
	ttbdf.o \
	ttcmap.o \
	ttcolr.o \
	ttcpal.o \
	ttkern.o \
	ttload.o \
	ttmtx.o \
	ttpost.o \
	ttsbit.o \
	ttsvg.o \
	sfwoff.o \
	ftgrays.o \
	ftsmooth.o \
	ftsvg.o \
	truetype.o \
	ttdriver.o \
	ttgload.o \
	ttgxvar.o \
	ttinterp.o \
	ttobjs.o \
	ttpload.o \
	winfnt.o \
	adler32.o \
	crc32.o \
	ftgzip.o \
	inffast.o \
	inflate.o \
	inftrees.o \
	zutil.o \
	ftlzw.o \
	pcf.o \
	autofit.o \
	type1.o \
	type42.o \
	raster.o \
	svg.o

all: $(OBJS)
	$(AR) rcs $(PREFIX)/lib/libfreetype.a $(OBJS)

ftadvanc.o: src/base/ftadvanc.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftbase.o: src/base/ftbase.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftbbox.o: src/base/ftbbox.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftbdf.o: src/base/ftbdf.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftbitmap.o: src/base/ftbitmap.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftcalc.o: src/base/ftcalc.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftcid.o: src/base/ftcid.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftcolor.o: src/base/ftcolor.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftdbgmem.o: src/base/ftdbgmem.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftdebug.o: src/base/ftdebug.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

fterrors.o: src/base/fterrors.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftfntfmt.o: src/base/ftfntfmt.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftfstype.o: src/base/ftfstype.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftgasp.o: src/base/ftgasp.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftgloadr.o: src/base/ftgloadr.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftglyph.o: src/base/ftglyph.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftgxval.o: src/base/ftgxval.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

fthash.o: src/base/fthash.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftinit.o: src/base/ftinit.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftlcdfil.o: src/base/ftlcdfil.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftmac.o: src/base/ftmac.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftmm.o: src/base/ftmm.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftobjs.o: src/base/ftobjs.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftotval.o: src/base/ftotval.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftoutln.o: src/base/ftoutln.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftpatent.o: src/base/ftpatent.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftpfr.o: src/base/ftpfr.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftpsprop.o: src/base/ftpsprop.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftrfork.o: src/base/ftrfork.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftsnames.o: src/base/ftsnames.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftstream.o: src/base/ftstream.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftstroke.o: src/base/ftstroke.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftsynth.o: src/base/ftsynth.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftsystem.o: src/base/ftsystem.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

fttrigon.o: src/base/fttrigon.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

fttype1.o: src/base/fttype1.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftutil.o: src/base/ftutil.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftwinfnt.o: src/base/ftwinfnt.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

md5.o: src/base/md5.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

bdf.o: src/bdf/bdf.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

bdfdrivr.o: src/bdf/bdfdrivr.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

bdflib.o: src/bdf/bdflib.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftbzip2.o: src/bzip2/ftbzip2.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftcache.o: src/cache/ftcache.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftcbasic.o: src/cache/ftcbasic.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftccache.o: src/cache/ftccache.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftccmap.o: src/cache/ftccmap.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftcglyph.o: src/cache/ftcglyph.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftcimage.o: src/cache/ftcimage.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftcmanag.o: src/cache/ftcmanag.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftcmru.o: src/cache/ftcmru.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftcsbits.o: src/cache/ftcsbits.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cff.o: src/cff/cff.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cffcmap.o: src/cff/cffcmap.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cffdrivr.o: src/cff/cffdrivr.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cffgload.o: src/cff/cffgload.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cffload.o: src/cff/cffload.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cffobjs.o: src/cff/cffobjs.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cffparse.o: src/cff/cffparse.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cidgload.o: src/cid/cidgload.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cidload.o: src/cid/cidload.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cidobjs.o: src/cid/cidobjs.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cidparse.o: src/cid/cidparse.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cidriver.o: src/cid/cidriver.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

type1cid.o: src/cid/type1cid.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

otvalid.o: src/otvalid/otvalid.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

otvbase.o: src/otvalid/otvbase.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

otvcommn.o: src/otvalid/otvcommn.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

otvgdef.o: src/otvalid/otvgdef.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

otvgpos.o: src/otvalid/otvgpos.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

otvgsub.o: src/otvalid/otvgsub.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

otvjstf.o: src/otvalid/otvjstf.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

otvmath.o: src/otvalid/otvmath.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

otvmod.o: src/otvalid/otvmod.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pfr.o: src/pfr/pfr.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pfrcmap.o: src/pfr/pfrcmap.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pfrdrivr.o: src/pfr/pfrdrivr.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pfrgload.o: src/pfr/pfrgload.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pfrload.o: src/pfr/pfrload.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pfrobjs.o: src/pfr/pfrobjs.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pfrsbit.o: src/pfr/pfrsbit.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

afmparse.o: src/psaux/afmparse.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

cffdecode.o: src/psaux/cffdecode.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psarrst.o: src/psaux/psarrst.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psaux.o: src/psaux/psaux.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psauxmod.o: src/psaux/psauxmod.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psblues.o: src/psaux/psblues.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psconv.o: src/psaux/psconv.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pserror.o: src/psaux/pserror.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psfont.o: src/psaux/psfont.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psft.o: src/psaux/psft.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pshints.o: src/psaux/pshints.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psintrp.o: src/psaux/psintrp.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psobjs.o: src/psaux/psobjs.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psread.o: src/psaux/psread.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psstack.o: src/psaux/psstack.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pshalgo.o: src/pshinter/pshalgo.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pshglob.o: src/pshinter/pshglob.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pshinter.o: src/pshinter/pshinter.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pshmod.o: src/pshinter/pshmod.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pshrec.o: src/pshinter/pshrec.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psmodule.o: src/psnames/psmodule.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

psnames.o: src/psnames/psnames.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftraster.o: src/raster/ftraster.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftbsdf.o: src/sdf/ftbsdf.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftsdf.o: src/sdf/ftsdf.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftsdfcommon.o: src/sdf/ftsdfcommon.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftsdfrend.o: src/sdf/ftsdfrend.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pngshim.o: src/sfnt/pngshim.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

sfdriver.o: src/sfnt/sfdriver.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

sfobjs.o: src/sfnt/sfobjs.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttbdf.o: src/sfnt/ttbdf.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttcmap.o: src/sfnt/ttcmap.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttcolr.o: src/sfnt/ttcolr.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttcpal.o: src/sfnt/ttcpal.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttkern.o: src/sfnt/ttkern.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttload.o: src/sfnt/ttload.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttmtx.o: src/sfnt/ttmtx.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttpost.o: src/sfnt/ttpost.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttsbit.o: src/sfnt/ttsbit.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttsvg.o: src/sfnt/ttsvg.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

sfwoff.o: src/sfnt/sfwoff.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftgrays.o: src/smooth/ftgrays.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftsmooth.o: src/smooth/ftsmooth.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftsvg.o: src/svg/ftsvg.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

truetype.o: src/truetype/truetype.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttdriver.o: src/truetype/ttdriver.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttgload.o: src/truetype/ttgload.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttgxvar.o: src/truetype/ttgxvar.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttinterp.o: src/truetype/ttinterp.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttobjs.o: src/truetype/ttobjs.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ttpload.o: src/truetype/ttpload.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

winfnt.o: src/winfonts/winfnt.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

adler32.o: src/gzip/adler32.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

crc32.o: src/gzip/crc32.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftgzip.o: src/gzip/ftgzip.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

inffast.o: src/gzip/inffast.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

inflate.o: src/gzip/inflate.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

inftrees.o: src/gzip/inftrees.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

zutil.o: src/gzip/zutil.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

ftlzw.o: src/lzw/ftlzw.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

pcf.o: src/pcf/pcf.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

autofit.o: src/autofit/autofit.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

type1.o: src/type1/type1.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

type42.o: src/type42/type42.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

raster.o: src/raster/raster.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<

svg.o: src/svg/svg.c
	$(CC) -DFT2_BUILD_LIBRARY -Dft_memcpy=memcpy -I./include -I$(PREFIX)/include $(CFLAGS) -c -w $<
