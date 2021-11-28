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

#define VER 1.43

void help() {
  std::cout << "playlist version " << VER << std::endl;
  std::cout << "Copyright (C) 2021, 2022 James D. Smith" << std::endl;
  std::cout << std::endl;
  std::cout << "Usage: playlist [-l|-L|-P|-J|-K|-A|-T|-M|-E|-D|-G|-N "
               "all|dupe|net|unfound|unique] [-p] [-f path] "
               "[[-O|-I]|[-R|-B path]] [-a target] [-e track:FIELD=value] "
#ifdef LIBCURL
#ifdef TAGLIB
               "[-r track] [-s] [-i] [-d] [-u] [-n] [-m] "
#else
               "[-r track] [-s] [-d] [-u] [-n] [-m] "
#endif
#else
#ifdef TAGLIB
               "[-r track] [-i] [-d] [-u] [-n] [-m] "
#else
               "[-r track] [-d] [-u] [-n] [-m] "
#endif
#endif
               "[-t title] [-g target] [-q] [-v] [-x] [-o outfile.ext] "
               "infile..."
            << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "\t-l LIST Targets only" << std::endl;
  std::cout << "\t-L LIST Tracks and targets" << std::endl;
  std::cout << "\t-P LIST Playlist and targets" << std::endl;
  std::cout << "\t-J LIST Playlist title and targets" << std::endl;
  std::cout << "\t-K LIST Playlist image and targets" << std::endl;
  std::cout << "\t-A LIST Artists and targets" << std::endl;
  std::cout << "\t-T LIST Titles and targets" << std::endl;
  std::cout << "\t-M LIST Albums and targets" << std::endl;
  std::cout << "\t-E LIST Comments and targets" << std::endl;
  std::cout << "\t-D LIST Identifiers and targets" << std::endl;
  std::cout << "\t-G LIST Image and targets" << std::endl;
  std::cout << "\t-N LIST Info and targets" << std::endl;
  std::cout << "\t-p Same as -l all" << std::endl;
  std::cout << std::endl;
  std::cout << "\t-x Preview changes (with -o)" << std::endl;
  std::cout << "\t-f In playlist relative local target base path" << std::endl;
  std::cout << "\t-o Out playlist file (.jspf, .m3u, .pls, .xspf)" << std::endl;
  std::cout << "\t-R Out playlist local targets relative to out playlist"
            << std::endl;
  std::cout << "\t-B Out playlist local targets base path" << std::endl;
  std::cout << "\t-O Out playlist local targets in absolute paths" << std::endl;
  std::cout << "\t-I Out playlist local targets in file URI scheme (implied -O)"
            << std::endl;
  std::cout << "\t-t Set title for out playlist" << std::endl;
  std::cout << "\t-g Set image for out playlist" << std::endl;
  std::cout << "\t-a Append entry" << std::endl;
  std::cout << "\t-e track:FIELD=value Set entry field" << std::endl;
  std::cout << "\t-r Remove entry (same as -e track:ta=)" << std::endl;
  std::cout << "\t-d Remove duplicate entries from out playlist" << std::endl;
  std::cout << "\t-u Remove unfound target entries from out playlist"
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
  std::cout << "\t-q Quiet (clobber out playlist)" << std::endl;
  std::cout << "\t-v Verbose" << std::endl;
  std::cout << "\t-h This help" << std::endl;
  std::cout << std::endl;
  std::cout
      << "FIELD can be one of: (ta)rget, (ar)tist, (ti)tle, (al)bum, "
         "(co)mment, (id)entifier, (im)age, (in)fo, album (tr)ack, (du)ration"
      << std::endl;
  std::cout << std::endl;
  std::cout << "LIST can be one of: all, dupe, net, unfound, or unique "
               "(multiple infiles)"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Exit codes:" << std::endl;
  std::cout << "0: Success or quiet flag" << std::endl;
  std::cout
      << "1: Out playlist has unfound target entries; or a dupe or unfound "
         "list contains at least one entry"
      << std::endl;
  std::cout << "2: IO, parse, or command line error" << std::endl;
}

