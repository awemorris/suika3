all: cdjpeg.o jaricom.o jcapimin.o jcapistd.o jcarith.o jccoefct.o jccolor.o jcdctmgr.o jchuff.o jcinit.o jcmainct.o jcmarker.o jcmaster.o jcomapi.o jcparam.o jcprepct.o jcsample.o jctrans.o jdapimin.o jdapistd.o jdarith.o jdatadst.o jdatasrc.o jdcoefct.o jdcolor.o jddctmgr.o jdhuff.o jdinput.o jdmainct.o jdmarker.o jdmaster.o jdmerge.o jdpostct.o jdsample.o jdtrans.o jerror.o jfdctflt.o jfdctfst.o jfdctint.o jidctflt.o jidctfst.o jidctint.o jmemansi.o jmemmgr.o jmemnobs.o jquant1.o jquant2.o jutils.o transupp.o
	$(AR) rcs $(PREFIX)/lib/libjpeg.a cdjpeg.o jaricom.o jcapimin.o jcapistd.o jcarith.o jccoefct.o jccolor.o jcdctmgr.o jchuff.o jcinit.o jcmainct.o jcmarker.o jcmaster.o jcomapi.o jcparam.o jcprepct.o jcsample.o jctrans.o jdapimin.o jdapistd.o jdarith.o jdatadst.o jdatasrc.o jdcoefct.o jdcolor.o jddctmgr.o jdhuff.o jdinput.o jdmainct.o jdmarker.o jdmaster.o jdmerge.o jdpostct.o jdsample.o jdtrans.o jerror.o jfdctflt.o jfdctfst.o jfdctint.o jidctflt.o jidctfst.o jidctint.o jmemansi.o jmemmgr.o jmemnobs.o jquant1.o jquant2.o jutils.o transupp.o

cdjpeg.o: cdjpeg.c
	$(CC) $(CFLAGS) -c $<

jaricom.o: jaricom.c
	$(CC) $(CFLAGS) -c $<

jcapimin.o: jcapimin.c
	$(CC) $(CFLAGS) -c $<

jcapistd.o: jcapistd.c
	$(CC) $(CFLAGS) -c $<

jcarith.o: jcarith.c
	$(CC) $(CFLAGS) -c $<

jccoefct.o: jccoefct.c
	$(CC) $(CFLAGS) -c $<

jccolor.o: jccolor.c
	$(CC) $(CFLAGS) -c $<

jcdctmgr.o: jcdctmgr.c
	$(CC) $(CFLAGS) -c $<

jchuff.o: jchuff.c
	$(CC) $(CFLAGS) -c $<

jcinit.o: jcinit.c
	$(CC) $(CFLAGS) -c $<

jcmainct.o: jcmainct.c
	$(CC) $(CFLAGS) -c $<

jcmarker.o: jcmarker.c
	$(CC) $(CFLAGS) -c $<

jcmaster.o: jcmaster.c
	$(CC) $(CFLAGS) -c $<

jcomapi.o: jcomapi.c
	$(CC) $(CFLAGS) -c $<

jcparam.o: jcparam.c
	$(CC) $(CFLAGS) -c $<

jcprepct.o: jcprepct.c
	$(CC) $(CFLAGS) -c $<

jcsample.o: jcsample.c
	$(CC) $(CFLAGS) -c $<

jctrans.o: jctrans.c
	$(CC) $(CFLAGS) -c $<

jdapimin.o: jdapimin.c
	$(CC) $(CFLAGS) -c $<

jdapistd.o: jdapistd.c
	$(CC) $(CFLAGS) -c $<

jdarith.o: jdarith.c
	$(CC) $(CFLAGS) -c $<

jdatadst.o: jdatadst.c
	$(CC) $(CFLAGS) -c $<

jdatasrc.o: jdatasrc.c
	$(CC) $(CFLAGS) -c $<

jdcoefct.o: jdcoefct.c
	$(CC) $(CFLAGS) -c $<

jdcolor.o: jdcolor.c
	$(CC) $(CFLAGS) -c $<

jddctmgr.o: jddctmgr.c
	$(CC) $(CFLAGS) -c $<

jdhuff.o: jdhuff.c
	$(CC) $(CFLAGS) -c $<

jdinput.o: jdinput.c
	$(CC) $(CFLAGS) -c $<

jdmainct.o: jdmainct.c
	$(CC) $(CFLAGS) -c $<

jdmarker.o: jdmarker.c
	$(CC) $(CFLAGS) -c $<

jdmaster.o: jdmaster.c
	$(CC) $(CFLAGS) -c $<

jdmerge.o: jdmerge.c
	$(CC) $(CFLAGS) -c $<

jdpostct.o: jdpostct.c
	$(CC) $(CFLAGS) -c $<

jdsample.o: jdsample.c
	$(CC) $(CFLAGS) -c $<

jdtrans.o: jdtrans.c
	$(CC) $(CFLAGS) -c $<

jerror.o: jerror.c
	$(CC) $(CFLAGS) -c $<

jfdctflt.o: jfdctflt.c
	$(CC) $(CFLAGS) -c $<

jfdctfst.o: jfdctfst.c
	$(CC) $(CFLAGS) -c $<

jfdctint.o: jfdctint.c
	$(CC) $(CFLAGS) -c $<

jidctflt.o: jidctflt.c
	$(CC) $(CFLAGS) -c $<

jidctfst.o: jidctfst.c
	$(CC) $(CFLAGS) -c $<

jidctint.o: jidctint.c
	$(CC) $(CFLAGS) -c $<

jmemansi.o: jmemansi.c
	$(CC) $(CFLAGS) -c $<

jmemmgr.o: jmemmgr.c
	$(CC) $(CFLAGS) -c $<

jmemnobs.o: jmemnobs.c
	$(CC) $(CFLAGS) -c $<

jquant1.o: jquant1.c
	$(CC) $(CFLAGS) -c $<

jquant2.o: jquant2.c
	$(CC) $(CFLAGS) -c $<

jutils.o: jutils.c
	$(CC) $(CFLAGS) -c $<

transupp.o: transupp.c
	$(CC) $(CFLAGS) -c $<
