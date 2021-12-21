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

#include <bitset>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <vector>

#include "pugixml.hpp"

#include "unistd.h"
#ifdef LIBCURL
#include "curl/curl.h"
#endif
#ifdef TAGLIB
#include "taglib/fileref.h"
#endif

#define VER 1.43

#define PLS_SECTION "[playlist]"
#define PLS_VERSION 2
#define XSPF_ROOT "playlist"

namespace fs = std::filesystem;

struct Entry {
  fs::path image;
  fs::path playlist;
  std::string playlistTitle;
  fs::path playlistImage;
  fs::path target;
  std::string album;
  std::string artist;
  std::string comment;
  std::string identifier;
  std::string info;
  std::string title;
  int albumTrack = 0;
  float duration = 0;
  int track = 0;
  bool duplicateTarget = false;
  bool localTarget = false;
  bool validTarget = false;
};

typedef std::pair<const std::string, std::string> KeyValue;
typedef std::vector<KeyValue> TargetItems;
typedef std::vector<Entry> Entries;

std::bitset<27> Flags;

const std::string ProcessUri(std::string uri) {
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

  if (uri.rfind("~/", 0) != std::string::npos) {
    uri = getenv("HOME") + uri.substr(1);
  }

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

const Entries ParseXSPF(const fs::path &playlist) {
  pugi::xml_document playlistDoc;
  std::ifstream file(playlist);
  pugi::xml_parse_result result(playlistDoc.load(file));
  pugi::xml_node trackList, track;
  std::string image, title;
  Entries entries;

  file.close();

  if (result && !file.bad() && playlistDoc.child(XSPF_ROOT)) {
    trackList = playlistDoc.child("playlist").child("trackList");
    image = playlistDoc.child("playlist").child("image").text().as_string();
    title = playlistDoc.child("playlist").child("title").text().as_string();

    for (track = trackList.child("track"); track;
         track = track.next_sibling("track")) {
      Entry entry;

      entry.duration = track.child("duration").text().as_int() / 1000.0;
      entry.playlist = playlist;
      entry.playlistImage = image;
      entry.playlistTitle = title;
      entry.target = ProcessUri(track.child("location").text().as_string());
      entry.title = track.child("title").text().as_string();
      entry.artist = track.child("creator").text().as_string();
      entry.album = track.child("album").text().as_string();
      entry.comment = track.child("annotation").text().as_string();
      entry.identifier = track.child("identifier").text().as_string();
      entry.image = track.child("image").text().as_string();
      entry.info = track.child("info").text().as_string();
      entry.albumTrack = track.child("trackNum").text().as_int();
      entry.track =
          std::distance<pugi::xml_node::iterator>(trackList.begin(), track) + 1;

      entries.push_back(entry);
    }

    return entries;
  } else {
    if (!result)
      std::cerr << "Parse error: " << result.description() << std::endl;

    if (!playlistDoc.child(XSPF_ROOT))
      std::cerr << "Parse error: Unrecognized root node" << std::endl;

    return entries;
  }
}

const KeyValue Split(const std::string &line, std::string delim = "=") {
  std::size_t pos = line.find_first_of(delim);

  if (!pos)
    return KeyValue();

  return KeyValue(line.substr(0, pos), line.substr(pos + 1));
}

const Entries ParsePLS(const fs::path &playlist) {
  std::ifstream file(playlist);
  std::string line;
  bool plsSection(false);
  Entries entries;
  int plsEntries(0), plsVersion(0);

  std::getline(file, line);
  while (line.empty())
    std::getline(file, line);

  if (line == PLS_SECTION)
    plsSection = true;

  int t = 1;
  while (!file.eof()) {
    Entry entry;

    if (line.rfind("File" + std::to_string(t), 0) != std::string::npos) {
      entry.target = ProcessUri(Split(line).second);

      while ((line.rfind("File" + std::to_string(t + 1), 0) ==
              std::string::npos) &&
             (line.rfind("NumberOfEntries", 0) == std::string::npos) &&
             !file.eof()) {
        if (line.rfind("Title" + std::to_string(t), 0) != std::string::npos)
          entry.title = Split(line).second;

        if (line.rfind("Length" + std::to_string(t), 0) != std::string::npos)
          entry.duration = std::stoi(Split(line).second);

        std::getline(file, line);
      }

      entry.track = t;
      entry.playlist = playlist;

      entries.push_back(entry);

      t++;

      continue;
    }

    if (line.rfind("NumberOfEntries", 0) != std::string::npos)
      plsEntries = std::stoi(Split(line).second);

    if (line.rfind("Version", 0) != std::string::npos)
      plsVersion = std::stoi(Split(line).second);

    std::getline(file, line);
  }

  file.close();

  if (file.bad() || !plsSection || (plsEntries != (int)entries.size()) ||
      (plsVersion != PLS_VERSION)) {
    if (!plsSection)
      std::cerr << "Parse error: Section not found" << std::endl;

    if (plsEntries != (int)entries.size())
      std::cerr << "Parse error: Entry count mismatch" << std::endl;

    if (plsVersion != PLS_VERSION)
      std::cerr << "Parse error: Invalid or missing version" << std::endl;

    entries.clear();
  }

  return entries;
}

const Entries ParseM3U(const fs::path &playlist) {
  std::ifstream file(playlist);
  std::string image, line, title;
  bool invalidExtInfo(false);
  std::regex regex(
      "[^\\s\"]+(?:\"[^\"]*\"[^\\s\"]*)*|(?:\"[^\"]*\"[^\\s\"]*)+");
  Entries entries;

  int t = 1;
  while (!file.eof()) {
    Entry entry;

    while ((line.rfind("#EXTINF:", 0) == std::string::npos) &&
           (line.rfind("#", 0) != std::string::npos) && !file.eof()) {
      if (line.rfind("#EXTIMG:", 0) != std::string::npos)
        image = Split(line, ":").second;

      if (line.rfind("#PLAYLIST:", 0) != std::string::npos)
        title = Split(line, ":").second;

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
        KeyValue pair = Split(it->str());
        std::istringstream strStream(pair.second);
        strStream >> std::quoted(pair.second);

        if (pair.first == "artist")
          entry.artist = pair.second;
        if (pair.first == "title")
          entry.title = pair.second;
        if (pair.first == "album")
          entry.album = pair.second;
        if (pair.first == "comment")
          entry.comment = pair.second;
        if (pair.first == "identifier")
          entry.identifier = pair.second;
        if (pair.first == "info")
          entry.info = pair.second;
        if (pair.first == "image")
          entry.image = pair.second;
        if (pair.first == "track")
          entry.albumTrack = std::stoi(pair.second);
      }

      std::getline(file, line);
      while (line.empty())
        std::getline(file, line);
    }

    if (!line.empty() && (line.rfind("#", 0) == std::string::npos)) {
      if (fs::is_fifo(playlist)) {
        entry.playlist = fs::current_path().append(".");
      } else {
        entry.playlist = playlist;
      }
      entry.playlistImage = image;
      entry.playlistTitle = title;
      entry.target = ProcessUri(line);
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

  return entries;
}

void ParsePlaylist(const fs::path &playlist, Entries &entries) {
  std::string extension = playlist.extension().string();

  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](const char &c) { return std::tolower(c); });

  if (extension == ".m3u" || fs::is_fifo(playlist)) {
    entries = ParseM3U(playlist);
  } else if (extension == ".pls") {
    entries = ParsePLS(playlist);
  } else if (extension == ".xspf") {
    entries = ParseXSPF(playlist);
  }

  if (entries.empty())
    std::cerr << "Playlist parse or other read error: " << playlist
              << std::endl;

  if (Flags[12])
    std::cout << "Parsed " << entries.size() << " entries"
              << " from playlist file: " << playlist << std::endl;
}

