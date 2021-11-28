/* playlist m3u module
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

#include "m3u.h"

#include <cmath>
#include <regex>
#include <sstream>

void M3U::parse(Entries &entries) {
  std::ifstream file(m_playlist);
  std::string image, line, title;
  bool invalidExtInfo(false);
  std::regex regex(
      "[^\\s\"]+(?:\"[^\"]*\"[^\\s\"]*)*|(?:\"[^\"]*\"[^\\s\"]*)+");

  int t = 1;
  while (!file.eof()) {
    Entry entry;

    while ((line.rfind("#EXTINF:", 0) == std::string::npos) &&
           (line.rfind("#", 0) != std::string::npos) && !file.eof()) {
      if (line.rfind("#EXTIMG:", 0) != std::string::npos)
        image = split(line, ":").second;

      if (line.rfind("#PLAYLIST:", 0) != std::string::npos)
        title = split(line, ":").second;

      std::getline(file, line);
    }

    if (line.rfind("#EXTINF:", 0) != std::string::npos) {
      std::size_t pos = line.find_first_of(",");
      std::string info;
      float duration;

      if (!invalidExtInfo)
        invalidExtInfo = (pos > line.size());

      entry.title = line.substr(pos + 1);

      info = line.substr(8, pos - 8);
      pos = info.find_first_of(' ');

      duration = std::stoi(info.substr(0, pos));
      if (duration > 0)
        entry.duration = duration;

      info = info.substr(pos + 1);
      std::sregex_iterator it(info.begin(), info.end(), regex), end;

      for (; it != end; ++it) {
        KeyValue pair = split(it->str());
        std::istringstream strStream(pair.second);

        strStream >> std::quoted(pair.second);

        if (pair.first == "album")
          entry.album = pair.second;
        if (pair.first == "artist")
          entry.artist = pair.second;
        if (pair.first == "comment")
          entry.comment = pair.second;
        if (pair.first == "identifier")
          entry.identifier = pair.second;
        if (pair.first == "image")
          entry.image = pair.second;
        if (pair.first == "info")
          entry.info = pair.second;
        if (pair.first == "title")
          entry.title = pair.second;
        if (pair.first == "track")
          entry.albumTrack = std::stoi(pair.second);
      }

      std::getline(file, line);
      while (line.empty())
        std::getline(file, line);
    }

    if (!line.empty() && (line.rfind("#", 0) == std::string::npos)) {
      if (fs::is_fifo(m_playlist)) {
        entry.playlist = fs::current_path().append(".");
      } else {
        entry.playlist = m_playlist;
      }
      entry.playlistImage = image;
      entry.playlistTitle = title;
      entry.target = line;
      entry.track = t;

      entries.push_back(entry);

      t++;
    }

    std::getline(file, line);
  }

  file.close();

  if (file.bad() || invalidExtInfo) {
    if (invalidExtInfo)
      std::cerr << "Parse error: Invalid extended information" << std::endl;

    entries.clear();
  }
}

const bool M3U::write(const List &list) {
  std::stringstream playList;
  std::ofstream file(m_playlist);

  for (const Entry &entry : list.entries) {
    if (!flags[5]) {
      playList << "#EXTINF:";
      if (entry.duration > 0) {
        playList << std::round(entry.duration);
      } else {
        playList << "-1";
      }

      if (!entry.album.empty())
        playList << " album=" << std::quoted(entry.album);
      if (!entry.artist.empty())
        playList << " artist=" << std::quoted(entry.artist);
      if (!entry.comment.empty())
        playList << " comment=" << std::quoted(entry.comment);
      if (!entry.identifier.empty())
        playList << " identifier=" << std::quoted(entry.identifier);
      if (!entry.image.empty())
        playList << " image=" << std::quoted(entry.image.string());
      if (!entry.info.empty())
        playList << " info=" << std::quoted(entry.info);
      if (!entry.title.empty())
        playList << " title=" << std::quoted(entry.title);
      if (entry.albumTrack)
        playList << " track=\"" << entry.albumTrack << "\"";

      playList << ",";
    }

    if (!entry.artist.empty() || !entry.title.empty()) {
      playList << entry.artist;

      if (!entry.artist.empty() && !entry.title.empty())
        playList << " - ";

      playList << entry.title;
    }

    playList << std::endl;

    playList << entry.target.string() << std::endl;

    if (!flags[5] && (entry.track != (int)list.entries.size()))
      playList << std::endl;
  }

  if (!flags[5]) {
    file << "#EXTM3U" << std::endl;
    file << "#EXTENC:UTF-8" << std::endl;
    if (!list.title.empty())
      file << "#PLAYLIST:" << list.title << std::endl;
    if (!list.image.empty())
      file << "#EXTIMG:" << list.image.string() << std::endl;
    file << std::endl;
  }

  file << playList.rdbuf();
  file.close();

  return !file.fail();
}
