/*
 * Copyright (C) 2003-2014 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "PlsPlaylistPlugin.hxx"
#include "../PlaylistPlugin.hxx"
#include "../MemorySongEnumerator.hxx"
#include "input/InputStream.hxx"
#include "DetachedSong.hxx"
#include "tag/TagBuilder.hxx"
#include "util/Error.hxx"
#include "util/Domain.hxx"
#include "Log.hxx"

#ifdef HAVE_GLIB
#include <glib.h>
#else
#include <iostream>
#include <utility>
#include <vector>
#include <cstdlib>
#endif

#include <string>
#include <stdio.h>

#include <stdio.h>

static constexpr Domain pls_domain("pls");

#ifdef HAVE_GLIB
static void
pls_parser(GKeyFile *keyfile, std::forward_list<DetachedSong> &songs)
{
	gchar *value;
	GError *error = nullptr;
	int num_entries = g_key_file_get_integer(keyfile, "playlist",
						 "NumberOfEntries", &error);
	if (error) {
		FormatError(pls_domain,
			    "Invalid PLS file: '%s'", error->message);
		g_error_free(error);
		error = nullptr;

		/* Hack to work around shoutcast failure to comform to spec */
		num_entries = g_key_file_get_integer(keyfile, "playlist",
						     "numberofentries", &error);
		if (error) {
			g_error_free(error);
			error = nullptr;
		}
	}

	for (; num_entries > 0; --num_entries) {
		char key[64];
		sprintf(key, "File%u", num_entries);
		char *uri = g_key_file_get_string(keyfile, "playlist", key,
						  &error);
		if(error) {
			FormatError(pls_domain, "Invalid PLS entry %s: '%s'",
				    key, error->message);
			g_error_free(error);
			return;
		}

		TagBuilder tag;

		sprintf(key, "Title%u", num_entries);
		value = g_key_file_get_string(keyfile, "playlist", key,
					      nullptr);
		if (value != nullptr)
			tag.AddItem(TAG_TITLE, value);

		g_free(value);

		sprintf(key, "Length%u", num_entries);
		int length = g_key_file_get_integer(keyfile, "playlist", key,
						    nullptr);
		if (length > 0)
			tag.SetDuration(SignedSongTime::FromS(length));

		songs.emplace_front(uri, tag.Commit());
		g_free(uri);
	}

}
#else

typedef std::pair<std::string, std::string> 	PLS_ENTRY_PAIR;
typedef std::vector<PLS_ENTRY_PAIR> 			PLS_ENTRY_PAIR_VEC;
typedef PLS_ENTRY_PAIR_VEC::iterator 			PLS_ENTRY_PAIR_VEC_ITER;

#define GET_PLS_ENTRY_CONTENT(evp, key, content) \
	content.clear(); \
	for(PLS_ENTRY_PAIR_VEC_ITER iter = evp.begin(); iter != evp.end(); ++iter) { \
		if(iter->first.compare(key) == 0) { \
			content = iter->second; \
			break; \
		} \
	}\

static void
pls_parser(std::string fdata, std::forward_list<DetachedSong> &songs)
{
	std::size_t start = 0, end = 0, pos = 0;
	std::string content;
	PLS_ENTRY_PAIR_VEC pls_evp;
	//PLS_ENTRY_PAIR_VEC_ITER iter;

	while ((end = fdata.find("\n", start)) != std::string::npos) {
		std::string line = fdata.substr(start, end - start);
		start = end + 1;

		// get pls entry pair
		if ((pos = line.find("=")) != std::string::npos) {
			pls_evp.push_back(std::make_pair(line.substr(0, pos), line.substr(pos+1)));
		}
	}

	GET_PLS_ENTRY_CONTENT(pls_evp,"NumberOfEntries", content)
	int num_entries = std::atoi(content.c_str());
	//LogWarning(pls_domain, content.c_str());

	for (; num_entries > 0; --num_entries) {
		std::string uri;
		char key[64];

		sprintf(key, "File%u", num_entries);
		GET_PLS_ENTRY_CONTENT(pls_evp, key, uri)

		if(uri.empty()) {
			FormatError(pls_domain, "Invalid PLS entry %s not found", key);
			return;
		}

		//LogWarning(pls_domain, key);
		//LogWarning(pls_domain, uri.c_str());

		TagBuilder tag;

		sprintf(key, "Title%u", num_entries);
		GET_PLS_ENTRY_CONTENT(pls_evp, key, content)
		if (content.size() > 0)
			tag.AddItem(TAG_TITLE, content.c_str());

		//LogWarning(pls_domain, key);
		//LogWarning(pls_domain, content.c_str());

		sprintf(key, "Length%u", num_entries);
		GET_PLS_ENTRY_CONTENT(pls_evp, key, content)
		int length = std::atoi(content.c_str());
		if (length > 0)
			tag.SetDuration(SignedSongTime::FromS(length));

		//LogWarning(pls_domain, key);
		//LogWarning(pls_domain, content.c_str());

		songs.emplace_front(uri.c_str(), tag.Commit());
	}
}
#endif

static SongEnumerator *
pls_open_stream(InputStream &is)
{
#ifdef HAVE_GLIB
	GError *error = nullptr;
#endif
	Error error2;

	std::string kf_data;

	do {
		char buffer[1024];
		size_t nbytes = is.LockRead(buffer, sizeof(buffer), error2);
		if (nbytes == 0) {
			if (error2.IsDefined()) {
				LogError(error2);
				return nullptr;
			}

			break;
		}

		kf_data.append(buffer, nbytes);
		/* Limit to 64k */
	} while (kf_data.length() < 65536);

	if (kf_data.empty()) {
		LogWarning(pls_domain, "KeyFile parser failed: No Data");
		return nullptr;
	}

#ifdef HAVE_GLIB
	GKeyFile *keyfile = g_key_file_new();
	if (!g_key_file_load_from_data(keyfile,
				       kf_data.data(), kf_data.length(),
				       G_KEY_FILE_NONE, &error)) {
		FormatError(pls_domain,
			    "KeyFile parser failed: %s", error->message);
		g_error_free(error);
		g_key_file_free(keyfile);
		return nullptr;
	}

	std::forward_list<DetachedSong> songs;
	pls_parser(keyfile, songs);
	g_key_file_free(keyfile);
#else
	std::forward_list<DetachedSong> songs;
	pls_parser(kf_data, songs);
#endif

	return new MemorySongEnumerator(std::move(songs));
}

static const char *const pls_suffixes[] = {
	"pls",
	nullptr
};

static const char *const pls_mime_types[] = {
	"audio/x-scpls",
	nullptr
};

const struct playlist_plugin pls_playlist_plugin = {
	"pls",

	nullptr,
	nullptr,
	nullptr,
	pls_open_stream,

	nullptr,
	pls_suffixes,
	pls_mime_types,
};