void WriteXSPF(std::ofstream &file, const Entries &entries) {
  pugi::xml_node doc, playlist, track, trackList;
  pugi::xml_document pl;
  std::string image, title;

  doc = pl.prepend_child(pugi::node_declaration);
  doc.append_attribute("version") = "1.0";
  doc.append_attribute("encoding") = "UTF-8";

  playlist = pl.append_child("playlist");
  playlist.append_attribute("version").set_value(1);
  playlist.append_attribute("xmlns").set_value("http://xspf.org/ns/0/");

  trackList = playlist.append_child("trackList");

  for (const Entry &entry : entries) {
    image = entry.playlistImage;
    title = entry.playlistTitle;

    track = trackList.append_child("track");
    track.append_child("location").text().set(entry.target.c_str());

    if (!Flags[5]) {
      if (!entry.title.empty())
        track.append_child("title").text().set(entry.title.c_str());
      if (!entry.artist.empty())
        track.append_child("creator").text().set(entry.artist.c_str());
      if (!entry.album.empty())
        track.append_child("album").text().set(entry.album.c_str());
      if (!entry.comment.empty())
        track.append_child("annotation").text().set(entry.comment.c_str());
      if (!entry.identifier.empty())
        track.append_child("identifier").text().set(entry.identifier.c_str());
      if (!entry.info.empty())
        track.append_child("info").text().set(entry.info.c_str());
      if (!entry.image.empty())
        track.append_child("image").text().set(entry.image.c_str());
      if (entry.albumTrack)
        track.append_child("trackNum").text().set(entry.albumTrack);
      if (entry.duration > 0)
        track.append_child("duration")
            .text()
            .set(std::to_string((int)(entry.duration * 1000)).c_str());
    }
  }

  if (!title.empty() && !Flags[5])
    playlist.insert_child_before("title", trackList).text().set(title.c_str());
  if (!image.empty() && !Flags[5])
    playlist.insert_child_before("image", trackList).text().set(image.c_str());

  pl.save(file, "  ");
}

