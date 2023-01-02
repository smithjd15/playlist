/* playlist jspf module
 * Copyright (C) 2022 James D. Smith
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

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include "jspf.h"

#define JSPF_ROOT "playlist"

using namespace rapidjson;

void JSPF::parse(Entries &entries) {
  std::ifstream file(m_playlist);
  rapidjson::IStreamWrapper plWrapper(file);
  rapidjson::Document doc;
  rapidjson::ParseResult result(doc.ParseStream(plWrapper));
  std::string comment, creator, image, title;

  file.close();

  if (result && !file.bad() && doc.HasMember(JSPF_ROOT)) {
    if (doc[JSPF_ROOT].HasMember("annotation"))
      comment = doc[JSPF_ROOT]["annotation"].GetString();
    if (doc[JSPF_ROOT].HasMember("creator"))
      creator = doc[JSPF_ROOT]["creator"].GetString();
    if (doc[JSPF_ROOT].HasMember("image"))
      image = doc[JSPF_ROOT]["image"].GetString();
    if (doc[JSPF_ROOT].HasMember("title"))
      title = doc[JSPF_ROOT]["title"].GetString();

    rapidjson::GenericArray tracks = doc[JSPF_ROOT]["track"].GetArray();

    int t = 1;
    for (const rapidjson::GenericValue<
             UTF8<char>, MemoryPoolAllocator<CrtAllocator>> &track : tracks) {
      Entry entry;

      if (track.HasMember("album"))
        entry.album = track["album"].GetString();
      if (track.HasMember("annotation"))
        entry.comment = track["annotation"].GetString();
      if (track.HasMember("creator"))
        entry.artist = track["creator"].GetString();
      if (track.HasMember("duration"))
        entry.duration = track["duration"].GetInt();
      if (track.HasMember("identifier"))
        entry.identifier = track["identifier"].GetString();
      if (track.HasMember("image"))
        entry.image = track["image"].GetString();
      if (track.HasMember("info"))
        entry.info = track["info"].GetString();
      if (track.HasMember("location"))
        entry.target = track["location"].GetString();
      if (track.HasMember("title"))
        entry.title = track["title"].GetString();
      if (track.HasMember("trackNum"))
        entry.albumTrack = track["trackNum"].GetInt();
      entry.track = t;
      entry.playlist = m_playlist;
      entry.playlistArtist = creator;
      entry.playlistComment = comment;
      entry.playlistImage = image;
      entry.playlistTitle = title;

      entries.push_back(entry);

      t++;
    }
  } else {
    cwar << "Playlist parse error(s): " << m_playlist << std::endl;

    if (!result)
      cwar << rapidjson::GetParseError_En(result.Code()) << std::endl;

    if (!doc.HasMember(JSPF_ROOT))
      cwar << "Unrecognized root" << std::endl;
  }
}

const bool JSPF::write(const List &list) {
  rapidjson::Document doc;
  rapidjson::Value tracks(kObjectType), track(kArrayType);
  std::ofstream file(m_playlist);
  rapidjson::OStreamWrapper plWrapper(file);
  rapidjson::PrettyWriter<OStreamWrapper> plWriter(plWrapper);

  doc.SetObject();
  doc.AddMember(JSPF_ROOT, tracks, doc.GetAllocator());

  for (const Entry &entry : list.entries) {
    rapidjson::Value plEntry(kObjectType);

    plEntry.AddMember("location", StringRef(entry.target.c_str()),
                      doc.GetAllocator());

    if (!flags[18]) {
      if (!entry.album.empty())
        plEntry.AddMember("album", StringRef(entry.album.c_str()),
                          doc.GetAllocator());
      if (!entry.comment.empty())
        plEntry.AddMember("annotation", StringRef(entry.comment.c_str()),
                          doc.GetAllocator());
      if (!entry.artist.empty())
        plEntry.AddMember("creator", StringRef(entry.artist.c_str()),
                          doc.GetAllocator());
      if (entry.duration > 0)
        plEntry.AddMember("duration", entry.duration, doc.GetAllocator());
      if (!entry.identifier.empty())
        plEntry.AddMember("identifier", StringRef(entry.identifier.c_str()),
                          doc.GetAllocator());
      if (!entry.image.empty())
        plEntry.AddMember("image", StringRef(entry.image.c_str()),
                          doc.GetAllocator());
      if (!entry.info.empty())
        plEntry.AddMember("info", StringRef(entry.info.c_str()),
                          doc.GetAllocator());
      if (!entry.title.empty())
        plEntry.AddMember("title", StringRef(entry.title.c_str()),
                          doc.GetAllocator());
      if (entry.albumTrack)
        plEntry.AddMember("trackNum", entry.albumTrack, doc.GetAllocator());
    }

    track.PushBack(plEntry, doc.GetAllocator());
  }

  if (!flags[18]) {
    if (!list.comment.empty())
      doc[JSPF_ROOT].AddMember("annotation", StringRef(list.comment.c_str()),
                               doc.GetAllocator());
    if (!list.artist.empty())
      doc[JSPF_ROOT].AddMember("creator", StringRef(list.artist.c_str()),
                               doc.GetAllocator());
    if (!list.image.empty())
      doc[JSPF_ROOT].AddMember("image", StringRef(list.image.c_str()),
                               doc.GetAllocator());
    if (!list.title.empty())
      doc[JSPF_ROOT].AddMember("title", StringRef(list.title.c_str()),
                               doc.GetAllocator());
  }

  doc[JSPF_ROOT].AddMember("track", track, doc.GetAllocator());

  plWriter.SetIndent(' ', 2);
  doc.Accept(plWriter);

  file.close();

  return !file.fail();
}
