/* playlist common module
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

#pragma once

#include <bitset>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

struct Entry {
  fs::path image;
  fs::path playlist;
  fs::path playlistImage;
  fs::path target;
  std::string album;
  std::string artist;
  std::string comment;
  std::string identifier;
  std::string info;
  std::string playlistArtist;
  std::string playlistTitle;
  std::string title;
  int albumTrack = 0;
  int duration = 0;
  int track = 0;
  bool duplicateTarget = false;
  bool localImage = false;
  bool localTarget = false;
  bool validImage = false;
  bool validTarget = false;
};

typedef std::pair<const std::string, std::string> KeyValue;
typedef std::vector<Entry> Entries;
typedef std::bitset<33> Flags;

struct List {
  fs::path image;
  fs::path playlist;
  std::string artist;
  std::string title;
  Entries entries;
  int artists = 0;
  int dupeTargets = 0;
  int images = 0;
  int knownDuration = 0;
  int netImages = 0;
  int netTargets = 0;
  int titles = 0;
  int unfoundImages = 0;
  int unfoundTargets = 0;
  bool localImage = false;
  bool relative = false;
  bool validImage = false;
};

class Playlist {
public:
  Playlist(const fs::path &playlist) { m_playlist = playlist; };
  ~Playlist() { m_playlist.clear(); };

  virtual void parse(Entries &entries) = 0;
  virtual void writePreProcess(List &list) = 0;
  virtual const bool write(const List &list) = 0;

  fs::path m_playlist;
};

void show(const List &list);
void list(const List &list);
#ifdef TAGLIB
void fetchMetadata(Entry &entry);
#endif

const std::string processTarget(std::string target);
const std::string unquote(std::string quoted);
const std::string percentEncode(const std::string &uri);
const std::string percentDecode(std::string uri);
const fs::path absPath(const fs::path &p1, const fs::path &p2);
const KeyValue split(const std::string &line, std::string delim = "=");
const bool isUri(const std::string &target);
const bool validTarget(const fs::path &target);
const Entries::const_iterator find(const Entry &entry, const Entries &entries,
                                   bool sameList = true);
Playlist *playlist(const fs::path &playlist);

extern Flags flags;
extern std::stringstream cwar;
extern std::string ver;