void WritePLS(std::ofstream &file, const Entries &entries) {
  file << "[playlist]" << std::endl;
  file << std::endl;

  for (const Entry &entry : entries) {
    file << "File" << entry.track << "=" << entry.target.string() << std::endl;

    if (!Flags[5]) {
      if (!entry.artist.empty() || !entry.title.empty() || (entry.duration > 0)) {
        if (!entry.artist.empty() || !entry.title.empty()) {
          file << "Title" << entry.track << "=" << entry.artist;

          if (!entry.artist.empty() && !entry.title.empty())
            file << " - ";

          file << entry.title << std::endl;
        }

        if (entry.duration > 0) {
          file << "Length" << entry.track << "=" << std::round(entry.duration)
               << std::endl;
        } else if (!entry.localTarget) {
          file << "Length=-1" << std::endl;
        }
      }

      if (entry.track != (int)entries.size())
        file << std::endl;
    }
  }

  file << std::endl;
  file << "NumberOfEntries"
       << "=" << entries.size() << std::endl;
  file << "Version=2" << std::endl;
}

void WriteM3U(std::ofstream &file, const Entries &entries) {
  std::stringstream playlist;
  std::string image, title;

  for (const Entry &entry : entries) {
    image = entry.playlistImage;
    title = entry.playlistTitle;

    if (!Flags[5]) {
      playlist << "#EXTINF:";
      if (entry.duration > 0) {
        playlist << std::round(entry.duration);
      } else {
        playlist << "-1";
      }

      if (!entry.artist.empty())
        playlist << " artist=" << std::quoted(entry.artist);
      if (!entry.title.empty())
        playlist << " title=" << std::quoted(entry.title);
      if (!entry.album.empty())
        playlist << " album=" << std::quoted(entry.album);
      if (!entry.comment.empty())
        playlist << " comment=" << std::quoted(entry.comment);
      if (!entry.identifier.empty())
        playlist << " identifier=" << std::quoted(entry.identifier);
      if (!entry.info.empty())
        playlist << " info=" << std::quoted(entry.info);
      if (!entry.image.empty())
        playlist << " image=" << std::quoted(entry.image.string());
      if (entry.albumTrack)
        playlist << " track=\"" << entry.albumTrack << "\"";

      playlist << ",";
    }

    if (!entry.artist.empty() || !entry.title.empty()) {
      playlist << entry.artist;

      if (!entry.artist.empty() && !entry.title.empty())
        playlist << " - ";

      playlist << entry.title;
    }

    playlist << std::endl;

    playlist << entry.target.string() << std::endl;

    if (!Flags[5] && (entry.track != (int)entries.size()))
      playlist << std::endl;
  }

  if (!Flags[5]) {
    file << "#EXTM3U" << std::endl;
    file << "#EXTENC:UTF-8" << std::endl;
    if (!title.empty())
      file << "#PLAYLIST:" << title << std::endl;
    if (!image.empty())
      file << "#EXTIMG:" << image << std::endl;
    file << std::endl;
  }

  file << playlist.rdbuf();
}

