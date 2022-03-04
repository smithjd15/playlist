# Playlist

Playlist is a tool for media playlist files. It can convert between common playlist formats; change and remove metadata; concatenate multiple playlists; remove duplicate entries; randomize entry order; insert, append and remove entries; remove unfound target entries and image targets; identify unfound, unfound image, duplicate, network, network image, and unique (between multiple playlists) targets; get metadata from local targets; and transform local targets into absolute paths, or paths relative to the out playlist or any arbitrary path.

Playlist supports the common title and duration metadata for m3u, pls, and xspf/jspf, and also creator (artist), album, annotation (comment), identifier, image, info, and album track for m3u and xspf/jspf. It also supports m3u and xspf/jspf playlist artist, title, and image, and cue TITLE (playlist title and title), PERFORMER (playlist artist and artist), and REM (comment).

### Inspect
#### Playlist can list all or certain targets, and provide an entry overview and summary.

To list playlist targets, pick one of the following options,

-l, list targets only.  
-L, list track with targets.  
-P, list playlists with targets.  
-S, list playlist artist with targets.  
-J, list playlist title with targets.  
-K, list playlist image with targets.  
-A, list artists with targets.  
-T, list titles with targets.  
-M, list albums with targets.  
-E, list comments with targets.  
-D, list identifiers with targets.  
-G, list image with targets.  
-N, list info with targets.  

and combine it with one of the following modifiers:

all, list all entry targets;  
dupe, list duplicate entry targets;  
image, list all entry image targets;  
net, list entry network streams;  
netimg, list entry network image targets;  
unfound, list entry unfound targets (-s to also verify network targets);  
unfoundimg, list entry unfound image targets (-s to also verify network targets);  
unique, list entry targets unique to a playlist.  

##### Example listing all targets:

playlist -l all inlist.m3u

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
playlist -L all -x -u -R -w bar/outlist.m3u inlist.m3u

### Convert
#### Playlist can convert between m3u, pls, cue, xspf and jspf formats.

##### Example converting an m3u to an xspf:

playlist -w outlist.xspf inlist.m3u

##### Example converting an xspf to a minimal pls, stripping all information but the targets:

playlist -m -w outlist.pls inlist.xspf

### Transform
#### Playlist can transform targets absolutely or relatively.

##### Example relocating an xspf, transforming the targets to paths relative to foo:

playlist -R -w foo/outlist.xspf inlist.xspf

##### Example transforming the targets of an xspf to paths relative to /foo/bar:

playlist -B /foo/bar -w outlist.xspf inlist.xspf

##### Example transforming the targets of an m3u into absolute paths:

playlist -O -w outlist.m3u inlist.m3u

##### Example uri-encoding the targets of an m3u into absolute paths:

playlist -I -w outlist.m3u inlist.m3u

### Reconstruct
#### Playlist can insert or append, change, and remove entries, drop duplicate and unfound target entries and images, randomize the order of entries, and concatenate multiple playlists.

The -a, -e, and -r options can be used more than once.

##### Example inserting an entry:

playlist -a 22:/foo/bar/target.ext -w outlist.m3u inlist.m3u

##### Example appending an entry:

playlist -a /foo/bar/target.ext -w outlist.m3u inlist.m3u

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

##### Example setting a playlist title (cue, m3u or xspf/jspf):

playlist -t "Some Title" -w outlist.xspf inlist.xspf

##### Example setting a playlist artist (cue, m3u or xspf/jspf):

playlist -b "Some Creator" -w outlist.xspf inlist.xspf

##### Example setting a playlist image (m3u or xspf/jspf):

playlist -g "/foo/bar/image.ext" -w outlist.xspf inlist.xspf
