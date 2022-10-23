# Playlist

Playlist is a tool for media playlist files. It can convert between common playlist formats; change and remove metadata; concatenate multiple playlists; remove duplicate entries; randomize entry order; insert, append, reorder and remove entries; remove unfound target entries and images; identify unfound or network targets or images, and identify duplicate or unique (between multiple playlists) targets; get metadata from local targets; and transform local targets and images into absolute paths, or paths relative to the out playlist or any arbitrary path.

Playlist writes metadata into extension fields when available in destination playlist types lacking native field support, unless the minimal flag is specified. This does not usually make playlists feature-par, because a program loading the playlist might not be capable of reading this information.
<br/>

| | asx | cue | jspf | m3u | pls | wpl | xspf |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| playlist artist | x | x | x | x | | x | x |
| playlist image | | | x | x | | | x |
| playlist title | x | x | x | x | | x | x |
| | | | | | | | |
| album | | | x | | | | x |
| album track | | | x | | | | x |
| artist | x | x | x | | | | x |
| comment | | x | x | | | | x |
| duration | | | x | x | x | | x |
| identifier | | | x | | | | x |
| image | | | x | | | | x |
| info | x | | x | | | | x |
| title | x | x | x | x | x | | x |

^ Native playlist feature support

### Inspect
#### Playlist can list all or certain targets or images, and provide an entry overview and summary.

Pick one of the following list options,

dupe, list duplicate entry targets;  
image, list all entry images;  
net, list entry network streams;  
netimg, list entry network images;  
target, list all entry targets;  
unfound, list entry unfound targets (-s to also verify network targets);  
unfoundimg, list entry unfound images (-s to also verify network targets);  
unique, list entry targets unique to a playlist;  

and append it to one of the following flags:

-l, list targets or images only.  
-L, list track with targets or images.  
-P, list playlists with targets or images.  
-S, list playlist artist with targets or images.  
-J, list playlist title with targets or images.  
-K, list playlist image with targets or images.  
-A, list artists with targets or images.  
-T, list titles with targets or images.  
-M, list albums with targets or images.  
-E, list comments with targets or images.  
-D, list identifiers with targets or images.  
-G, list image with targets or images.  
-N, list info with targets or images.  

##### Example listing all targets:

playlist -l target inlist.m3u

##### Example listing duplicate targets and identifiers:

playlist -D dupe inlist.xspf

##### Example listing network targets and titles:

playlist -T net inlist.m3u

##### Example listing unfound targets and tracks:

playlist -L unfound inlist.m3u

##### Example listing unique targets and playlists:

playlist -P unique comparelist.m3u inlist.m3u

##### Example providing an entry overview and summary:

playlist inlist.m3u

##### Examples previewing changes without writing the out playlist:

playlist -x -u -R -w foo/outlist.m3u inlist.m3u  
playlist -L target -x -u -R -w bar/outlist.m3u inlist.m3u

### Convert
#### Playlist can convert between asx, cue, m3u, pls, wpl, xspf and jspf formats.

##### Example converting an m3u to an xspf:

playlist -w outlist.xspf inlist.m3u

##### Example converting an xspf to a minimal pls, stripping all information but the targets:

playlist -m -w outlist.pls inlist.xspf

### Transform
#### Playlist can transform local target and image paths absolutely or relatively.

##### Example relocating an xspf, transforming paths relative to foo:

playlist -R -w foo/outlist.xspf inlist.xspf

##### Example transforming paths relative to /foo/bar:

playlist -B /foo/bar -w outlist.xspf inlist.xspf

##### Example transforming paths absolutely:

playlist -O -w outlist.m3u inlist.m3u

##### Example transforming paths absolutely and into file URI scheme:

playlist -I -w outlist.m3u inlist.m3u

### Reconstruct
#### Playlist can insert or append, move, change, and remove entries, drop duplicate and unfound target entries and images, randomize the order of entries, and concatenate multiple playlists.

The -a, -c, -e, and -r options can be used more than once.

##### Example inserting an entry:

playlist -a 22:/foo/bar/target.ext -w outlist.m3u inlist.m3u

##### Example appending an entry:

playlist -a /foo/bar/target.ext -w outlist.m3u inlist.m3u

##### Example moving an entry from track 23 to track 22:

playlist -c 22:23 -w outlist.m3u inlist.m3u

##### Example changing the target of an entry:

playlist -e 23:ta=foo/bar/target.ext -w outlist.m3u inlist.m3u

##### Example changing the duration of an entry (in seconds):

playlist -e 23:du=254 -w outlist.m3u inlist.m3u

##### Example removing the title of an entry:

playlist -e 23:ti= -w outlist.m3u inlist.m3u

##### Examples removing matching entries:

playlist -r 22 -w outlist.m3u inlist.m3u  
playlist -r foo/bar/target.ext -w outlist.m3u inlist.m3u  
playlist -e 22:ta= -w outlist.m3u inlist.m3u

##### Example removing duplicate entries:

playlist -d -w outlist.m3u inlist.m3u

##### Example removing unfound entries and images:

playlist -u -w outlist.m3u inlist.m3u

##### Example randomizing playlist entries:

playlist -n -w outlist.m3u inlist.m3u

##### Example concatenating playlists:

playlist -w outlist.m3u inlist.m3u appendlist.pls

##### Example setting a playlist title (asx, cue, m3u, wpl or xspf/jspf):

playlist -t "Some Title" -w outlist.xspf inlist.xspf

##### Example setting a playlist artist (asx, cue, m3u or xspf/jspf):

playlist -b "Some Creator" -w outlist.xspf inlist.xspf

##### Example setting a playlist image (m3u or xspf/jspf):

playlist -g "/foo/bar/image.ext" -w outlist.xspf inlist.xspf
