# starcounter

Uses `gh` github cli to retrieve detailed data about stars from all of a user's repositories.

Then turns it into a pretty video.

[![YT Video](https://img.youtube.com/vi/oONCBe2fzv4/hqdefault.jpg)](https://www.youtube.com/embed/oONCBe2fzv4)


### Requirements

```
sudo apt-get install gh jq ffmpeg
```

```
gh auth login
```


### Operation

Usage:

```
rm -rf allstarlist.txt
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

