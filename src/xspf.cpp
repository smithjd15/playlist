/* playlist xspf module
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

#include "xspf.h"

#include <algorithm>

#include <pugixml.hpp>

#define XSPF_ROOT "playlist"

using namespace pugi;

void XSPF::parse(Entries &entries) {
  pugi::xml_document playlist;
  std::ifstream file(m_playlist);
  pugi::xml_parse_result result(playlist.load(file));
  pugi::xml_node trackList, track;
  std::string creator, image, title;

  file.close();

  if (result && !file.bad() && playlist.child(XSPF_ROOT)) {
    trackList = playlist.child(XSPF_ROOT).child("trackList");
    creator = playlist.child(XSPF_ROOT).child("creator").text().as_string();
    image = playlist.child(XSPF_ROOT).child("image").text().as_string();
    title = playlist.child(XSPF_ROOT).child("title").text().as_string();

    for (track = trackList.child("track"); track;
         track = track.next_sibling("track")) {
      Entry entry;

      entry.album = track.child("album").text().as_string();
      entry.comment = track.child("annotation").text().as_string();
      entry.artist = track.child("creator").text().as_string();
      entry.duration = track.child("duration").text().as_float();
      entry.identifier = track.child("identifier").text().as_string();
      entry.image = track.child("image").text().as_string();
      entry.info = track.child("info").text().as_string();
      entry.target = track.child("location").text().as_string();
      entry.title = track.child("title").text().as_string();
      entry.albumTrack = track.child("trackNum").text().as_int();
      entry.track =
          std::distance<pugi::xml_node::iterator>(trackList.begin(), track) + 1;
      entry.playlist = m_playlist;
      entry.playlistArtist = creator;
      entry.playlistImage = image;
      entry.playlistTitle = title;

      entries.push_back(entry);
    }
  } else {
    cwar << "Playlist parse error(s): " << m_playlist << std::endl;

    if (!result)
      cwar << result.description() << std::endl;

    if (!playlist.child(XSPF_ROOT))
      cwar << "Unrecognized root node" << std::endl;
  }
}

const bool XSPF::write(const List &list) {
  pugi::xml_node doc, playList, track, trackList;
  pugi::xml_document pl;
  std::ofstream file(m_playlist);

  doc = pl.prepend_child(pugi::node_declaration);
  doc.append_attribute("version") = "1.0";
  doc.append_attribute("encoding") = "UTF-8";

  playList = pl.append_child(XSPF_ROOT);
  playList.append_attribute("version").set_value(1);
  playList.append_attribute("xmlns").set_value("http://xspf.org/ns/0/");

  trackList = playList.append_child("trackList");

  for (const Entry &entry : list.entries) {
    track = trackList.append_child("track");
    track.append_child("location").text().set(entry.target.c_str());

    if (!flags[18]) {
      if (!entry.album.empty())
        track.append_child("album").text().set(entry.album.c_str());
      if (!entry.comment.empty())
        track.append_child("annotation").text().set(entry.comment.c_str());
      if (!entry.artist.empty())
        track.append_child("creator").text().set(entry.artist.c_str());
      if (entry.duration > 0)
        track.append_child("duration")
            .text()
            .set(std::to_string((int)entry.duration).c_str());
      if (!entry.identifier.empty())
        track.append_child("identifier").text().set(entry.identifier.c_str());
      if (!entry.image.empty())
        track.append_child("image").text().set(entry.image.c_str());
      if (!entry.info.empty())
        track.append_child("info").text().set(entry.info.c_str());
      if (!entry.title.empty())
        track.append_child("title").text().set(entry.title.c_str());
      if (entry.albumTrack)
        track.append_child("trackNum").text().set(entry.albumTrack);
    }
  }

  if (!flags[18]) {
    if (list.relative) {
      std::string base =
          list.playlist.parent_path().string() + fs::path::preferred_separator;
      playList.append_attribute("xml:base").set_value(base.c_str());
    }

    if (!list.title.empty())
      playList.insert_child_before("title", trackList)
          .text()
          .set(list.title.c_str());
    if (!list.artist.empty())
      playList.insert_child_before("creator", trackList)
          .text()
          .set(list.artist.c_str());
    if (!list.image.empty())
      playList.insert_child_before("image", trackList)
          .text()
          .set(list.image.c_str());
  }

  pl.save(file, "  ");

  file.close();

  return !file.fail();
}
