# Playlist

Playlist is a tool for media playlist files. It can convert between common playlist formats; identify unfound, duplicate, network, and unique (between multiple playlists) entries; concatenate multiple playlists; remove duplicate and unfound entries; randomize entry order; append and remove entries; change and remove metadata; get metadata from local targets; and transform local targets into absolute paths, or paths relative to the out playlist or any arbitrary path.

Playlist supports the common title and duration metadata for m3u, pls, and xspf/jspf, and also creator (artist), album, annotation (comment), identifier, image, info, and album track for m3u and xspf/jspf. It also supports m3u and xspf/jspf playlist title and image.

### Inspect
#### Playlist can list all or certain targets, and provide an entry overview and summary.

To list playlist targets, pick one of the following options,

-l, list targets only.  
-L, list track with targets.  
-P, list playlists with targets.  
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
net, list entry network streams;  
unfound, list entry unfound targets (-s to also verify network targets);  
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

playlist -x -u -R -o foo/outlist.m3u inlist.m3u  
playlist -L all -x -u -R -o bar/outlist.m3u inlist.m3u

### Convert
#### Playlist can convert between m3u, pls, xspf and jspf formats.

##### Example converting an m3u to an xspf:

playlist -o outlist.xspf inlist.m3u

##### Example converting an xspf to a minimal pls, stripping all information but the targets:

playlist -m -o outlist.pls inlist.xspf

### Transform
#### Playlist can transform targets absolutely or relatively.

##### Example relocating an xspf, transforming the targets to paths relative to foo:

playlist -R -o foo/outlist.xspf inlist.xspf

##### Example transforming the targets of an xspf to paths relative to /foo/bar:

playlist -B /foo/bar -o outlist.xspf inlist.xspf

##### Example transforming the targets of an m3u into absolute paths:

playlist -O -o outlist.m3u inlist.m3u

##### Example uri-encoding the targets of an m3u into absolute paths:

playlist -I -o outlist.m3u inlist.m3u

### Reconstruct
#### Playlist can append, change, and remove entries, drop duplicate and unfound entries, randomize the order of entries, and concatenate multiple playlists.

The -a and -e options can be used more than once.

##### Example appending an entry:

playlist -a /foo/bar/target.ext -o outlist.m3u inlist.m3u

##### Example changing the target of an entry:

playlist -e 23:ta=foo/bar/target.ext -o outlist.m3u inlist.m3u

##### Example changing the duration of an entry (in seconds):

playlist -e 23:du=254 -o outlist.m3u inlist.m3u

##### Example removing the title of an entry:

playlist -e 23:ti= -o outlist.m3u inlist.m3u

##### Example removing an entry:

playlist -e 22:ta= -o outlist.m3u inlist.m3u

##### Example removing duplicate entries:

playlist -d -o outlist.m3u inlist.m3u

##### Example removing unfound entries:

playlist -u -o outlist.m3u inlist.m3u

##### Example randomizing playlist entries:

playlist -n -o outlist.m3u inlist.m3u

##### Example concatenating playlists:

playlist -o outlist.m3u inlist.m3u appendlist.pls

##### Example setting a playlist title (m3u or xspf/jspf):

playlist -t "Some Title" -o outlist.xspf inlist.xspf

##### Example setting a playlist image (m3u or xspf/jspf):

playlist -g "/foo/bar/image.ext" -o outlist.xspf inlist.xspf