void WritePlaylist(fs::path &playlist, const Entries &entries) {
  std::string extension = playlist.extension().string();
  std::ofstream file;

  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](const char &c) { return std::tolower(c); });

  if (extension == ".m3u") {
    file.open(playlist);
    WriteM3U(file, entries);
  } else if (extension == ".pls") {
    file.open(playlist);
    WritePLS(file, entries);
  } else if (extension == ".xspf") {
    file.open(playlist);
    WriteXSPF(file, entries);
  } else {
    std::cerr << "Unsupported output file format: " << extension << std::endl;

    std::exit(2);
  }

  file.close();

  if (file.fail()) {
    std::cerr << "Write fail: " << playlist << std::endl;

    std::exit(2);
  }

  if (Flags[12])
    std::cout << "Playlist successfully written: " << playlist << std::endl;
}

void ShowPlaylist(const Entries &entries) {
  int totalDuration(0), dupe(0), network(0), unfound(0);
  time_t duration;
  tm *dur;
  std::string playlistImage, playlistTitle, totalDur;

  std::cout << "Track"
            << "\tStatus"
            << "\tDuration"
            << "\tTitle"
            << "\tTarget" << std::endl;

  for (const Entry &entry : entries) {
    std::string target = entry.localTarget ? entry.target.filename().string()
                                           : entry.target.string(),
                title = (!entry.artist.empty() && !entry.title.empty())
                                           ? entry.artist + " - " + entry.title
                                           : entry.title,
                status;
    float duration = (!entry.validTarget && Flags[11]) ? 0 : entry.duration;

    if (entry.duplicateTarget) {
      status += "D";
      dupe++;
    }

    if (!entry.localTarget) {
      status += "N";
      network++;
    }

    if (!entry.validTarget) {
      status += "U";
      unfound++;
    }

    playlistImage = entry.playlistImage;
    playlistTitle = entry.playlistTitle;

    if (!entry.duplicateTarget && entry.localTarget && entry.validTarget)
      status = "*";

    if (duration > 0)
      totalDuration += entry.duration;

    std::cout << entry.track << "\t" << status << "\t" << duration << "\t"
              << title << "\t" << target << std::endl;
  }

  duration = totalDuration;
  dur = gmtime(&duration);

  totalDur = "(";
  if (dur->tm_yday > 0)
    totalDur += std::to_string(dur->tm_yday) + " days, ";
  if (dur->tm_hour > 0)
    totalDur += std::to_string(dur->tm_hour) + " hours, ";
  if (dur->tm_min > 0)
    totalDur += std::to_string(dur->tm_min) + " minutes and ";
  totalDur += std::to_string(dur->tm_sec) + " seconds";
  totalDur += ")";

  std::cout << std::endl;
  std::cout << "[D]upe: " << dupe << "\t[N]etwork: " << network
            << "\t[U]nfound: " << unfound << "\tTotal: " << entries.size()
            << std::endl;
  std::cout << std::endl;
  std::cout << "Known title: " << playlistTitle << std::endl;
  std::cout << "Known image: " << playlistImage << std::endl;
  std::cout << "Total known duration: " << totalDuration << " seconds "
            << totalDur << std::endl;
}

void ShowHelp() {
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
  std::cout << "\t-o Out playlist file (.m3u, .pls, .xspf)" << std::endl;
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
  std::cout << "\t-u Remove unfound target entries from out playlist" << std::endl;
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
  std::cout << "FIELD can be one of: (ta)rget, (ar)tist, (ti)tle, (al)bum, "
               "(co)mment, (id)entifier, (im)age, (in)fo, album (tr)ack, (du)ration"
            << std::endl;
  std::cout << std::endl;
  std::cout << "LIST can be one of: all, dupe, net, unfound, or unique "
               "(multiple infiles)"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Exit codes:" << std::endl;
  std::cout << "0: Success or quiet flag" << std::endl;
  std::cout << "1: Out playlist has unfound target entries; or a dupe or unfound "
               "list contains at least one entry"
            << std::endl;
  std::cout << "2: IO, parse, or command line error" << std::endl;
}

