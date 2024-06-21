# starcounter

Uses `gh` github cli to retrieve detailed data about stars from all of a user's repositories.

Then turns it into a pretty video.  (Linux only, unless you really want to install ffmpeg and bash on Windows)

Features usage of:
 * stb_truetype.h
 * olive.c
 * rawdraw_sf.h

[![YT Video](https://img.youtube.com/vi/oONCBe2fzv4/hqdefault.jpg)](https://youtu.be/oONCBe2fzv4)


### Requirements

```
sudo apt-get install gh jq ffmpeg build-essential
```

```
gh auth login
```

### Operation

Usage:

```
make clobber
./allstar_gen.sh {username}
./allstar_gen.sh {username}
make video.mp4
```

Which will output `video.mp4`

Optionally: `./parseallstars.sh` if just one user to make a CSV to `starsbydate.txt`.

If you wish to change the width/height, you can edit it in `starslidegen.c` here:

```
#define WINDOWW 1920
#define WINDOWH 1080
```

**NOTE**: Because we are using bmp's it's fast, but longer runs could use in upwards of 40GB of intermediate files.
