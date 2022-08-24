/* playlist for media playlist files
 * Copyright (C) 2021, 2022 James D. Smith
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "playlist.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>

#include <unistd.h>

void help() {
  std::cout << "playlist version " << ver << std::endl;
  std::cout << "Copyright (C) 2021, 2022 James D. Smith" << std::endl;
  std::cout << std::endl;
  std::cout << "Usage: playlist [-l|-L|-P|-J|-S|-K|-A|-T|-M|-E|-D|-G|-N "
               "dupe|image|net|netimg|target|unfound|unfoundimg|unique] [-p] "
               "[-f path] [-z] [[-O|-I]|[-R|-B path]] [-a [track:]target] "
               "[-e track:FIELD=value] "
#ifdef LIBCURL
#ifdef TAGLIB
               "[-r track|target] [-s] [-i] [-d] [-u] [-n] [-m] "
#else
               "[-r track|target] [-s] [-d] [-u] [-n] [-m] "
#endif
#else
#ifdef TAGLIB
               "[-r track|target] [-i] [-d] [-u] [-n] [-m] "
#else
               "[-r track|target] [-d] [-u] [-n] [-m] "
#endif
#endif
               "[-b artist] [-t title] [-g image] [-q] [-v] [-x] [-o] "
               "[-w outfile.ext] infile..."
            << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "\t-l LIST Targets only" << std::endl;
  std::cout << "\t-L LIST Tracks and targets" << std::endl;
  std::cout << "\t-P LIST Playlist and targets" << std::endl;
  std::cout << "\t-J LIST Playlist title and targets" << std::endl;
  std::cout << "\t-S LIST Playlist artist and targets" << std::endl;
  std::cout << "\t-K LIST Playlist image and targets" << std::endl;
  std::cout << "\t-A LIST Artists and targets" << std::endl;
  std::cout << "\t-T LIST Titles and targets" << std::endl;
  std::cout << "\t-M LIST Albums and targets" << std::endl;
  std::cout << "\t-E LIST Comments and targets" << std::endl;
  std::cout << "\t-D LIST Identifiers and targets" << std::endl;
  std::cout << "\t-G LIST Image and targets" << std::endl;
  std::cout << "\t-N LIST Info and targets" << std::endl;
  std::cout
      << "\t-p List all entry targets in absolute paths (same as -O -l target)"
      << std::endl;
  std::cout << std::endl;
  std::cout << "\t-x Preview changes (with -w)" << std::endl;
  std::cout << "\t-f In playlist relative local target base path" << std::endl;
  std::cout << "\t-z Ignore in playlist parse errors" << std::endl;
  std::cout << "\t-w Out playlist file (.cue, .jspf, .m3u, .pls, .xspf)"
            << std::endl;
  std::cout << "\t-o Clobber playlist (with -w out; or first in)" << std::endl;
  std::cout << "\t-R Out playlist local targets relative to out playlist"
            << std::endl;
  std::cout << "\t-B Out playlist local targets base path" << std::endl;
  std::cout << "\t-O Out playlist local targets in absolute paths" << std::endl;
  std::cout << "\t-I Out playlist local targets in file URI scheme (implied -O)"
            << std::endl;
  std::cout << "\t-t Set title for out playlist" << std::endl;
  std::cout << "\t-b Set artist for out playlist" << std::endl;
  std::cout << "\t-g Set image for out playlist" << std::endl;
  std::cout << "\t-a Insert or append entry" << std::endl;
  std::cout << "\t-e track:FIELD=value Set entry field" << std::endl;
  std::cout << "\t-r Remove entry matching track or target" << std::endl;
  std::cout << "\t-d Remove duplicate entries from out playlist" << std::endl;
  std::cout << "\t-u Remove unfound target entries and images from out playlist"
            << std::endl;
  std::cout << "\t-n Out playlist entries in random order" << std::endl;
  std::cout << "\t-m Minimal out playlist (targets only)" << std::endl;
#ifdef LIBCURL
  std::cout << "\t-s Verify network targets" << std::endl;
#endif
#ifdef TAGLIB
  std::cout << "\t-i Get entry metadata from local targets" << std::endl;
#endif
  std::cout << std::endl;
  std::cout << "\t-q Quiet" << std::endl;
  std::cout << "\t-v Verbose" << std::endl;
  std::cout << "\t-h This help" << std::endl;
  std::cout << std::endl;
  std::cout
      << "FIELD can be one of: (ta)rget, (ar)tist, (ti)tle, (al)bum, "
         "(co)mment, (id)entifier, (im)age, (in)fo, album (tr)ack, (du)ration"
      << std::endl;
  std::cout << std::endl;
  std::cout << "LIST can be one of: dupe, image, net, netimg, target, unfound, "
               "unfoundimg, or unique (multiple infiles)"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Exit codes:" << std::endl;
  std::cout << "0: Success or quiet flag" << std::endl;
  std::cout
      << "1: Out playlist has unfound entry targets or images; or multiple "
         "artists, images, or titles; or a dupe or unfound list contains at "
         "least one entry; or an in playlist was not found or was parsed with "
         "errors"
#ifdef TAGLIB
         "; or metadata could not be read from a target"
#endif
      << std::endl;
  std::cout << "2: IO, parse, or command line error" << std::endl;
}

int main(int argc, char **argv) {
  fs::path base, image, prepend;
  std::string artist, title;
  List list;
  std::vector<std::string> addItems, changeItems, removeItems;
  Playlist *outPlaylist;

  auto parseList = [](const std::string &arg) {
    flags[2] = (arg == "dupe");
    flags[3] = (arg == "image");
    flags[4] = (arg == "net");
    flags[5] = (arg == "netimg");
    flags[1] = (arg == "target");
    flags[6] = (arg == "unfound");
    flags[7] = (arg == "unfoundimg");
    flags[8] = (arg == "unique");
  };

  const auto transformPath = [&](const fs::path &basePath, fs::path &path) {
    fs::path target = absPath(basePath, path);

    if (flags[14]) {
      path = "file://" + percentEncode(target);
    } else if (flags[23]) {
      path = target;
    } else if (flags[25]) {
      path = target.lexically_relative(list.playlist.parent_path());
    } else if (!base.empty()) {
      path = target.lexically_relative(base);
    }
  };

  int c;
#ifdef LIBCURL
#ifdef TAGLIB
  while (
      (c = getopt(
           argc, argv,
           "a:A:b:B:dD:e:E:f:g:G:iIJ:K:l:L:mM:nN:oOpP:r:RsS:t:T:uvw:xzqh")) !=
      -1) {
#else
  while ((c = getopt(
              argc, argv,
              "a:A:b:B:dD:e:E:f:g:G:IJ:K:l:L:mM:nN:oOpP:r:RsS:t:T:uvw:xzqh")) !=
         -1) {
#endif
#else
#ifdef TAGLIB
  while ((c = getopt(
              argc, argv,
              "a:A:b:B:dD:e:E:f:g:G:iJ:K:Il:L:mM:nN:oOpP:r:RS:t:T:uvw:xzqh")) !=
         -1) {
#else
  while ((c = getopt(
              argc, argv,
              "a:A:b:B:dD:e:E:f:g:G:IJ:K:l:L:mM:nN:oOpP:r:RS:t:T:uvw:xzqh")) !=
         -1) {
#endif
#endif
    switch (c) {
    case 'A':
      flags[0] = true;

      parseList(optarg);

      break;
    case 'a':
      addItems.emplace_back(optarg);

      break;
    case 'b':
      artist = optarg;

      break;
    case 'B':
      base = absPath(fs::current_path(), optarg);

      break;
    case 'd':
      flags[9] = true;

      break;
    case 'D':
      flags[10] = true;

      parseList(optarg);

      break;
    case 'e':
      changeItems.emplace_back(optarg);

      break;
    case 'E':
      flags[11] = true;

      parseList(optarg);

      break;
    case 'f':
      prepend = absPath(fs::current_path(), fs::path(optarg));

      break;
    case 'g':
      image = fs::path(optarg);

      break;
    case 'G':
      flags[12] = true;

      parseList(optarg);

      break;
#ifdef TAGLIB
    case 'i':
      flags[13] = true;

      break;
#endif
    case 'I':
      flags[14] = true;

      break;
    case 'J':
      flags[15] = true;

      parseList(optarg);

      break;
    case 'K':
      flags[16] = true;

      parseList(optarg);

      break;
    case 'l':
      flags[17] = true;

      parseList(optarg);

      break;
    case 'L':
      parseList(optarg);

      break;
    case 'm':
      flags[18] = true;

      break;
    case 'M':
      flags[19] = true;

      parseList(optarg);

      break;
    case 'n':
      flags[20] = true;

      break;
    case 'N':
      flags[21] = true;

      parseList(optarg);

      break;
    case 'o':
      flags[22] = true;

      break;
    case 'O':
      flags[23] = true;

      break;
    case 'p':
      flags[1] = true;
      flags[17] = true;
      flags[23] = true;
      flags[30] = true;
      list.playlist = fs::temp_directory_path() /= "playlisttargets.m3u";

      break;
    case 'P':
      flags[24] = true;

      parseList(optarg);

      break;
    case 'r':
      removeItems.emplace_back(optarg);

      break;
    case 'R':
      flags[25] = true;

      break;
#ifdef LIBCURL
    case 's':
      flags[26] = true;

      break;
#endif
    case 'S':
      flags[27] = true;

      parseList(optarg);

      break;
    case 't':
      title = optarg;

      break;
    case 'T':
      flags[28] = true;

      parseList(optarg);

      break;
    case 'u':
      flags[29] = true;

      break;
    case 'w':
      list.playlist = absPath(fs::current_path(), optarg);

      break;
    case 'x':
      flags[30] = true;

      break;
    case 'z':
      flags[31] = true;

      break;
    case 'v':
      flags[32] = true;

      break;
    case 'q':
      flags[33] = true;

      break;
    case 'h':
      help();

      return 0;
    case '?':
      return 2;
    default:
      return 2;
    }
  }

  for (; optind < argc; optind++) {
    const fs::path inPl = absPath(fs::current_path(), argv[optind]);
    Entries entries;

    if (fs::exists(inPl)) {
      playlist(inPl)->parse(entries);

      if (flags[32])
        std::cout << "Parsed " << entries.size() << " entries"
                  << " from playlist file: " << inPl << std::endl;

      list.entries.insert(list.entries.end(), entries.begin(), entries.end());

      if (list.playlist.empty() && flags[22])
        list.playlist = inPl;
    } else {
      cwar << "Skipping unfound file: " << inPl << std::endl;
    }
  }

  for (Entries::iterator it = list.entries.begin(); it != list.entries.end();
       it++) {
    auto computeTargets = [&](fs::path &target, bool &local, bool &valid) {
      target = processTarget(target.string());

      if (!prepend.empty())
        target = absPath(prepend, target);

      local = !isUri(target.string());
      valid = validTarget(absPath(it->playlist.parent_path(), target));
    };

    if (!it->playlistImage.empty()) {
      fs::path plImage = processTarget(it->playlistImage.string());

      if (list.image.empty() || (plImage != list.image))
        list.images++;

      if (list.image.empty() || (!list.validImage && (plImage != list.image))) {
        list.image = it->playlistImage;

        computeTargets(list.image, list.localImage, list.validImage);
      }
    }

    if (!it->playlistArtist.empty()) {
      if (list.artist.empty() || (it->playlistArtist != list.artist))
        list.artists++;

      if (list.artist.empty())
        list.artist = it->playlistArtist;
    }

    if (!it->playlistTitle.empty()) {
      if (list.title.empty() || (it->playlistTitle != list.title))
        list.titles++;

      if (list.title.empty())
        list.title = it->playlistTitle;
    }

    computeTargets(it->target, it->localTarget, it->validTarget);

    if (!it->image.empty())
      computeTargets(it->image, it->localImage, it->validImage);

    if (it->localTarget) {
#ifdef TAGLIB
      if (it->validTarget && flags[13])
        fetchMetadata(*it);

#endif
      if (!list.relative)
        list.relative =
            (it->target.is_relative() || !it->target.has_parent_path());
    }

    it->duplicateTarget = find(*it, list.entries) < it;
  }

  if (list.playlist.empty()) {
    if (flags[30]) {
      std::cerr << "-x option requires an out playlist (-w)" << std::endl;

      return 2;
    }
  } else {
    if (fs::exists(list.playlist) && !flags[22] && !flags[30]) {
      std::cerr << "File exists: " << list.playlist << std::endl;

      return 2;
    }

    if ((!base.empty() && flags[23]) || (flags[23] && flags[25]) ||
        (!base.empty() && flags[14]) || (flags[14] && flags[25])) {
      std::cerr << "Cannot combine absolute and relative path transforms"
                << std::endl;

      return 2;
    }

    outPlaylist = playlist(list.playlist);

    for (Entries::iterator it = list.entries.begin(); it != list.entries.end();
         it++)
      it->track = std::distance(list.entries.begin(), it) + 1;

    if (!image.empty()) {
      list.image = image;
      list.localImage = !isUri(list.image.string());
      list.validImage = validTarget(list.image);
      list.images = !image.empty();
    }

    if (!artist.empty()) {
      list.artist = artist;
      list.artists = !artist.empty();
    }

    if (!title.empty()) {
      list.title = title;
      list.titles = !title.empty();
    }

    for (const std::string &addItem : addItems) {
      std::pair<std::string, std::string> pair = split(addItem, ":");
      Entry entry;
      Entries::iterator index;

      entry.playlist = fs::current_path().append(".");

      if (!pair.first.empty() &&
          std::all_of(pair.first.begin(), pair.first.end(), isdigit)) {
        int track = std::stoi(pair.first);

        entry.target = processTarget(pair.second);
        entry.track = track;

        if ((track - 1) < list.entries.size()) {
          index = list.entries.begin() + (track - 1);
        } else {
          index = list.entries.end();
        }
      } else {
        entry.target = processTarget(addItem);
        entry.track = list.entries.size() + 1;

        index = list.entries.end();
      }

      entry.localTarget = !isUri(entry.target.string());
      entry.validTarget = validTarget(entry.target);

      list.entries.emplace(index, entry);
    }

    for (const std::string &changeItem : changeItems) {
      std::pair<std::string, std::string> pair1 = split(changeItem, ":");
      std::pair<std::string, std::string> pair2 = split(pair1.second, "=");
      const std::string track = pair1.first, key = pair2.first,
                        value = pair2.second;
      Entry entry;

      auto invalid = [&changeItem]() {
        std::cerr << "Parse error: " << changeItem << std::endl;

        std::exit(2);
      };

      auto setEntry = [&](Entry &entry) {
        entry.playlist = fs::current_path().append(".");

        if (key == "ta") {
          entry.target = processTarget(value);
          entry.localTarget = !isUri(entry.target.string());
          entry.validTarget = validTarget(entry.target);
        } else if (key == "ar") {
          entry.artist = value;
        } else if (key == "ti") {
          entry.title = value;
        } else if (key == "al") {
          entry.album = value;
        } else if (key == "co") {
          entry.comment = value;
        } else if (key == "id") {
          entry.identifier = value;
        } else if (key == "im") {
          entry.image = processTarget(value);
          entry.localImage = !isUri(entry.image.string());
          entry.validImage = validTarget(entry.image);
        } else if (key == "in") {
          entry.info = value;
        } else if (key == "tr") {
          if (!std::all_of(value.begin(), value.end(), isdigit))
            invalid();
          entry.albumTrack = value.empty() ? 0 : std::stoi(value);
        } else if (key == "du") {
          if (!std::all_of(value.begin(), value.end(), isdigit))
            invalid();
          entry.duration = value.empty() ? 0 : (std::stoi(value) * 1000);
        } else {
          invalid();
        }
      };

      if (track.empty() || !std::all_of(track.begin(), track.end(), isdigit))
        invalid();
      if (key.empty())
        invalid();
      if (key == pair1.second)
        invalid();

      for (Entry &entry : list.entries)
        if (entry.track == std::stoi(track))
          setEntry(entry);
    }

    for (const std::string &removeItem : removeItems) {
      for (Entry &entry : list.entries) {
        if (std::all_of(removeItem.begin(), removeItem.end(), isdigit)) {
          if (entry.track == std::stoi(removeItem))
            entry.target.clear();
        } else {
          if (absPath(fs::current_path(), entry.target) ==
              absPath(fs::current_path(), removeItem))
            entry.target.clear();
        }
      }
    }

    if (flags[6])
      std::shuffle(list.entries.begin(), list.entries.end(),
                   std::default_random_engine());

    for (Entries::iterator it = list.entries.begin();
         it != list.entries.end();) {
      it->duplicateTarget = find(*it, list.entries) < it;

      if (it->target.empty() || (!it->validTarget && flags[29]) ||
          (it->duplicateTarget && flags[9])) {

        it = list.entries.erase(it);

        continue;
      }

      it->track = std::distance(list.entries.begin(), it) + 1;

      if (it->localTarget) {
        transformPath(it->playlist.parent_path(), it->target);

        it->validTarget = fs::exists(
            absPath(list.playlist.parent_path(), processTarget(it->target)));
      }

      if (!it->image.empty()) {
        if (!it->validImage && flags[29]) {
          it->image.clear();
        } else if (it->localImage) {
          transformPath(it->playlist.parent_path(), it->image);
          it->validImage = fs::exists(
              absPath(list.playlist.parent_path(), processTarget(it->image)));
        }
      }

      it->playlist = list.playlist;

      it++;
    }

    if (!list.image.empty()) {
      if (!list.validImage && flags[29]) {
        list.image.clear();
      } else {
        if (list.localImage) {
          transformPath(list.playlist.parent_path(), list.image);
          list.validImage = fs::exists(
              absPath(list.playlist.parent_path(), processTarget(list.image)));
        }
      }
    }

    if (!base.empty() || flags[25])
      list.relative = (!base.empty() || flags[25]);

    outPlaylist->writePreProcess(list);

    if (flags[32])
      std::cout << "Generated playlist: " << list.entries.size() << " entries"
                << std::endl;
  }

  if (list.entries.empty()) {
    if ((cwar.rdbuf()->in_avail() != 0) && !flags[33])
      std::cerr << cwar.rdbuf();
    std::cerr << "Nothing to do!" << std::endl;

    std::cout << "playlist -h for usage information" << std::endl;

    return 2;
  }

  if (flags[1] || flags[2] || flags[3] || flags[4] || flags[5] || flags[6] ||
      flags[7] || flags[8])
    ::list(list);

  for (const Entry &entry : list.entries) {
    if (entry.duration > 0)
      list.knownDuration += entry.duration;

    list.dupeTargets += entry.duplicateTarget;
    list.netTargets += !entry.localTarget;
    list.unfoundTargets += !entry.validTarget;

    if (!entry.image.empty()) {
      list.netImages += !entry.localImage;
      list.unfoundImages += !entry.validImage;
    }
  }

  if (!list.playlist.empty()) {
    if (list.unfoundTargets > 0)
      cwar << "WARNING: out playlist has " << list.unfoundTargets
           << " unfound entry target(s)" << std::endl;

    if (!flags[18]) {
      if (list.unfoundImages > 0)
        cwar << "WARNING: out playlist has " << list.unfoundImages
             << " unfound entry image(s)" << std::endl;

      if (!list.image.empty() && !list.validImage)
        cwar << "WARNING: out playlist image not found" << std::endl;

      if (list.artists > 1)
        cwar << "WARNING: 1 of " << list.artists
             << " playlist artists auto-selected" << std::endl;

      if (list.images > 1)
        cwar << "WARNING: 1 of " << list.images
             << " playlist images auto-selected" << std::endl;

      if (list.titles > 1)
        cwar << "WARNING: 1 of " << list.titles
             << " playlist titles auto-selected" << std::endl;
    }

    if (flags[30]) {
      show(list);
    } else {
      if (outPlaylist->write(list)) {
        if (flags[32])
          std::cout << "Playlist successfully written: " << list.playlist
                    << std::endl;
      } else {
        std::cerr << "Write fail: " << list.playlist << std::endl;

        return 2;
      }
    }
  } else {
    show(list);
  }

  const bool cwarEmpty = (cwar.rdbuf()->in_avail() == 0);

  if (!cwarEmpty && !flags[33])
    std::cerr << cwar.rdbuf();

  return !flags[33] ? !cwarEmpty : 0;
}