int main(int argc, char **argv) {
  fs::path base, image, outPl, prepend;
  std::string title;
  Entries entries;
  std::vector<std::string> addItems, changeItems;
  TargetItems all, dupe, network, unfound, unique;

  const auto localTarget = [](const std::string &target) {
    return (target.find("://") == std::string::npos);
  };

  const auto validTarget = [&](const fs::path &target) {
    const bool local = localTarget(target.string());
    bool valid = (!local || fs::exists(target));
#ifdef LIBCURL

    if (!local && Flags[19]) {
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
  };

  const auto getPath = [&](const fs::path &p1, const fs::path &p2) {
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
  };

  auto parseList = [](const std::string &arg) {
    Flags[14] = (arg == "all");
    Flags[15] = (arg == "dupe");
    Flags[16] = (arg == "net");
    Flags[17] = (arg == "unfound");
    Flags[18] = (arg == "unique");
  };

#ifdef TAGLIB
  auto fetchMetadata = [&](Entry &entry) {
    TagLib::FileRef file = TagLib::FileRef(
        getPath(entry.playlist.parent_path(), entry.target).c_str());

    if (!file.isNull() && !file.tag()->isEmpty()) {
      entry.album = file.tag()->album().toCString();
      entry.albumTrack = file.tag()->track();
      entry.artist = file.tag()->artist().toCString();
      entry.comment = file.tag()->comment().toCString();
      entry.duration = file.audioProperties()->lengthInSeconds();
      entry.title = file.tag()->title().toCString();
    } else if (!Flags[13]) {
      std::cerr << "Could not read target tag: " << entry.target << std::endl;
    }
  };

#endif
  const auto targetItem = [](const Entry &entry) {
    if (Flags[7])
      return KeyValue(entry.playlist.string(), entry.target.string());

    if (Flags[26])
      return KeyValue(entry.playlistTitle, entry.target.string());

    if (Flags[27])
      return KeyValue(entry.playlistImage.string(), entry.target.string());

    if (Flags[20])
      return KeyValue(entry.artist, entry.target.string());

    if (Flags[9])
      return KeyValue(entry.title, entry.target.string());

    if (Flags[21])
      return KeyValue(entry.album, entry.target.string());

    if (Flags[22])
      return KeyValue(entry.comment, entry.target.string());

    if (Flags[2])
      return KeyValue(entry.identifier, entry.target.string());

    if (Flags[23])
      return KeyValue(entry.image, entry.target.string());

    if (Flags[24])
      return KeyValue(entry.info, entry.target.string());

    return KeyValue(std::to_string(entry.track), entry.target.string());
  };

  auto find = [](const Entry &entry, const Entries &entries,
                 bool sameList = true) {
    return std::find_if(
        entries.begin(), entries.end(), [&entry, &sameList](const Entry &e) {
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

  auto groupTargetItems = [&](const Entries &entries) {
    for (Entries::const_iterator it = entries.begin(); it != entries.end();
         it++) {
      if (it->duplicateTarget)
        dupe.push_back(targetItem(*it));

      if (!it->localTarget)
        network.push_back(targetItem(*it));

      if (!it->validTarget)
        unfound.push_back(targetItem(*it));

      if (find(*it, entries, false) == entries.end())
        unique.push_back(targetItem(*it));

      all.push_back(targetItem(*it));
    }
  };

  auto listTargetItems = [&](const TargetItems &items) {
    for (const KeyValue &item : items)
      if (Flags[4]) {
        std::cout << item.second << std::endl;
      } else {
        std::cout << item.first << "\t" << item.second << std::endl;
      }
  };

  int c;
#ifdef LIBCURL
#ifdef TAGLIB
  while ((c = getopt(argc, argv,
                     "a:A:B:dD:e:E:f:g:G:iIJ:K:l:L:mM:nN:o:OpP:r:Rst:T:uvxqh")) != -1) {
#else
  while ((c = getopt(argc, argv,
                     "a:A:B:dD:e:E:f:g:G:IJ:K:l:L:mM:nN:o:OpP:r:Rst:T:uvxqh")) != -1) {
#endif
#else
#ifdef TAGLIB
  while ((c = getopt(argc, argv,
                     "a:A:B:dD:e:E:f:g:G:iJ:K:Il:L:mM:nN:o:OpP:r:Rt:T:uvxqh")) != -1) {
#else
  while ((c = getopt(argc, argv, "a:A:B:dD:e:E:f:g:G:IJ:K:l:L:mM:nN:o:OpP:r:Rt:T:uvxqh")) !=
         -1) {
#endif
#endif
    switch (c) {
    case 'A':
      Flags[20] = true;

      parseList(optarg);

      break;
    case 'a':
      addItems.emplace_back(optarg);

      break;
    case 'B':
      base = getPath(fs::current_path(), optarg);

      break;
    case 'd':
      Flags[1] = true;

      break;
    case 'D':
      Flags[2] = true;

      parseList(optarg);

      break;
    case 'e':
      changeItems.emplace_back(optarg);

      break;
    case 'E':
      Flags[22] = true;

      parseList(optarg);

      break;
    case 'f':
      prepend = getPath(fs::current_path(), optarg);

      break;
    case 'g':
      image = fs::path(optarg);

      break;
    case 'G':
      Flags[23] = true;

      parseList(optarg);

      break;
#ifdef TAGLIB
    case 'i':
      Flags[25] = true;

      break;
#endif
    case 'I':
      Flags[3] = true;

      break;
    case 'J':
      Flags[26] = true;

      parseList(optarg);

      break;
    case 'K':
      Flags[27] = true;

      parseList(optarg);

      break;
    case 'l':
      Flags[4] = true;

      parseList(optarg);

      break;
    case 'L':
      parseList(optarg);

      break;
    case 'm':
      Flags[5] = true;

      break;
    case 'M':
      Flags[21] = true;

      parseList(optarg);

      break;
    case 'n':
      Flags[6] = true;

      break;
    case 'N':
      Flags[24] = true;

      parseList(optarg);

      break;
    case 'o':
      outPl = getPath(fs::current_path(), optarg);

      break;
    case 'O':
      Flags[0] = true;

      break;
    case 'p':
      Flags[14] = true;
      Flags[4] = true;

      break;
    case 'P':
      Flags[7] = true;

      parseList(optarg);

      break;
    case 'r':
      changeItems.emplace_back(std::string(optarg) + ":ta=");

      break;
    case 'R':
      Flags[8] = true;

      break;
#ifdef LIBCURL
    case 's':
      Flags[19] = true;

      break;
#endif
    case 't':
      title = optarg;

      break;
    case 'T':
      Flags[9] = true;

      parseList(optarg);

      break;
    case 'u':
      Flags[10] = true;

      break;
    case 'x':
      Flags[11] = true;

      break;
    case 'v':
      Flags[12] = true;

      break;
    case 'q':
      Flags[13] = true;

      break;
    case 'h':
      ShowHelp();

      return 0;
    case '?':
      return 2;
    default:
      return 2;
    }
  }

  for (; optind < argc; optind++) {
    const fs::path inPl = getPath(fs::current_path(), argv[optind]);
    Entries inEntries;

    if (fs::exists(inPl)) {
      ParsePlaylist(inPl, inEntries);

      entries.insert(entries.end(), inEntries.begin(), inEntries.end());
    } else {
      std::cerr << "Skipping unfound file: " << inPl << std::endl;
    }
  }

  for (Entries::iterator it = entries.begin(); it != entries.end(); it++) {
    auto computeTargets = [&](fs::path &target, bool &local, bool &valid) {
      target = ProcessUri(target.string());

      if (!prepend.empty())
        target = getPath(prepend, target);

      local = localTarget(target.string());
      valid = validTarget(getPath(it->playlist.parent_path(), target));
    };

    computeTargets(it->target, it->localTarget, it->validTarget);

#ifdef TAGLIB
    if (it->localTarget && it->validTarget && Flags[25])
      fetchMetadata(*it);
#endif

    it->duplicateTarget = find(*it, entries) < it;
  }

  if (outPl.empty()) {
    if (Flags[11]) {
      std::cerr << "-x option requires an out playlist (-o)" << std::endl;

      return 2;
    }

    groupTargetItems(entries);
  } else {
    if (fs::exists(outPl) && !Flags[13] && !Flags[11]) {
      std::cerr << "File exists: " << outPl << std::endl;

      return 2;
    }

    if ((!base.empty() && Flags[0]) || (Flags[0] && Flags[8]) ||
        (!base.empty() && Flags[3]) || (Flags[3] && Flags[8])) {
      std::cerr << "Cannot combine absolute and relative path transforms"
                << std::endl;

      return 2;
    }

    for (Entries::iterator it = entries.begin(); it != entries.end(); it++) {
      it->track = std::distance(entries.begin(), it) + 1;
      if (!title.empty())
        it->playlistTitle = title;
      if (!image.empty())
        it->playlistImage = image;
    }

    for (const std::string &addItem : addItems) {
      Entry entry;
      entry.playlist = fs::current_path().append(".");
      entry.target = ProcessUri(addItem);
      entry.track = entries.size() + 1;
      entry.localTarget = localTarget(entry.target.string());
      entry.validTarget = validTarget(entry.target.string());

      entries.push_back(entry);
    }

    for (const std::string &changeItem : changeItems) {
      std::pair<std::string, std::string> pair1 = Split(changeItem, ":");
      std::pair<std::string, std::string> pair2 = Split(pair1.second, "=");
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
          entry.target = ProcessUri(value);
          entry.localTarget = localTarget(entry.target.string());
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

      for (Entry &entry : entries)
        if (entry.track == std::stoi(track))
          setEntry(entry);
    }

    if (Flags[6])
      std::shuffle(entries.begin(), entries.end(),
                   std::default_random_engine());

    for (Entries::iterator it = entries.begin(); it != entries.end();) {
      const auto encodeUri = [](const std::string &uri) {
        std::ostringstream uriStr;
        std::regex r("[!:\\/\\-._~0-9A-Za-z]");

        for (const char &c : uri) {
          if (std::regex_match(std::string({c}), r)) {
            uriStr << c;
          } else {
            uriStr << "%" << std::uppercase << std::hex << (0xff & c);
          }
        }

        return uriStr.str();
      };

      const auto transformPath = [&](const fs::path &basePath, fs::path &path) {
        fs::path target = getPath(basePath, path);

        if (Flags[0]) {
          path = target;
        } else if (Flags[3]) {
          path = "file://" + encodeUri(target);
        } else if (Flags[8]) {
          path = target.lexically_relative(outPl.parent_path());
        } else if (!base.empty()) {
          path = target.lexically_relative(base);
        }
      };

      it->duplicateTarget = find(*it, entries) < it;

      if (it->target.empty() || (!it->validTarget && Flags[10]) ||
          (it->duplicateTarget && Flags[1])) {

        it = entries.erase(it);

        continue;
      }

      it->track = std::distance(entries.begin(), it) + 1;

      if (it->localTarget) {
        transformPath(it->playlist.parent_path(), it->target);

        it->validTarget = fs::exists(getPath(outPl.parent_path(), ProcessUri(it->target)));
      }

      it->playlist = outPl;

      it++;
    }

    if (Flags[12])
      std::cout << "Generated playlist: " << entries.size() << " entries"
                << std::endl;

    groupTargetItems(entries);
  }

  if (entries.empty()) {
    std::cerr << "Nothing to do!" << std::endl;
    std::cout << "playlist -h for usage information" << std::endl;

    return 2;
  }

  if (Flags[14]) {
    listTargetItems(all);

    return 0;
  } else if (Flags[15]) {
    listTargetItems(dupe);

    return !Flags[13] ? !dupe.empty() : 0;
  } else if (Flags[16]) {
    listTargetItems(network);

    return 0;
  } else if (Flags[17]) {
    listTargetItems(unfound);

    return !Flags[13] ? !unfound.empty() : 0;
  } else if (Flags[18]) {
    listTargetItems(unique);

    return 0;
  }

  if (!outPl.empty() && !Flags[11]) {
    if (!unfound.empty() && !Flags[13] && !Flags[10])
      std::cout << "WARNING: out playlist has " << unfound.size()
                << " unfound target(s)" << std::endl;

    WritePlaylist(outPl, entries);
  } else {
    ShowPlaylist(entries);
  }

  return (!outPl.empty() && !Flags[13] && !Flags[10] && !Flags[11])
             ? !unfound.empty()
             : 0;
}
