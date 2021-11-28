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
  std::string playlistTitle;
  std::string title;
  int albumTrack = 0;
  int track = 0;
  float duration = 0;
  bool duplicateTarget = false;
  bool localTarget = false;
  bool validTarget = false;
};

typedef std::pair<const std::string, std::string> KeyValue;
typedef std::vector<Entry> Entries;
typedef std::bitset<27> Flags;

struct List {
  fs::path image;
  fs::path playlist;
  std::string title;
  Entries entries;
  int dupeTargets = 0;
  int netTargets = 0;
  int unfoundTargets = 0;
  bool relative = false;
};

class Playlist {
public:
  Playlist(const fs::path &playlist) { m_playlist = playlist; };
  ~Playlist() { m_playlist.clear(); };

  virtual void parse(Entries &entries) = 0;
  virtual const bool write(const List &list) = 0;

  fs::path m_playlist;
};

void show(const List &list);
void list(const List &list);
#ifdef TAGLIB
void fetchMetadata(Entry &entry);
#endif

const std::string processUri(std::string uri);
const std::string encodeUri(const std::string &uri);
const fs::path absPath(const fs::path &p1, const fs::path &p2);
const KeyValue split(const std::string &line, std::string delim = "=");
const bool localTarget(const std::string &target);
const bool validTarget(const fs::path &target);
const Entries::const_iterator find(const Entry &entry, const Entries &entries,
                                   bool sameList = true);
Playlist *playlist(const fs::path &playlist);

extern Flags flags;
