
#include "StringUtil.h"
#include "CharUtil.h"

#include <stddef.h>

static const char *StripLeft(const char *p)
{
	while (IsWhitespaceNotNull(*p))
		++p;

	return p;
}

static const char *StripLeftEnd(const char *p, const char *end)
{
	while (p < end && IsWhitespaceOrNull(*p))
		++p;

	return p;
}

static const char *StripRightEnd(const char *p, const char *end)
{
	while (end > p && IsWhitespaceOrNull(end[-1]))
		--end;

	return end;
}

static size_t StripRightLength(const char *p, size_t length)
{
	while (length > 0 && IsWhitespaceOrNull(p[length - 1]))
		--length;

	return length;
}

static const char * StripRight(char *p)
{
	size_t old_length = strlen(p);
	size_t new_length = StripRightLength(p, old_length);
	p[new_length] = 0;
}

char *Strip(char *p)
{
	p = StripLeft(p);
	StripRight(p);
	return p;
}
