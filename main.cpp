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

#define VER 1.42

#define PLS_SECTION "[playlist]"
#define PLS_VERSION 2
#define XSPF_ROOT "playlist"

namespace fs = std::filesystem;

struct Entry {
  fs::path playlist;
  fs::path target;
  std::string title;
  float duration = 0;
  int track = 0;
  bool localTarget = false;
  bool validTarget = false;
  bool duplicateTarget = false;
};

typedef std::pair<const std::string, std::string> KeyValue;
typedef std::vector<KeyValue> TargetItems;
typedef std::vector<Entry> Entries;

std::bitset<18> Flags;

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

  if (uri.rfind("/..", 0) != std::string::npos) {
    uri = uri.substr(1);
  }

  return uri;
}

const Entries ParseXSPF(const fs::path &playlist) {
  pugi::xml_document playlistDoc;
  std::ifstream file(playlist);
  pugi::xml_parse_result result(playlistDoc.load(file));
  pugi::xml_node trackList, track;
  Entries entries;

  file.close();

  if (result && !file.bad() && playlistDoc.child(XSPF_ROOT)) {
    trackList = playlistDoc.child("playlist").child("trackList");

    for (track = trackList.child("track"); track;
         track = track.next_sibling("track")) {
      Entry entry;

      entry.duration = track.child("duration").text().as_int() / 1000.0;
      entry.playlist = playlist;
      entry.target = ProcessUri(track.child("location").text().as_string());
      entry.title = track.child("title").text().as_string();
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
  int plsEntries(0);
  int plsVersion(0);

  int t = 1;
  while (!file.eof()) {
    Entry entry;

    if (entries.empty() && (line.rfind(PLS_SECTION, 0) != std::string::npos))
      plsSection = true;

    if (line.rfind("File" + std::to_string(t), 0) != std::string::npos) {
      entry.playlist = playlist;
      entry.target = ProcessUri(Split(line).second);
      entry.track = t;

      std::getline(file, line);
      while (line.empty())
        std::getline(file, line);

      if (line.rfind("Title" + std::to_string(t), 0) != std::string::npos) {
        entry.title = Split(line).second;

        std::getline(file, line);
        while (line.empty())
          std::getline(file, line);
      }

      if (line.rfind("Length" + std::to_string(t), 0) != std::string::npos) {
        entry.duration = std::stoi(Split(line).second);

        std::getline(file, line);
        while (line.empty())
          std::getline(file, line);
      }

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
      std::cerr << "Parse error: Invalid version" << std::endl;

    entries.clear();
  }

  return entries;
}

const Entries ParseM3U(const fs::path &playlist) {
  std::ifstream file(playlist);
  std::string line;
  Entries entries;

  int t = 1;
  while (!file.eof()) {
    Entry entry;

    if (line.rfind("#EXTINF:", 0) != std::string::npos) {
      std::size_t pos = line.find_first_of(",");

      if (pos != std::string::npos) {
        entry.duration = std::stoi(line.substr(8, pos - 8));
        entry.title = line.substr(pos + 1);
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
      entry.target = ProcessUri(line);
      entry.track = t;

      entries.push_back(entry);

      t++;
    }

    std::getline(file, line);
  }

  file.close();

  if (file.bad())
    entries.clear();

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
  pugi::xml_node doc, playlist, trackList;
  pugi::xml_document pl;

  doc = pl.prepend_child(pugi::node_declaration);
  doc.append_attribute("version") = "1.0";
  doc.append_attribute("encoding") = "UTF-8";

  playlist = pl.append_child("playlist");
  playlist.append_attribute("version").set_value(1);
  playlist.append_attribute("xmlns").set_value("http://xspf.org/ns/0/");

  trackList = playlist.append_child("trackList");

  for (const Entry &entry : entries) {
    pugi::xml_node track = trackList.append_child("track");
    track.append_child("location").text().set(entry.target.c_str());

    if (!Flags[5]) {
      if (!entry.title.empty())
        track.append_child("title").text().set(entry.title.c_str());

      if (entry.duration > 0) {
        track.append_child("duration")
            .text()
            .set(std::to_string((int)(entry.duration * 1000)).c_str());
      }
    }
  }

  pl.save(file, "  ");
}

void WritePLS(std::ofstream &file, const Entries &entries) {
  file << "[playlist]" << std::endl;
  file << std::endl;

  for (const Entry &entry : entries) {
    file << "File" << entry.track << "=" << entry.target.string() << std::endl;

    if (!Flags[5]) {
      if (!entry.title.empty() || (entry.duration > 0)) {
        if (!entry.title.empty())
          file << "Title" << entry.track << "=" << entry.title << std::endl;

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
  file << "#EXTM3U" << std::endl;
  file << "#EXTENC: UTF-8" << std::endl;
  file << std::endl;

  for (const Entry &entry : entries) {
    if (!Flags[5]) {
      if ((!entry.title.empty() || entry.duration)) {
        file << "#EXTINF:";

        if (entry.duration > 0) {
          file << std::round(entry.duration) << ",";
        } else {
          file << "-1,";
        }

        if (!entry.title.empty())
          file << entry.title;

        file << std::endl;
      }
    }

    file << entry.target.string() << std::endl;

    if (!Flags[5] && (entry.track != (int)entries.size()))
      file << std::endl;
  }
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

    exit(2);
  }

  file.close();

  if (file.fail()) {
    std::cerr << "Write fail: " << playlist << std::endl;

    exit(2);
  }

  if (Flags[12])
    std::cout << "Playlist successfully written: " << playlist << std::endl;
}

void ShowPlaylist(const Entries &entries) {
  int totalDuration(0), dupe(0), network(0), unfound(0);
  time_t duration;
  tm *dur;
  std::string totalDur;

  std::cout << "Track"
            << "\tStatus"
            << "\tDuration"
            << "\tTitle"
            << "\tTarget" << std::endl;

  for (const Entry &entry : entries) {
    std::string target = entry.localTarget ? entry.target.filename().string()
                                           : entry.target.string(),
                status;
    float duration = !entry.validTarget && Flags[11] ? 0 : entry.duration;

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

    if (!entry.duplicateTarget && entry.localTarget && entry.validTarget)
      status = "*";

    if (duration > 0)
      totalDuration += entry.duration;

    std::cout << entry.track << "\t" << status << "\t" << duration << "\t"
              << entry.title << "\t" << target << std::endl;
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
  std::cout << "Total known duration: " << totalDuration << " seconds "
            << totalDur << std::endl;
}

void ShowHelp() {
  std::cout << "playlist version " << VER << std::endl;
  std::cout << "Copyright (C) 2021, 2022 James D. Smith" << std::endl;
  std::cout << std::endl;
  std::cout << "Usage: playlist [-l|-L|-D|-T|-P all|dupe|net|unfound|unique] "
               "[-p] [-f path] [[-O|-I]|[-R|-B path]] [-a target] [-r track] "
               "[-e track:ta(rget)|ti(tle)|du(ration)=value] [-d] [-u] [-n] "
               "[-m] [-q] [-v] [-x] [-o outfile.ext] infile..."
            << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "\t-l LIST Targets only" << std::endl;
  std::cout << "\t-L LIST Tracks and targets" << std::endl;
  std::cout << "\t-D LIST Durations and targets" << std::endl;
  std::cout << "\t-T LIST Titles and targets" << std::endl;
  std::cout << "\t-P LIST Playlist and targets" << std::endl;
  std::cout << "\t-p Same as -l all" << std::endl;
  std::cout << std::endl;
  std::cout << "\t-x Preview changes (with -o)" << std::endl;
  std::cout << "\t-f Prepend path to the in playlist targets" << std::endl;
  std::cout << "\t-o Out playlist file (.m3u, .pls, .xspf)" << std::endl;
  std::cout << "\t-R Out playlist targets relative to out playlist"
            << std::endl;
  std::cout << "\t-B Out playlist targets base path" << std::endl;
  std::cout << "\t-O Out playlist targets in absolute paths" << std::endl;
  std::cout << "\t-I Out playlist local targets in file URI scheme (implied -O)"
            << std::endl;
  std::cout << "\t-a Append entry" << std::endl;
  std::cout << "\t-r Remove entry (same as -e track:ta=)" << std::endl;
  std::cout << "\t-e Change entry" << std::endl;
  std::cout << "\t-d Remove duplicate targets from out playlist" << std::endl;
  std::cout << "\t-u Remove unfound targets from out playlist" << std::endl;
  std::cout << "\t-n Out playlist targets in random order" << std::endl;
  std::cout << "\t-m Minimal out playlist (targets only)" << std::endl;
  std::cout << std::endl;
  std::cout << "\t-q Quiet (clobber out playlist)" << std::endl;
  std::cout << "\t-v Verbose" << std::endl;
  std::cout << "\t-h This help" << std::endl;
  std::cout << std::endl;
  std::cout << "LIST can be one of: all, dupe, net, unfound, or unique "
               "(multiple infiles)"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Exit codes:" << std::endl;
  std::cout << "0: Success or quiet flag" << std::endl;
  std::cout << "1: Out playlist has unfound targets; or a dupe or unfound "
               "list contains at least one entry"
            << std::endl;
  std::cout << "2: IO, parse, or command line error" << std::endl;
}

int main(int argc, char **argv) {
  fs::path base, outPl, prepend;
  Entries entries;
  std::vector<std::string> addItems, changeItems;
  TargetItems all, dupe, network, unfound, unique;

  const auto getPath = [](const fs::path &p1, const fs::path &p2) {
    fs::path path(p1);

    if (p2.has_root_directory()) {
      path = p2;
    } else {
      path /= p2;
    }

    return path;
  };

  auto local = [](const std::string &target) {
    return (target.find("://") == std::string::npos);
  };

  auto parseList = [](const std::string &arg) {
    Flags[14] = (arg == "all");
    Flags[15] = (arg == "dupe");
    Flags[16] = (arg == "net");
    Flags[17] = (arg == "unfound");
    Flags[18] = (arg == "unique");
  };

  const auto targetItem = [](const Entry &entry) {
    if (Flags[2])
      return KeyValue(std::to_string((int)std::round(entry.duration)),
                      entry.target.string());

    if (Flags[7])
      return KeyValue(entry.playlist.string(), entry.target.string());

    if (Flags[9])
      return KeyValue(entry.title, entry.target.string());

    return KeyValue(std::to_string(entry.track), entry.target.string());
  };

  auto find = [](const Entry &entry, const Entries &entries,
                 bool sameList = true) {
    return std::find_if(entries.begin(), entries.end(),
                        [&entry, &sameList](const Entry &e) {
                          if (!sameList && (entry.playlist == e.playlist))
                            return false;

                          return (fs::weakly_canonical(entry.target.string()) ==
                                  fs::weakly_canonical(e.target.string()));
                        });
  };

  auto computeTargetDisposition = [&](Entries &entries) {
    for (Entries::iterator it = entries.begin(); it != entries.end(); it++) {
      fs::path target = ProcessUri(it->target.string());

      if (it->localTarget)
        target = getPath(it->playlist.parent_path(), target);

      it->validTarget = (!it->localTarget || fs::exists(target));
      it->duplicateTarget = find(*it, entries) < it;
    }
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
  while ((c = getopt(argc, argv, "a:B:dD:e:f:Il:L:mno:OpP:r:RT:uvxqh")) != -1) {
    switch (c) {
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
    case 'f':
      prepend = fs::path(optarg);

      break;
    case 'I':
      Flags[3] = true;

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
    case 'n':
      Flags[6] = true;

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
    it->localTarget = local(it->target);

    if (!prepend.empty() && it->localTarget) {
      fs::path target = prepend;
      target /= it->target;

      it->target = target;
    }
  }

  if (outPl.empty()) {
    if (Flags[11]) {
      std::cerr << "-x option requires an out playlist (-o)" << std::endl;

      return 2;
    }

    computeTargetDisposition(entries);
    groupTargetItems(entries);
  } else {
    if (fs::exists(outPl) && !Flags[13] && !Flags[11]) {
      std::cerr << "File exists: " << outPl << std::endl;

      return 2;
    }

    if ((Flags[0] && Flags[8]) || (Flags[0] && !base.empty()) ||
        (Flags[3] && Flags[8]) || (Flags[3] && !base.empty())) {
      std::cerr << "Cannot combine absolute and relative path transforms"
                << std::endl;

      return 2;
    }

    for (Entries::iterator it = entries.begin(); it != entries.end(); it++)
      it->track = std::distance(entries.begin(), it) + 1;

    for (const std::string &addItem : addItems) {
      Entry entry;
      entry.playlist = fs::current_path().append(".");
      entry.target = ProcessUri(addItem);
      entry.track = entries.size() + 1;
      entry.localTarget = local(entry.target);

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

        exit(2);
      };

      auto setEntry = [&](Entry &entry) {
        entry.playlist = fs::current_path().append(".");

        if (key == "du") {
          if (!std::all_of(value.begin(), value.end(), ::isdigit))
            invalid();
          entry.duration = value.empty() ? 0 : std::stoi(value);
        } else if (key == "ta") {
          entry.target = ProcessUri(value);
          entry.localTarget = local(entry.target);
        } else if (key == "ti") {
          entry.title = value;
        } else {
          invalid();
        }
      };

      if (track.empty() || !std::all_of(track.begin(), track.end(), ::isdigit))
        invalid();
      if (key.empty())
        invalid();
      if (key == pair1.second)
        invalid();

      for (Entry &entry : entries)
        if (entry.track == std::stoi(track))
          setEntry(entry);
    }

    computeTargetDisposition(entries);

    if (Flags[6])
      std::shuffle(entries.begin(), entries.end(),
                   std::default_random_engine());

    for (Entries::iterator it = entries.begin(); it != entries.end();) {
      const auto resolvePathDotSegments = [](const fs::path &inPath) {
        std::string path, segment;
        std::stringstream segStream(inPath.string());

        while (std::getline(segStream, segment, '/')) {
          if (segment.empty())
            continue;

          if (segment == "..") {
            std::size_t pos = path.find_last_of('/');

            if (pos)
              path = path.substr(0, pos);
          } else {
            if (inPath.has_root_path() || !path.empty())
              path += "/";
            path += segment;
          }
        }

        return fs::path(path);
      };

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

      if (it->target.empty() || (!it->validTarget && Flags[10]) ||
          (it->duplicateTarget && Flags[1])) {

        it = entries.erase(it);

        continue;
      }

      it->track = std::distance(entries.begin(), it) + 1;

      if (it->localTarget) {
        fs::path target = getPath(it->playlist.parent_path(), it->target);
        target = resolvePathDotSegments(target);

        if (Flags[0] || Flags[3]) {
          it->target = target;

          if (Flags[3])
            it->target = "file://" + encodeUri(target);
        } else {
          if (!base.empty()) {
            it->target = target.lexically_relative(base);
          } else if (Flags[8]) {
            it->target = target.lexically_relative(outPl.parent_path());
          }
        }
      }

      it->playlist = outPl;

      it++;
    }

    if (Flags[12])
      std::cout << "Generated playlist: " << entries.size() << " entries"
                << std::endl;

    computeTargetDisposition(entries);
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