int main(int argc, char **argv) {
  fs::path base, prepend;
  List list;
  std::vector<std::string> addItems, changeItems;

  auto parseList = [](const std::string &arg) {
    flags[14] = (arg == "all");
    flags[15] = (arg == "dupe");
    flags[16] = (arg == "net");
    flags[17] = (arg == "unfound");
    flags[18] = (arg == "unique");
  };

  const auto transformPath = [&](const fs::path &basePath, fs::path &path) {
    fs::path target = absPath(basePath, path);

    if (flags[0]) {
      path = target;
    } else if (flags[3]) {
      path = "file://" + encodeUri(target);
    } else if (flags[8]) {
      path = target.lexically_relative(list.playlist.parent_path());
    } else if (!base.empty()) {
      path = target.lexically_relative(base);
    }
  };

  int c;
#ifdef LIBCURL
#ifdef TAGLIB
  while (
      (c = getopt(argc, argv,
                  "a:A:B:dD:e:E:f:g:G:iIJ:K:l:L:mM:nN:o:OpP:r:Rst:T:uvxqh")) !=
      -1) {
#else
  while ((c = getopt(
              argc, argv,
              "a:A:B:dD:e:E:f:g:G:IJ:K:l:L:mM:nN:o:OpP:r:Rst:T:uvxqh")) != -1) {
#endif
#else
#ifdef TAGLIB
  while ((c = getopt(
              argc, argv,
              "a:A:B:dD:e:E:f:g:G:iJ:K:Il:L:mM:nN:o:OpP:r:Rt:T:uvxqh")) != -1) {
#else
  while ((c = getopt(argc, argv,
                     "a:A:B:dD:e:E:f:g:G:IJ:K:l:L:mM:nN:o:OpP:r:Rt:T:uvxqh")) !=
         -1) {
#endif
#endif
    switch (c) {
    case 'A':
      flags[20] = true;

      parseList(optarg);

      break;
    case 'a':
      addItems.emplace_back(optarg);

      break;
    case 'B':
      base = absPath(fs::current_path(), optarg);

      break;
    case 'd':
      flags[1] = true;

      break;
    case 'D':
      flags[2] = true;

      parseList(optarg);

      break;
    case 'e':
      changeItems.emplace_back(optarg);

      break;
    case 'E':
      flags[22] = true;

      parseList(optarg);

      break;
    case 'f':
      prepend = absPath(fs::current_path(), fs::path(optarg));

      break;
    case 'g':
      list.image = fs::path(optarg);

      break;
    case 'G':
      flags[23] = true;

      parseList(optarg);

      break;
#ifdef TAGLIB
    case 'i':
      flags[25] = true;

      break;
#endif
    case 'I':
      flags[3] = true;

      break;
    case 'J':
      flags[26] = true;

      parseList(optarg);

      break;
    case 'K':
      flags[27] = true;

      parseList(optarg);

      break;
    case 'l':
      flags[4] = true;

      parseList(optarg);

      break;
    case 'L':
      parseList(optarg);

      break;
    case 'm':
      flags[5] = true;

      break;
    case 'M':
      flags[21] = true;

      parseList(optarg);

      break;
    case 'n':
      flags[6] = true;

      break;
    case 'N':
      flags[24] = true;

      parseList(optarg);

      break;
    case 'o':
      list.playlist = absPath(fs::current_path(), optarg);

      break;
    case 'O':
      flags[0] = true;

      break;
    case 'p':
      flags[14] = true;
      flags[4] = true;

      break;
    case 'P':
      flags[7] = true;

      parseList(optarg);

      break;
    case 'r':
      changeItems.emplace_back(std::string(optarg) + ":ta=");

      break;
    case 'R':
      flags[8] = true;

      break;
#ifdef LIBCURL
    case 's':
      flags[19] = true;

      break;
#endif
    case 't':
      list.title = optarg;

      break;
    case 'T':
      flags[9] = true;

      parseList(optarg);

      break;
    case 'u':
      flags[10] = true;

      break;
    case 'x':
      flags[11] = true;

      break;
    case 'v':
      flags[12] = true;

      break;
    case 'q':
      flags[13] = true;

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

      if (entries.empty())
        std::cerr << "Playlist parse or other read error: " << inPl
                  << std::endl;

      if (flags[12])
        std::cout << "Parsed " << entries.size() << " entries"
                  << " from playlist file: " << inPl << std::endl;

      list.entries.insert(list.entries.end(), entries.begin(), entries.end());
    } else {
      std::cerr << "Skipping unfound file: " << inPl << std::endl;
    }
  }

  for (Entries::iterator it = list.entries.begin(); it != list.entries.end();
       it++) {
    auto computeTargets = [&](fs::path &target, bool &local, bool &valid) {
      target = processUri(target.string());

      if (!prepend.empty())
        target = absPath(prepend, target);

      local = localTarget(target.string());
      valid = validTarget(absPath(it->playlist.parent_path(), target));
    };

    computeTargets(it->target, it->localTarget, it->validTarget);

    if (it->localTarget) {
#ifdef TAGLIB
      if (it->validTarget && flags[25])
        fetchMetadata(*it);

#endif
      if (!list.relative)
        list.relative =
            (it->target.is_relative() || !it->target.has_parent_path());
    }

    it->duplicateTarget = find(*it, list.entries) < it;
  }

  if (list.playlist.empty()) {
    if (flags[11]) {
      std::cerr << "-x option requires an out playlist (-o)" << std::endl;

      return 2;
    }
  } else {
    if (fs::exists(list.playlist) && !flags[13] && !flags[11]) {
      std::cerr << "File exists: " << list.playlist << std::endl;

      return 2;
    }

    if ((!base.empty() && flags[0]) || (flags[0] && flags[8]) ||
        (!base.empty() && flags[3]) || (flags[3] && flags[8])) {
      std::cerr << "Cannot combine absolute and relative path transforms"
                << std::endl;

      return 2;
    }

    for (Entries::iterator it = list.entries.begin(); it != list.entries.end();
         it++) {
      it->track = std::distance(list.entries.begin(), it) + 1;
      if (!list.title.empty())
        it->playlistTitle = list.title;
      if (!list.image.empty())
        it->playlistImage = list.image;
    }

    for (const std::string &addItem : addItems) {
      Entry entry;
      entry.playlist = fs::current_path().append(".");
      entry.target = processUri(addItem);
      entry.track = list.entries.size() + 1;
      entry.localTarget = localTarget(entry.target.string());
      entry.validTarget = validTarget(entry.target);

      list.entries.push_back(entry);
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
          entry.target = processUri(value);
          entry.localTarget = localTarget(entry.target.string());
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
          entry.image = value;
        } else if (key == "in") {
          entry.info = value;
        } else if (key == "tr") {
          if (!std::all_of(value.begin(), value.end(), isdigit))
            invalid();
          entry.albumTrack = value.empty() ? 0 : std::stoi(value);
        } else if (key == "du") {
          if (!std::all_of(value.begin(), value.end(), isdigit))
            invalid();
          entry.duration = value.empty() ? 0 : std::stoi(value);
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

    if (flags[6])
      std::shuffle(list.entries.begin(), list.entries.end(),
                   std::default_random_engine());

    for (Entries::iterator it = list.entries.begin();
         it != list.entries.end();) {
      it->duplicateTarget = find(*it, list.entries) < it;

      if (it->target.empty() || (!it->validTarget && flags[10]) ||
          (it->duplicateTarget && flags[1])) {

        it = list.entries.erase(it);

        continue;
      }

      it->track = std::distance(list.entries.begin(), it) + 1;

      if (it->localTarget) {
        transformPath(it->playlist.parent_path(), it->target);

        it->validTarget = fs::exists(
            absPath(list.playlist.parent_path(), processUri(it->target)));
      }

      it->playlist = list.playlist;

      it++;
    }

    if (!base.empty() || flags[8])
      list.relative = (!base.empty() || flags[8]);

    if (flags[12])
      std::cout << "Generated playlist: " << list.entries.size() << " entries"
                << std::endl;
  }

  if (list.entries.empty()) {
    std::cerr << "Nothing to do!" << std::endl;
    std::cout << "playlist -h for usage information" << std::endl;

    return 2;
  }

  if (flags[14] || flags[15] || flags[16] || flags[17] || flags[18])
    ::list(list);

  for (const Entry &entry : list.entries) {
    list.dupeTargets += entry.duplicateTarget;
    list.netTargets += !entry.localTarget;
    list.unfoundTargets += !entry.validTarget;
  }

  if (!list.playlist.empty() && !flags[11]) {
    if ((list.unfoundTargets > 0) && !flags[13] && !flags[10])
      std::cout << "WARNING: out playlist has " << list.unfoundTargets
                << " unfound target(s)" << std::endl;

    if (playlist(list.playlist)->write(list)) {
      if (flags[12])
        std::cout << "Playlist successfully written: " << list.playlist
                  << std::endl;
    } else {
      std::cerr << "Write fail: " << list.playlist << std::endl;

      return 2;
    }
  } else {
    show(list);
  }

  return (!list.playlist.empty() && !flags[11]) && !flags[13]
             ? (list.unfoundTargets > 0)
             : 0;
}