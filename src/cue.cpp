/* playlist cue sheet module
 * Copyright (C) 2022, 2023 James D. Smith
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

#include "cue.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <iomanip>
#include <sstream>

void CUE::parse(Entries &entries) {
  std::ifstream file(m_playlist);
  std::string comment, image, line, performer, rem, title;
  bool invalidTrack(false), singleFileCueSheet(false);

  while (!file.eof()) {
    while ((line.rfind("FILE", 0) == std::string::npos) && !file.eof()) {
      if (line.rfind("TITLE", 0) != std::string::npos)
        title = unquote(split(line, " ").second);

      if (line.rfind("PERFORMER", 0) != std::string::npos)
        performer = unquote(split(line, " ").second);

      if (line.rfind("REM", 0) != std::string::npos) {
        rem = split(line, " ").second;

        if (rem.rfind("COMMENT", 0) != std::string::npos)
          comment = unquote(split(rem, " ").second);
        if (rem.rfind("IMAGE", 0) != std::string::npos)
          image = unquote(split(rem, " ").second);
      }

      std::getline(file, line);
    }

    if (line.rfind("FILE", 0) != std::string::npos) {
      Entry entry;
      bool validTrack(false);

      entry.target = unquote(split(line, " ").second);

      std::getline(file, line);

      while ((line.rfind("FILE", 0) == std::string::npos) && !file.eof()) {
        while (line.rfind(" ", 0) != std::string::npos)
          line = line.substr(1);

        if (line.rfind("TRACK", 0) != std::string::npos) {
          std::string track = line.substr(6, 3);

          if (entry.track != 0)
            singleFileCueSheet = true;

          while (!track.empty() && std::isspace(track.back()))
            track.pop_back();

          entry.track = std::stoi(track);

          if (entry.track < 100)
            validTrack = true;
        }

        if (line.rfind("TITLE", 0) != std::string::npos)
          entry.title = unquote(split(line, " ").second);

        if (line.rfind("PERFORMER", 0) != std::string::npos)
          entry.artist = unquote(split(line, " ").second);

        if (line.rfind("REM", 0) != std::string::npos) {
          rem = split(line, " ").second;

          if (rem.rfind("ALBUM", 0) != std::string::npos)
            entry.album = unquote(split(rem, " ").second);
          if (rem.rfind("COMMENT", 0) != std::string::npos)
            entry.comment = unquote(split(rem, " ").second);
          if (rem.rfind("DURATION", 0) != std::string::npos)
            entry.duration = std::stoi(split(rem, " ").second);
          if (rem.rfind("IDENTIFIER", 0) != std::string::npos)
            entry.identifier = unquote(split(rem, " ").second);
          if (rem.rfind("IMAGE", 0) != std::string::npos)
            entry.image = unquote(split(rem, " ").second);
          if (rem.rfind("INFO", 0) != std::string::npos)
            entry.info = unquote(split(rem, " ").second);
          if (rem.rfind("TRACK", 0) != std::string::npos)
            entry.albumTrack = std::stoi(split(rem, " ").second);
        }

        std::getline(file, line);
      }

      invalidTrack = !validTrack;

      if ((invalidTrack || singleFileCueSheet) && !flags[31])
        break;

      entry.playlist = m_playlist;
      entry.playlistArtist = performer;
      entry.playlistComment = comment;
      entry.playlistImage = image;
      entry.playlistTitle = title;

      entries.push_back(entry);

      continue;
    }

    std::getline(file, line);
  }

  file.close();

  if (file.bad() || invalidTrack || singleFileCueSheet) {
    cwar << "Playlist parse error(s): " << m_playlist << std::endl;

    if (invalidTrack)
      cwar << "Invalid or missing track element" << std::endl;

    if (singleFileCueSheet)
      cwar << "Detected single file cue sheet" << std::endl;

    if (file.bad() || !flags[31])
      entries.clear();
  }
}

void CUE::writePreProcess(List &list) {
  for (Entries::iterator it = list.entries.begin(); it != list.entries.end();) {
    if (isUri(it->target.string())) {
      if (!flags[32])
        cwar << "Skipping URI: " << it->target << std::endl;

      it = list.entries.erase(it);

      continue;
    }

    it->track = std::distance(list.entries.begin(), it) + 1;

    it++;
  }
}

const bool CUE::write(const List &list) {
  std::ofstream file(m_playlist);

  if (!flags[18]) {
    if (!list.title.empty())
      file << "TITLE " << std::quoted(list.title) << std::endl;
    if (!list.artist.empty())
      file << "PERFORMER " << std::quoted(list.artist) << std::endl;
    if (!list.comment.empty())
      file << "REM COMMENT " << std::quoted(list.comment) << std::endl;
    if (!list.image.empty())
      file << "REM IMAGE " << std::quoted(list.image.c_str()) << std::endl;
  }

  for (const Entry &entry : list.entries) {
    std::string type;
    std::ostringstream track;

    if (entry.track == 100) {
      cwar << "WARNING: Can only write 99 tracks to a cue file" << std::endl;

      break;
    }

    std::string extension = entry.target.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](const char &c) { return std::tolower(c); });

    if (extension == ".aiff") {
      type = "AIFF";
    } else if (extension == ".mp3") {
      type = "MP3";
    } else {
      type = "WAVE";
    }

    track << std::setw(2) << std::setfill('0') << entry.track;

    file << "FILE " << std::quoted(entry.target.string()) << " " << type
         << std::endl;
    file << "  TRACK " << track.str() << " AUDIO" << std::endl;

    if (!flags[18]) {
      if (!entry.title.empty())
        file << "    TITLE " << std::quoted(entry.title) << std::endl;
      if (!entry.artist.empty())
        file << "    PERFORMER " << std::quoted(entry.artist) << std::endl;
      if (!entry.album.empty())
        file << "    REM ALBUM " << std::quoted(entry.album) << std::endl;
      if (!entry.comment.empty())
        file << "    REM COMMENT " << std::quoted(entry.comment) << std::endl;
      if (entry.duration > 0)
        file << "    REM DURATION " << entry.duration << std::endl;
      if (!entry.identifier.empty())
        file << "    REM IDENTIFIER " << std::quoted(entry.identifier) << std::endl;
      if (!entry.image.empty())
        file << "    REM IMAGE " << std::quoted(entry.image.c_str()) << std::endl;
      if (!entry.info.empty())
        file << "    REM INFO " << std::quoted(entry.info) << std::endl;
      if (entry.albumTrack)
        file << "    REM TRACK " << entry.albumTrack << std::endl;
    }

    file << "    INDEX 01 00:00:00" << std::endl;
  }

  file.close();

  return !file.fail();
}
