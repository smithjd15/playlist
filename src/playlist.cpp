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

#include "playlist.h"

#include "cue.h"
#include "jspf.h"
#include "m3u.h"
#include "pls.h"
#include "xspf.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <ios>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <stdlib.h>

#ifdef LIBCURL
#include <curl/curl.h>
#endif

#ifdef TAGLIB
#include <taglib/fileref.h>
#include <taglib/taglib.h>
#include <taglib/tpropertymap.h>
#endif

namespace fs = std::filesystem;

Flags flags;
std::stringstream cwar;

void show(const List &list) {
  time_t totalDuration(0);
  uint size(0);
  tm *dur;
  std::string totalDur, totalArtists, totalImages, totalTitles;

  std::cout << "Track"
            << "\tStatus"
            << "\tDuration"
            << "\tTitle"
            << "\tTarget" << std::endl;

  for (const Entry &entry : list.entries) {
    std::string target = entry.localTarget ? entry.target.filename().string()
                                           : entry.target.string(),
                title = (!entry.artist.empty() && !entry.title.empty())
                            ? entry.artist + " - " + entry.title
                            : entry.title,
                status;
    int duration = (!entry.validTarget && flags[30])
                       ? 0
                       : std::round(entry.duration / 1000);

    if (!entry.image.empty()) {
      if (!entry.localImage)
        status += "n";

      if (!entry.validImage)
        status += "u";
    }

    if (entry.duplicateTarget)
      status += "D";

    if (!entry.localTarget)
      status += "N";

    if (!entry.validTarget)
      status += "U";

    if (status.empty())
      status = "*";

    if (entry.localTarget && entry.validTarget)
      size +=
          fs::file_size(absPath(entry.playlist.parent_path(), entry.target));

    if (entry.localImage && entry.validImage)
      size += fs::file_size(absPath(entry.playlist.parent_path(), entry.image));

    if (duration > 0)
      totalDuration += entry.duration;

    std::cout << entry.track << "\t" << status << "\t" << duration << "\t"
              << title << "\t" << target << std::endl;
  }

  if (list.localImage && list.validImage)
    size += fs::file_size(list.image);

  size = size / 1024 / 1024;

  totalDuration = totalDuration / 1000;
  dur = gmtime(&totalDuration);

  totalDur = "(";
  if (dur->tm_yday > 0)
    totalDur += std::to_string(dur->tm_yday) + " days, ";
  if (dur->tm_hour > 0)
    totalDur += std::to_string(dur->tm_hour) + " hours, ";
  if (dur->tm_min > 0)
    totalDur += std::to_string(dur->tm_min) + " minutes and ";
  totalDur += std::to_string(dur->tm_sec) + " seconds";
  totalDur += ")";

  if (list.artists > 1)
    totalArtists = " (of " + std::to_string(list.artists) + "!)";

  if (list.images > 1)
    totalImages = " (of " + std::to_string(list.images) + "!)";

  if (list.titles > 1)
    totalTitles = " (of " + std::to_string(list.titles) + "!)";

  std::cout << "[n]etwork images: " << list.netImages
            << "\t[u]nfound images: " << list.unfoundImages << std::endl;
  std::cout << "[D]upe: " << list.dupeTargets
            << "\t[N]etwork: " << list.netTargets
            << "\t[U]nfound: " << list.unfoundTargets << std::endl;
  std::cout << "Entries: " << list.entries.size() << std::endl;
  std::cout << std::endl;
  std::cout << "Total known duration: " << std::round(totalDuration) << " seconds "
            << totalDur << std::endl;
  std::cout << "Total known disk used: " << size << " MB" << std::endl;
  std::cout << "Known title: " << list.title << totalTitles << std::endl;
  std::cout << "Known artist: " << list.artist << totalArtists << std::endl;
  std::cout << "Known image: " << list.image.string() << totalImages
            << std::endl;

  if (cwar.rdbuf()->in_avail() > 0)
    std::cout << std::endl;
}

void list(const List &list) {
  std::vector<KeyValue> all, dupe, image, network, netImg, unfound, unfoundImg,
      unique;

  const auto targetItem = [](const Entry &entry) {
    if (flags[0])
      return KeyValue(entry.artist, entry.target.string());

    if (flags[10])
      return KeyValue(entry.identifier, entry.target.string());

    if (flags[11])
      return KeyValue(entry.comment, entry.target.string());

    if (flags[12])
      return KeyValue(entry.image.string(), entry.target.string());

    if (flags[15])
      return KeyValue(entry.playlistTitle, entry.target.string());

    if (flags[16])
      return KeyValue(entry.playlistImage.string(), entry.target.string());

    if (flags[19])
      return KeyValue(entry.album, entry.target.string());

    if (flags[21])
      return KeyValue(entry.info, entry.target.string());

    if (flags[24])
      return KeyValue(entry.playlist.string(), entry.target.string());

    if (flags[27])
      return KeyValue(entry.playlistArtist, entry.target.string());

    if (flags[28])
      return KeyValue(entry.title, entry.target.string());

    return KeyValue(std::to_string(entry.track), entry.target.string());
  };

  const auto listTargetItems = [](const std::vector<KeyValue> &items) {
    for (const KeyValue &item : items) {
      if (flags[17]) {
        std::cout << item.second << std::endl;
      } else {
        std::cout << item.first << "\t" << item.second << std::endl;
      }
    }
  };

  for (Entries::const_iterator it = list.entries.begin();
       it != list.entries.end(); it++) {
    if (it->duplicateTarget)
      dupe.push_back(targetItem(*it));

    if (!it->localTarget)
      network.push_back(targetItem(*it));

    if (!it->validTarget)
      unfound.push_back(targetItem(*it));

    if (find(*it, list.entries, false) == list.entries.end())
      unique.push_back(targetItem(*it));

    if (!it->image.empty()) {
      if (!it->localImage)
        netImg.push_back(targetItem(*it));

      if (!it->validImage)
        unfoundImg.push_back(targetItem(*it));

      image.push_back(targetItem(*it));
    }

    all.push_back(targetItem(*it));
  }

  if (flags[1]) {
    listTargetItems(all);

    std::exit(0);
  } else if (flags[2]) {
    listTargetItems(dupe);

    std::exit(!flags[33] ? !dupe.empty() : 0);
  } else if (flags[3]) {
    listTargetItems(image);

    std::exit(0);
  } else if (flags[4]) {
    listTargetItems(network);

    std::exit(0);
  } else if (flags[5]) {
    listTargetItems(netImg);

    std::exit(0);
  } else if (flags[6]) {
    listTargetItems(unfound);

    std::exit(!flags[33] ? !unfound.empty() : 0);
  } else if (flags[7]) {
    listTargetItems(unfoundImg);

    std::exit(!flags[33] ? !unfoundImg.empty() : 0);
  } else if (flags[8]) {
    listTargetItems(unique);

    std::exit(0);
  }
}
#ifdef TAGLIB

