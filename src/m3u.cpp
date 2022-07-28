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
#include <iomanip>
#include <regex>

void M3U::parse(Entries &entries) {
  std::ifstream file(m_playlist);
  std::string artist, image, line, title;
  bool invalidExtInfo(false);
  std::regex regex(
      "[^\\s\"]+(?:\"[^\"]*\"[^\\s\"]*)*|(?:\"[^\"]*\"[^\\s\"]*)+");

  int t = 1;
  while (!file.eof()) {
    Entry entry;

    while ((line.rfind("#EXTINF:", 0) == std::string::npos) &&
           (line.rfind("#", 0) != std::string::npos) && !file.eof()) {
      if (line.rfind("#EXTART:", 0) != std::string::npos)
        artist = split(line, ":").second;

      if (line.rfind("#EXTIMG:", 0) != std::string::npos)
        image = split(line, ":").second;

      if (line.rfind("#PLAYLIST:", 0) != std::string::npos)
        title = split(line, ":").second;

      std::getline(file, line);
    }

    if (line.rfind("#EXTINF:", 0) != std::string::npos) {
      std::size_t pos = line.find_first_of(",");
      std::string info;

      if (!invalidExtInfo) {
        invalidExtInfo = (pos > line.size());
      } else if (!flags[31]) {
        break;
      }

      entry.title = line.substr(pos + 1);

      info = line.substr(8, pos - 8);
      pos = info.find_first_of(' ');

      entry.duration = std::stoi(info.substr(0, pos)) * 1000;

      info = info.substr(pos + 1);
      std::sregex_iterator it(info.begin(), info.end(), regex), end;

      for (; it != end; ++it) {
        KeyValue pair = split(it->str());
        pair.second = unquote(pair.second);

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
      entry.playlistArtist = artist;
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
    cwar << "Playlist parse error(s): " << m_playlist << std::endl;

    if (invalidExtInfo)
      cwar << "Invalid extended information" << std::endl;

    if (file.bad() || !flags[31])
      entries.clear();
  }
}

const bool M3U::write(const List &list) {
  std::stringstream playList;
  std::ofstream file(m_playlist);

  for (const Entry &entry : list.entries) {
    std::stringstream extInf;

    if (!fs::is_fifo(m_playlist) && !flags[18]) {
      extInf << std::endl;
      extInf << "#EXTINF:";
      if (entry.duration > 0) {
        extInf << std::ceil(entry.duration / 1000.0);
      } else {
        extInf << "-1";
      }

      if (!entry.album.empty())
        extInf << " album=" << std::quoted(entry.album);
      if (!entry.artist.empty())
        extInf << " artist=" << std::quoted(entry.artist);
      if (!entry.comment.empty())
        extInf << " comment=" << std::quoted(entry.comment);
      if (!entry.identifier.empty())
        extInf << " identifier=" << std::quoted(entry.identifier);
      if (!entry.image.empty())
        extInf << " image=" << std::quoted(entry.image.string());
      if (!entry.info.empty())
        extInf << " info=" << std::quoted(entry.info);
      if (!entry.title.empty())
        extInf << " title=" << std::quoted(entry.title);
      if (entry.albumTrack)
        extInf << " track=\"" << entry.albumTrack << "\"";

      extInf << ",";

      if (!entry.artist.empty() || !entry.title.empty()) {
        extInf << entry.artist;

        if (!entry.artist.empty() && !entry.title.empty())
          extInf << " - ";

        extInf << entry.title;
      }

      playList << extInf.rdbuf() << std::endl;
    }

    playList << entry.target.string() << std::endl;
  }

  if (!fs::is_fifo(m_playlist) && !flags[18]) {
    file << "#EXTM3U" << std::endl;
    file << "#EXTENC:UTF-8" << std::endl;
    if (!list.title.empty())
      file << "#PLAYLIST:" << list.title << std::endl;
    if (!list.artist.empty())
      file << "#EXTART:" << list.artist << std::endl;
    if (!list.image.empty())
      file << "#EXTIMG:" << list.image.string() << std::endl;
  }

  file << playList.rdbuf();
  file.close();

  return !file.fail();
}
