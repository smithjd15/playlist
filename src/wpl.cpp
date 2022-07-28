/* playlist wpl module
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

#include "wpl.h"

#include <cmath>

#include <pugixml.hpp>

#define WPL_PI "wpl"
#define WPL_ROOT "smil"

using namespace pugi;

void WPL::parse(Entries &entries) {
  pugi::xml_document playlist;
  std::ifstream file(m_playlist);
  pugi::xml_parse_result result(
      playlist.load(file, pugi::parse_default | pugi::parse_pi));
  pugi::xml_node head, seq;
  std::string creator, image, title;

  file.close();

  if (result && !file.bad() && playlist.child(WPL_PI)) {
    head = playlist.child(WPL_ROOT).child("head");
    seq = playlist.child(WPL_ROOT).child("body").child("seq");
    title = head.child("title").text().as_string();

    for (const pugi::xml_node &meta : head.children("meta")) {
      if (meta.attribute("name").as_string() == std::string("Author"))
        creator = meta.attribute("content").as_string();
      if (meta.attribute("name").as_string() == std::string("Image"))
        image = meta.attribute("content").as_string();
    }

    int t = 1;
    for (const pugi::xml_node &media : seq.children("media")) {
      Entry entry;

      entry.target = media.attribute("src").as_string();
      entry.track = t;
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

    if (!playlist.child(WPL_PI))
      cwar << "Unrecognized processing instruction" << std::endl;
  }
}

const bool WPL::write(const List &list) {
  pugi::xml_node media, meta, pi, root;
  pugi::xml_document pl;
  std::ofstream file(m_playlist);

  pi = pl.append_child(pugi::node_declaration);
  pi.set_name(WPL_PI);
  pi.append_attribute("version").set_value("1.0");

  root = pl.append_child(WPL_ROOT);

  if (!flags[18]) {
    root.append_child("head");

    if (!list.title.empty())
      root.child("head").append_child("title").text().set(list.title.c_str());

    meta = root.child("head").append_child("meta");
    meta.append_attribute("name").set_value("Generator");
    meta.append_attribute("content").set_value(
        std::string("playlist -- " + ver).c_str());

    if (list.knownDuration > 0) {
      meta = root.child("head").append_child("meta");
      meta.append_attribute("name").set_value("TotalDuration");
      meta.append_attribute("content").set_value(
          std::ceil(list.knownDuration / 1000.0));
    }

    if (!list.artist.empty()) {
      meta = root.child("head").append_child("meta");
      meta.append_attribute("name").set_value("Author");
      meta.append_attribute("content").set_value(list.artist.c_str());
    }

    if (!list.image.empty()) {
      meta = root.child("head").append_child("meta");
      meta.append_attribute("name").set_value("Image");
      meta.append_attribute("content").set_value(list.image.c_str());
    }
  }

  root.append_child("body").append_child("seq");

  for (const Entry &entry : list.entries) {
    media = root.child("body").child("seq").append_child("media");
    media.append_attribute("src").set_value(entry.target.c_str());
  }

  pl.save(file, "  ", pugi::format_default | pugi::format_no_declaration);

  file.close();

  return !file.fail();
}
