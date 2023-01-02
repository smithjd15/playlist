/* playlist pls module
 * Copyright (C) 2021 - 2023 James D. Smith
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

#include "pls.h"

#include <cmath>

#define PLS_SECTION "[playlist]"
#define PLS_VERSION 2

void PLS::parse(Entries &entries) {
  std::ifstream file(m_playlist);
  std::string line;
  bool plsSection(false);
  int plsEntries(0), plsVersion(0);

  while (line.empty())
    std::getline(file, line);

  if (line == PLS_SECTION)
    plsSection = true;

  int t = 1;
  while (!file.eof() && (plsSection || flags[31])) {
    if (line.rfind("File" + std::to_string(t), 0) != std::string::npos) {
      Entry entry;

      entry.target = split(line).second;

      while ((line.rfind("File" + std::to_string(t + 1), 0) ==
              std::string::npos) &&
             (line.rfind("NumberOfEntries", 0) == std::string::npos) &&
             !file.eof()) {
        if (line.rfind("Title" + std::to_string(t), 0) != std::string::npos)
          entry.title = split(line).second;

        if (line.rfind("Length" + std::to_string(t), 0) != std::string::npos)
          entry.duration = std::stoi(split(line).second) * 1000;

        std::getline(file, line);
      }

      entry.track = t;
      entry.playlist = m_playlist;

      entries.push_back(entry);

      t++;

      continue;
    }

    if (line.rfind("NumberOfEntries", 0) != std::string::npos)
      plsEntries = std::stoi(split(line).second);

    if (line.rfind("Version", 0) != std::string::npos)
      plsVersion = std::stoi(split(line).second);

    std::getline(file, line);
  }

  file.close();

  if (file.bad() || !plsSection || (plsEntries != (int)entries.size()) ||
      (plsVersion != PLS_VERSION)) {
    cwar << "Playlist parse error(s): " << m_playlist << std::endl;

    if (!plsSection)
      cwar << "Section not found" << std::endl;

    if (plsEntries != (int)entries.size())
      cwar << "Entry count mismatch" << std::endl;

    if (plsVersion != PLS_VERSION)
      cwar << "Invalid or missing version" << std::endl;

    if (file.bad() || !flags[31])
      entries.clear();
  }
}

const bool PLS::write(const List &list) {
  std::ofstream file(m_playlist);

  file << PLS_SECTION << std::endl;
  file << std::endl;

  for (const Entry &entry : list.entries) {
    bool targetOnly =
        (entry.artist.empty() && entry.title.empty() && (entry.duration == 0));

    file << "File" << entry.track << "=" << entry.target.string() << std::endl;

    if (!targetOnly && !flags[18]) {
      if (!entry.artist.empty() || !entry.title.empty()) {
        file << "Title" << entry.track << "=" << entry.artist;

        if (!entry.artist.empty() && !entry.title.empty())
          file << " - ";

        file << entry.title << std::endl;
      }

      if (entry.duration > 0) {
        file << "Length" << entry.track << "="
             << std::ceil(entry.duration / 1000.0) << std::endl;
      } else if (!entry.localTarget) {
        file << "Length" << entry.track << "=-1" << std::endl;
      }

      if (!targetOnly && (entry.track != (int)list.entries.size()))
        file << std::endl;
    }
  }

  file << std::endl;
  file << "NumberOfEntries"
       << "=" << list.entries.size() << std::endl;
  file << "Version=" << PLS_VERSION << std::endl;
  file.close();

  return !file.fail();
}
