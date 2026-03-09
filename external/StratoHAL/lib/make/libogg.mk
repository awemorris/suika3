all: bitwise.o framing.o
	$(AR) rcs $(PREFIX)/lib/libogg.a bitwise.o framing.o

bitwise.o: src/bitwise.c
	$(CC) -I./include $(CFLAGS) -c $<

framing.o: src/framing.c
	$(CC) -I./include $(CFLAGS) -c $<
