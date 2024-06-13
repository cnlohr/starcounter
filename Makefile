all : starslidegen

CFLAGS:=-g -O2
LDFLAGS:=-lX11 -lm
START_FRAME:=721

starslidegen : starslidegen.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

framedata/000000.bmp : starslidegen allstarlist.txt
	./starslidegen allstarlist.txt

video.mp4 : framedata/000000.bmp
	ffmpeg -f image2 -start_number $(START_FRAME) -framerate 60 -i framedata/%06d.bmp -f f32le -ss 12.0 -ar 48000 -i audio.dat -vcodec libx264 -crf 21 -c:a aac video.mp4

allstarlist.txt :
	echo "You have to execute allstar_gen.sh for each user, i.e."
	echo "$ ./allstar_gen.sh cnlohr"
	echo "$ ./allstar_gen.sh tsoding"

clobber : clean
	rm -rf allstarlist.txt

clean :
	rm -rf *.o *~ starslidegen framedata video.mp4

