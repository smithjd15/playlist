/* playlist asx module
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

#include "asx.h"

#include <iterator>

#include <pugixml.hpp>

#define ASX_ROOT "ASX"

using namespace pugi;

void ASX::parse(Entries &entries) {
  pugi::xml_document playlist;
  std::ifstream file(m_playlist);
  pugi::xml_parse_result result(playlist.load(file));
  pugi::xml_node plEntry;
  std::string creator, image, title;

  file.close();

  if (result && !file.bad() && playlist.child(ASX_ROOT)) {
    creator = playlist.child(ASX_ROOT).child("AUTHOR").text().as_string();
    title = playlist.child(ASX_ROOT).child("TITLE").text().as_string();

    for (const pugi::xml_node &param :
         playlist.child(ASX_ROOT).children("PARAM")) {
      if (param.attribute("NAME").as_string() == std::string("image"))
        image = param.attribute("VALUE").as_string();
    }

    int t = 1;
    for (const pugi::xml_node &plEntry :
         playlist.child(ASX_ROOT).children("ENTRY")) {
      Entry entry;

      entry.artist = plEntry.child("AUTHOR").text().as_string();
      entry.info = plEntry.child("MOREINFO").attribute("href").as_string();
      entry.target = plEntry.child("REF").attribute("href").as_string();
      entry.title = plEntry.child("TITLE").text().as_string();
      entry.track = t;

      for (const pugi::xml_node &param : plEntry.children("PARAM")) {
        if (param.attribute("NAME").as_string() == std::string("album"))
          entry.album = param.attribute("VALUE").as_string();
        if (param.attribute("NAME").as_string() == std::string("comment"))
          entry.comment = param.attribute("VALUE").as_string();
        if (param.attribute("NAME").as_string() == std::string("duration"))
          entry.duration = param.attribute("VALUE").as_int();
        if (param.attribute("NAME").as_string() == std::string("identifier"))
          entry.identifier = param.attribute("VALUE").as_string();
        if (param.attribute("NAME").as_string() == std::string("image"))
          entry.image = param.attribute("VALUE").as_string();
        if (param.attribute("NAME").as_string() == std::string("track"))
          entry.albumTrack = param.attribute("VALUE").as_int();
      }

      entry.playlist = m_playlist;
      entry.playlistArtist = creator;
      entry.playlistImage = image;
      entry.playlistTitle = title;

      entries.push_back(entry);

      t++;
    }
  } else {
    cwar << "Playlist parse error(s): " << m_playlist << std::endl;

    if (!result)
      cwar << result.description() << std::endl;

    if (!playlist.child(ASX_ROOT))
      cwar << "Unrecognized root node" << std::endl;
  }
}

void ASX::writePreProcess(List &list) {
  for (Entries::iterator it = list.entries.begin(); it != list.entries.end();) {
    if (!isUri(it->target.string()) && !it->target.is_relative()) {
      if (!flags[32])
        cwar << "Skipping absolute path: " << it->target << std::endl;

      it = list.entries.erase(it);

      continue;
    }

    it->track = std::distance(list.entries.begin(), it) + 1;

    it++;
  }
}

const bool ASX::write(const List &list) {
  pugi::xml_node doc, param, plEntry;
  pugi::xml_document pl;
  std::ofstream file(m_playlist);

  doc = pl.append_child(ASX_ROOT);
  doc.append_attribute("VERSION").set_value("3.0");

  if (!flags[18]) {
    if (!list.title.empty())
      doc.append_child("TITLE").text().set(list.title.c_str());
    if (!list.artist.empty())
      doc.append_child("AUTHOR").text().set(list.artist.c_str());

    if (!list.image.empty()) {
      param = doc.append_child("PARAM");
      param.append_attribute("NAME").set_value("image");
      param.append_attribute("VALUE").set_value(list.image.c_str());
    }
  }

  for (const Entry &entry : list.entries) {
    plEntry = doc.append_child("ENTRY");
    plEntry.append_child("REF").append_attribute("href").set_value(
        entry.target.c_str());

    if (!flags[18]) {
      if (!entry.title.empty())
        plEntry.append_child("TITLE").text().set(entry.title.c_str());
      if (!entry.artist.empty())
        plEntry.append_child("AUTHOR").text().set(entry.artist.c_str());

      if (!entry.info.empty())
        plEntry.append_child("MOREINFO")
            .append_attribute("href")
            .set_value(entry.info.c_str());

      if (!entry.album.empty()) {
        param = plEntry.append_child("PARAM");
        param.append_attribute("NAME").set_value("album");
        param.append_attribute("VALUE").set_value(entry.album.c_str());
      }

      if (!entry.comment.empty()) {
        param = plEntry.append_child("PARAM");
        param.append_attribute("NAME").set_value("comment");
        param.append_attribute("VALUE").set_value(entry.comment.c_str());
      }

      if (entry.duration > 0) {
        param = plEntry.append_child("PARAM");
        param.append_attribute("NAME").set_value("duration");
        param.append_attribute("VALUE").set_value(entry.duration);
      }

      if (!entry.identifier.empty()) {
        param = plEntry.append_child("PARAM");
        param.append_attribute("NAME").set_value("identifier");
        param.append_attribute("VALUE").set_value(entry.identifier.c_str());
      }

      if (!entry.image.empty()) {
        param = plEntry.append_child("PARAM");
        param.append_attribute("NAME").set_value("image");
        param.append_attribute("VALUE").set_value(entry.image.c_str());
      }

      if (entry.albumTrack) {
        param = plEntry.append_child("PARAM");
        param.append_attribute("NAME").set_value("track");
        param.append_attribute("VALUE").set_value(entry.albumTrack);
      }
    }
  }

  pl.save(file, "  ", pugi::format_default | pugi::format_no_declaration);

  file.close();

  return !file.fail();
}
