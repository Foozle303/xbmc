/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "SmartPlaylistDirectory.h"
#include "utils/log.h"
#include "playlists/SmartPlayList.h"
#include "music/MusicDatabase.h"
#include "video/VideoDatabase.h"
#include "Directory.h"
#include "File.h"
#include "FileItem.h"
#include "utils/URIUtils.h"

namespace XFILE
{
  CSmartPlaylistDirectory::CSmartPlaylistDirectory()
  {
  }

  CSmartPlaylistDirectory::~CSmartPlaylistDirectory()
  {
  }

  bool CSmartPlaylistDirectory::GetDirectory(const CStdString& strPath, CFileItemList& items)
  {
    // Load in the SmartPlaylist and get the WHERE query
    CSmartPlaylist playlist;
    if (!playlist.Load(strPath))
      return false;
    return GetDirectory(playlist, items);
  }
  
  bool CSmartPlaylistDirectory::GetDirectory(const CSmartPlaylist &playlist, CFileItemList& items)
  {
    bool success = false, success2 = false;
    std::set<CStdString> playlists;
    if (playlist.GetType().Equals("tvshows"))
    {
      CVideoDatabase db;
      db.Open();
      CVideoDatabase::Filter filter;
      filter.where = playlist.GetWhereClause(db, playlists);
      filter.order = playlist.GetOrderClause(db);
      filter.limit = playlist.GetLimitClause();
      success = db.GetTvShowsByWhere("videodb://2/2/", filter, items);
      items.SetContent("tvshows");
      db.Close();
    }
    else if (playlist.GetType().Equals("episodes"))
    {
      CVideoDatabase db;
      db.Open();
      CVideoDatabase::Filter filter;
      filter.where = playlist.GetWhereClause(db, playlists);
      filter.order = playlist.GetOrderClause(db);
      filter.limit = playlist.GetLimitClause();
      success = db.GetEpisodesByWhere("videodb://2/2/", filter, items);
      items.SetContent("episodes");
      db.Close();
    }
    else if (playlist.GetType().Equals("movies"))
    {
      CVideoDatabase db;
      db.Open();
      CVideoDatabase::Filter filter;
      filter.where = playlist.GetWhereClause(db, playlists);
      filter.order = playlist.GetOrderClause(db);
      filter.limit = playlist.GetLimitClause();
      success = db.GetMoviesByWhere("videodb://1/2/", filter, items, true);
      items.SetContent("movies");
      db.Close();
    }
    else if (playlist.GetType().Equals("albums"))
    {
      CMusicDatabase db;
      db.Open();
      CStdString whereClause = playlist.GetWhereClause(db, playlists);
      CStdString orderClause = playlist.GetOrderClause(db);
      CStdString limitClause = playlist.GetLimitClause();
      if (!whereClause.empty())
        whereClause = "WHERE " + whereClause;
      if (!orderClause.empty())
        orderClause = "ORDER BY " + orderClause;
      if (!limitClause.empty())
        orderClause += " LIMIT " + limitClause;

      success = db.GetAlbumsByWhere("musicdb://3/", whereClause, orderClause, items);
      items.SetContent("albums");
      db.Close();
    }
    if (playlist.GetType().Equals("songs") || playlist.GetType().Equals("mixed") || playlist.GetType().IsEmpty())
    {
      CMusicDatabase db;
      db.Open();
      CStdString whereOrder, whereClause, orderClause, limitClause;
      if (playlist.GetType().IsEmpty() || playlist.GetType().Equals("mixed"))
      {
        CSmartPlaylist songPlaylist(playlist);
        songPlaylist.SetType("songs");
        whereClause = songPlaylist.GetWhereClause(db, playlists);
        orderClause = songPlaylist.GetOrderClause(db);
        limitClause = songPlaylist.GetLimitClause();
      }
      else
      {
        whereClause = playlist.GetWhereClause(db, playlists);
        orderClause = playlist.GetOrderClause(db);
        limitClause = playlist.GetLimitClause();
      }

      if (!whereClause.empty())
        whereOrder = "WHERE " + whereClause;
      if (!orderClause.empty())
        whereOrder += " ORDER BY " + orderClause;
      if (!limitClause.empty())
        whereOrder += " LIMIT " + limitClause;

      success = db.GetSongsByWhere("", whereOrder, items);
      items.SetContent("songs");
      db.Close();
    }
    if (playlist.GetType().Equals("musicvideos") || playlist.GetType().Equals("mixed"))
    {
      CVideoDatabase db;
      db.Open();
      CVideoDatabase::Filter filter;
      if (playlist.GetType().Equals("mixed"))
      {
        CSmartPlaylist mvidPlaylist(playlist);
        mvidPlaylist.SetType("musicvideos");
        filter.where = mvidPlaylist.GetWhereClause(db, playlists);
        filter.order = mvidPlaylist.GetOrderClause(db);
        filter.limit = mvidPlaylist.GetLimitClause();
      }
      else
      {
        filter.where = playlist.GetWhereClause(db, playlists);
        filter.order = playlist.GetOrderClause(db);
        filter.limit = playlist.GetLimitClause();
      }

      CFileItemList items2;
      success2 = db.GetMusicVideosByWhere("videodb://3/2/", filter, items2, false); // TODO: SMARTPLAYLISTS Don't check locks???
      db.Close();
      items.Append(items2);
      if (items2.Size())
        items.SetContent("musicvideos");
    }
    items.SetLabel(playlist.GetName());
    // go through and set the playlist order
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      item->m_iprogramCount = i;  // hack for playlist order
    }
    if (playlist.GetType().Equals("mixed"))
      return success || success2;
    else if (playlist.GetType().Equals("musicvideos"))
      return success2;
    else
      return success;
  }

  bool CSmartPlaylistDirectory::ContainsFiles(const CStdString& strPath)
  {
    // smart playlists always have files??
    return true;
  }

  CStdString CSmartPlaylistDirectory::GetPlaylistByName(const CStdString& name, const CStdString& playlistType)
  {
    CFileItemList list;
    bool filesExist = false;
    if (playlistType == "songs" || playlistType == "albums")
      filesExist = CDirectory::GetDirectory("special://musicplaylists/", list, ".xsp", false);
    else // all others are video
      filesExist = CDirectory::GetDirectory("special://videoplaylists/", list, ".xsp", false);
    if (filesExist)
    {
      for (int i = 0; i < list.Size(); i++)
      {
        CFileItemPtr item = list[i];
        CSmartPlaylist playlist;
        if (playlist.OpenAndReadName(item->GetPath()))
        {
          if (playlist.GetName().CompareNoCase(name) == 0)
            return item->GetPath();
        }
      }
      for (int i = 0; i < list.Size(); i++)
      { // check based on filename
        CFileItemPtr item = list[i];
        if (URIUtils::GetFileName(item->GetPath()) == name)
        { // found :)
          return item->GetPath();
        }
      }
    }
    return "";
  }

  bool CSmartPlaylistDirectory::Remove(const char *strPath)
  {
    return XFILE::CFile::Delete(strPath);
  }
}



