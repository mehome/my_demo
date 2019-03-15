/**
 * Copyright (c) 2013-2014 Spotify AB
 *
 * This file is part of the Spotify Embedded SDK examples suite.
 */

#include "appkey.h"
#include <stdio.h>

uint8_t g_appkey[400];
size_t g_appkey_size = sizeof(g_appkey);

SpError SpReadAppkey(const char *keyfile)
{
	FILE *fp = fopen(keyfile, "rb");
	if (!fp)
		return kSpErrorFailed;

	g_appkey_size = fread(g_appkey, 1, g_appkey_size, fp);
	if (!feof(fp)) {
		fclose(fp);
		return kSpErrorFailed;
	}

	fclose(fp);

	return kSpErrorOk;
}
