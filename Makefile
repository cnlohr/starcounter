all : starslidegen

CFLAGS:=-g -O2
LDFLAGS:=-lX11 -lm

starslidegen : starslidegen.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

starslide.gif : starslidegen allstarlist.txt
	./starslidegen allstarlist.txt

clean :
	rm -rf *.o *~ starslidegen