void fetchMetadata(Entry &entry) {
  TagLib::FileRef file = TagLib::FileRef(
      absPath(entry.playlist.parent_path(), entry.target).c_str());

  if (!file.isNull() && !file.tag()->isEmpty()) {
    entry.album = file.tag()->album().toCString();
    entry.albumTrack = file.tag()->track();
    entry.artist = file.tag()->artist().toCString();
    entry.comment = file.tag()->comment().toCString();
    entry.duration = file.audioProperties()->lengthInMilliseconds();
    entry.title = file.tag()->title().toCString();
  } else {
    cwar << "Could not read target tag: " << entry.target << std::endl;
  }
}
#endif

const std::string processUri(std::string uri) {
  int len = uri.size() - 2;

  for (int i = 0; i < len; i++) {
    std::smatch match;
    std::string buf = uri.substr(i, 3);

    if (std::regex_match(buf, match, std::regex("%[0-9A-F]{2}"))) {
      std::string c;

      buf = buf.replace(0, 1, "0x");
      c = (char)std::stoi(buf, nullptr, 16);
      uri = uri.replace(uri.begin() + i, uri.begin() + i + 3, c);
    }

    len = uri.size() - 2;
  }

  if (uri.rfind("~/", 0) != std::string::npos)
    uri = getenv("HOME") + uri.substr(1);

  if (uri.rfind("file:///", 0) != std::string::npos) {
    uri = uri.substr(7);
  } else if (uri.rfind("file:/", 0) != std::string::npos) {
    uri = uri.substr(5);
  }

  if (uri.rfind(fs::path::preferred_separator + std::string(".."), 0) !=
      std::string::npos)
    uri = uri.substr(1);

  return uri;
}

const std::string unquote(std::string quoted) {
  std::istringstream unquote(quoted);
  unquote >> std::quoted(quoted);

  return quoted;
}

const std::string encodeUri(const std::string &uri) {
  std::ostringstream uriOut;
  std::regex r("[!:\\/\\-._~0-9A-Za-z]");

  for (const char &c : uri) {
    if (std::regex_match(std::string({c}), r)) {
      uriOut << c;
    } else {
      uriOut << "%" << std::uppercase << std::hex << (0xff & c);
    }
  }

  return uriOut.str();
}

const fs::path absPath(const fs::path &p1, const fs::path &p2) {
  fs::path path;

  if (localTarget(p2.string())) {
    if (p2.has_root_directory()) {
      path = p2;
    } else {
      path = p1;
      path /= p2;
    }

    path = path.lexically_normal();
  } else {
    path = p2;
  }

  return path;
}

const KeyValue split(const std::string &line, std::string delim) {
  std::size_t pos = line.find_first_of(delim);

  if (!pos)
    return KeyValue();

  return KeyValue(line.substr(0, pos), line.substr(pos + 1));
}

const bool localTarget(const std::string &target) {
  return (target.find("://") == std::string::npos);
}

const bool validTarget(const fs::path &target) {
  const bool local = localTarget(target.string());
  bool valid = (!local || fs::exists(target));

#ifdef LIBCURL
  if (!local && flags[26]) {
    CURL *curl = curl_easy_init();
    CURLcode result;

    curl_easy_setopt(curl, CURLOPT_URL, target.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, true);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

    result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    valid = (result == CURLE_OK);
  }
#endif
  return valid;
}

const Entries::const_iterator find(const Entry &entry, const Entries &entries,
                                   bool sameList) {
  return std::find_if(entries.begin(), entries.end(), [&](const Entry &e) {
    if (!sameList && (entry.playlist == e.playlist))
      return false;

    if (fs::weakly_canonical(entry.target.string()) ==
        fs::weakly_canonical(e.target.string()))
      return true;

    if (!entry.artist.empty() && !entry.title.empty())
      if ((entry.artist == e.artist) && (entry.title == e.title))
        return true;

    return false;
  });
};

Playlist *playlist(const fs::path &playlist) {
  std::string extension = playlist.extension().string();

  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](const char &c) { return std::tolower(c); });

  if (extension == ".cue") {
    return new CUE(playlist);
  } else if (extension == ".jspf") {
    return new JSPF(playlist);
  } else if (extension == ".m3u") {
    return new M3U(playlist);
  } else if (extension == ".pls") {
    return new PLS(playlist);
  } else if (extension == ".xspf") {
    return new XSPF(playlist);
  } else if (fs::is_fifo(playlist)) {
    return new M3U(playlist);
  }

  std::cerr << "Unsupported file format: " << playlist.extension().string()
            << std::endl;

  exit(2);
}
