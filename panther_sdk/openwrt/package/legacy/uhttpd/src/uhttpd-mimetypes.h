/*
 * uhttpd - Tiny single-threaded httpd - MIME type definitions
 *
 *   Copyright (C) 2010 Jo-Philipp Wich <xm@subsignal.org>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _UHTTPD_MIMETYPES_

static struct mimetype uh_mime_types[] = {

	{ "txt",    "text/plain; charset=%s" },
	{ "log",    "text/plain" },
	{ "js",     "text/javascript" },
	{ "css",    "text/css" },
	{ "htm",    "text/html; charset=%s" },
	{ "html",   "text/html; charset=%s" },
	{ "asp", 		"text/html; charset=%s" },
	{ "diff",   "text/x-patch" },
	{ "patch",  "text/x-patch" },
	{ "c",      "text/x-csrc" },
	{ "h",      "text/x-chdr" },
	{ "o",      "text/x-object" },
	{ "ko",     "text/x-object" },

	{ "bmp",    "image/bmp" },
	{ "gif",    "image/gif" },
	{ "png",    "image/png" },
	{ "jpg",    "image/jpeg" },
	{ "jpeg",   "image/jpeg" },
	{ "svg",    "image/svg+xml" },
#if defined(CAMELOT)
	{ "cdr",    "image/cdr" },
	{ "psd",    "image/psd" },
#endif

	{ "zip",    "application/zip" },
	{ "pdf",    "application/pdf" },
	{ "xml",    "application/xml" },
	{ "xsl",    "application/xml" },
	{ "doc",    "application/msword" },
	{ "ppt",    "application/vnd.ms-powerpoint" },
	{ "xls",    "application/vnd.ms-excel" },
	{ "odt",    "application/vnd.oasis.opendocument.text" },
	{ "odp",    "application/vnd.oasis.opendocument.presentation" },
	{ "pl",     "application/x-perl" },
	{ "sh",     "application/x-shellscript" },
	{ "php",    "application/x-php" },
	{ "deb",    "application/x-deb" },
	{ "iso",    "application/x-cd-image" },
	{ "tgz",    "application/x-compressed-tar" },
	{ "gz",     "application/x-gzip" },
	{ "bz2",    "application/x-bzip" },
	{ "tar",    "application/x-tar" },
	{ "rar",    "application/x-rar-compressed" },

	{ "mp3",    "audio/mpeg" },
	{ "ogg",    "audio/x-vorbis+ogg" },
	{ "wav",    "audio/x-wav" },
#if defined(CAMELOT)
	{ "m3u",    "audio/m3u" },
	{ "wma",    "audio/wma" },
	{ "mid",    "audio/mid" },
	{ "vma",    "audio/vma" },
	{ "wave",   "audio/wave" },
	{ "ac3",    "audio/ac3" },
	{ "acc",    "audio/acc" },
	{ "au",     "audio/au" },
	{ "mmf",    "audio/mmf" },
	{ "ram",    "audio/ram" },
	{ "rm",     "audio/rm" },
	{ "wax",    "audio/wax" },
	{ "wvx",    "audio/wvx" },
	{ "aac",    "audio/aac" },
	{ "amr",    "audio/amr" },
	{ "awb",    "audio/awb" },
	{ "cd",     "audio/cd" },
	{ "flac",   "audio/flac" },
	{ "m4a",    "audio/m4a" },
	{ "m4r",    "audio/m4r" },
#endif

	{ "mpg",    "video/mpeg" },
	{ "mpeg",   "video/mpeg" },
	{ "avi",    "video/x-msvideo" },
#if defined(CAMELOT)
	{ "mp4",    "video/mp4" },
	{ "rm",     "video/rm" },
	{ "ram",    "video/ram" },
	{ "rmvb",   "video/rmvb" },
	{ "3gp",    "video/3gp" },
	{ "mov",    "video/mov" },
	{ "wmv",    "video/wmv" },
	{ "wkv",    "video/wkv" },
	{ "asf",    "video/asf" },
	{ "m4v",    "video/m4v" },
#endif

	{ "README", "text/plain" },
	{ "log",    "text/plain" },
	{ "cgi", 		"text/html" },
	{ "bin", 		"application/x-unknown" },
	{ "cfg",    "text/plain" },
	{ "conf",   "text/plain" },

	{ NULL, NULL }
};

#endif

