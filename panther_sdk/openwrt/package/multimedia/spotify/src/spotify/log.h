#ifndef SPOTIFY_LOG_H
#define SPOTIFY_LOG_H

/**
 * log on console
 */
extern void LOG(const char *format, ...) __attribute__((format(printf, 1, 0)));

#endif
