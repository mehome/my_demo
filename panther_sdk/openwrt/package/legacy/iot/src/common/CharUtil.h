#ifndef __CHAR_UTIL_H__
#define __CHAR_UTIL_H__

static inline int IsASCII(unsigned char ch)
{
	return ch < 0x80;
}
static inline int IsWhitespaceOrNull(const char ch)
{
	return (unsigned char)ch <= 0x20;
}

static inline int IsWhitespaceNotNull(const char ch)
{
	return ch > 0 && ch <= 0x20;
}
static inline int IsPrintableASCII(char ch)
{
	return (signed char)ch >= 0x20;
}
static inline int IsDigitASCII(char ch)
{
	return ch >= '0' && ch <= '9';
}
static inline int IsUpperAlphaASCII(char ch)
{
	return ch >= 'A' && ch <= 'Z';
}

static inline int IsLowerAlphaASCII(char ch)
{
	return ch >= 'a' && ch <= 'z';
}

static inline int IsAlphaASCII(char ch)
{
	return IsUpperAlphaASCII(ch) || IsLowerAlphaASCII(ch);
}
static inline int IsAlphaNumericASCII(char ch)
{
	return IsAlphaASCII(ch) || IsDigitASCII(ch);
}

#endif