# starcounter


Uses `gh` github cli to retrieve detailed data about stars from all of a user's repositories.


### Requirements

```
sudo apt-get install gh jq
```

```
gh auth login
```


### Operation

Usage:

```
./allstar_gen.sh {username}
./parseallstars.sh
```

That outputs "starsbydate.txt"

