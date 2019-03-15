/*
 * Copyright (C) 2017 Facebook
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Important notes about in-place decompression
 *
 * At least on x86, the kernel is decompressed in place: the compressed data
 * is placed to the end of the output buffer, and the decompressor overwrites
 * most of the compressed data. There must be enough safety margin to
 * guarantee that the write position is always behind the read position.
 *
 * The safety margin for ZSTD with a 128 KB block size is calculated below.
 * Note that the margin with ZSTD is bigger than with GZIP or XZ!
 *
 * The worst case for in-place decompression is that the beginning of
 * the file is compressed extremely well, and the rest of the file is
 * uncompressible. Thus, we must look for worst-case expansion when the
 * compressor is encoding uncompressible data.
 *
 * The structure of the .zst file in case of a compresed kernel is as follows.
 * Maximum sizes (as bytes) of the fields are in parenthesis.
 *
 *    Frame Header: (18)
 *    Blocks: (N)
 *    Checksum: (4)
 *
 * The frame header and checksum overhead is at most 22 bytes.
 *
 * ZSTD stores the data in blocks. Each block has a header whose size is
 * a 3 bytes. After the block header, there is up to 128 KB of payload.
 * The maximum uncompressed size of the payload is 128 KB. The minimum
 * uncompressed size of the payload is never less than the payload size
 * (excluding the block header).
 *
 * The assumption, that the uncompressed size of the payload is never
 * smaller than the payload itself, is valid only when talking about
 * the payload as a whole. It is possible that the payload has parts where
 * the decompressor consumes more input than it produces output. Calculating
 * the worst case for this would be tricky. Instead of trying to do that,
 * let's simply make sure that the decompressor never overwrites any bytes
 * of the payload which it is currently reading.
 *
 * Now we have enough information to calculate the safety margin. We need
 *   - 22 bytes for the .zst file format headers;
 *   - 3 bytes per every 128 KiB of uncompressed size (one block header per
 *     block); and
 *   - 128 KiB (biggest possible zstd block size) to make sure that the
 *     decompressor never overwrites anything from the block it is currently
 *     reading.
 *
 * We get the following formula:
 *
 *    safety_margin = 22 + uncompressed_size * 3 / 131072 + 131072
 *                 <= 22 + (uncompressed_size >> 15) + 131072
 */

#ifdef STATIC
	/* Preboot environments #include "path/to/decompress_unzstd.c".
	 * All of the source files we depend on must be #included.
	 * zstd's only source dependeny is xxhash, which has no source
	 * dependencies.
	 *
	 * zstd and xxhash both avoid declaring themselves as modules
	 * when PREBOOT is defined.
	 */
#	define PREBOOT
#	include "xxhash.c"
#	include "zstd/entropy_common.c"
#	include "zstd/fse_decompress.c"
#	include "zstd/huf_decompress.c"
#	include "zstd/zstd_common.c"
#	include "zstd/decompress.c"
#endif

#include <linux/decompress/mm.h>
#include <linux/kernel.h>
#include <linux/zstd.h>

/* 8 MB maximum window size */
#define ZSTD_WINDOWSIZE_MAX	(1 << 23)
/* Size of the input and output buffers in multi-call mdoe */
#define ZSTD_IOBUF_SIZE		4096

static int INIT handle_zstd_error(size_t ret, void (*error)(char *x))
{
	const int err = ZSTD_getErrorCode(ret);

	if (!ZSTD_isError(ret))
		return 0;

	switch (err) {
	case ZSTD_error_memory_allocation:
		error("ZSTD decompressor ran out of memory");
		break;
	case ZSTD_error_prefix_unknown:
		error("Input is not in the ZSTD format (wrong magic bytes)");
		break;
	case ZSTD_error_dstSize_tooSmall:
	case ZSTD_error_corruption_detected:
	case ZSTD_error_checksum_wrong:
		error("ZSTD-compressed data is corrupt");
		break;
	default:
		error("ZSTD-compressed data is probably corrupt");
		break;
	}
	return -1;
}

/* Handle the case where we have the entire input and output in one segment.
 * We can allocate less memory (no circular buffer for the sliding window),
 * and avoid some memcpy() calls.
 */
static int INIT decompress_single(const u8 *in_buf, long in_len, u8 *out_buf,
				  long out_len, long *in_pos,
				  void (*error)(char *x))
{
	const size_t wksp_size = ZSTD_DCtxWorkspaceBound();
	void *wksp = large_malloc(wksp_size);
	ZSTD_DCtx *dctx = ZSTD_initDCtx(wksp, wksp_size);
	int err;
	size_t ret;

	if (dctx == NULL) {
		error("Out of memory while allocating ZSTD_DCtx");
		err = -1;
		goto out;
	}
	/* Find out how large the frame actually is, there may be junk at
	 * the end of the frame that ZSTD_decompressDCtx() can't handle.
	 */
	ret = ZSTD_findFrameCompressedSize(in_buf, in_len);
	err = handle_zstd_error(ret, error);
	if (err)
		goto out;
	in_len = (long)ret;

	ret = ZSTD_decompressDCtx(dctx, out_buf, out_len, in_buf, in_len);
	err = handle_zstd_error(ret, error);
	if (err)
		goto out;

	if (in_pos != NULL)
		*in_pos = in_len;

	err = 0;
out:
	if (wksp != NULL)
		large_free(wksp);
	return err;
}

static int INIT __unzstd(unsigned char *in_buf, long in_len,
			 long (*fill)(void*, unsigned long),
			 long (*flush)(void*, unsigned long),
			 unsigned char *out_buf, long out_len,
			 long *in_pos,
			 void (*error)(char *x))
{
	ZSTD_inBuffer in;
	ZSTD_outBuffer out;
	ZSTD_frameParams params;
	void *in_allocated = NULL;
	void *out_allocated = NULL;
	void *wksp = NULL;
	size_t wksp_size;
	ZSTD_DStream *dstream;
	int err;
	size_t ret;

#if defined(CONFIG_PANTHER)
	if (out_len == 0)
		out_len = 0x1FFFFFFF;
#else
	if (out_len == 0)
		out_len = LONG_MAX; /* no limit */
#endif

	if (fill == NULL && flush == NULL)
		/* We can decompress faster and with less memory when we have a
		 * single chunk.
		 */
		return decompress_single(in_buf, in_len, out_buf, out_len,
					 in_pos, error);

	/* If in_buf is not provided, we must be using fill(), so allocate
	 * a large enough buffer. If it is provided, it must be at least
	 * ZSTD_IOBUF_SIZE large.
	 */
	if (in_buf == NULL) {
		in_allocated = malloc(ZSTD_IOBUF_SIZE);
		if (in_allocated == NULL) {
			error("Out of memory while allocating input buffer");
			err = -1;
			goto out;
		}
		in_buf = in_allocated;
		in_len = 0;
	}
	/* Read the first chunk, since we need to decode the frame header */
	if (fill != NULL)
		in_len = fill(in_buf, ZSTD_IOBUF_SIZE);
	if (in_len < 0) {
		error("ZSTD-compressed data is truncated");
		err = -1;
		goto out;
	}
	/* Set the first non-empty input buffer */
	in.src = in_buf;
	in.pos = 0;
	in.size = in_len;
	/* Allocate the output buffer if we are using flush(). */
	if (flush != NULL) {
		out_allocated = malloc(ZSTD_IOBUF_SIZE);
		if (out_allocated == NULL) {
			error("Out of memory while allocating output buffer");
			err = -1;
			goto out;
		}
		out_buf = out_allocated;
		out_len = ZSTD_IOBUF_SIZE;
	}
	/* Set the output buffer */
	out.dst = out_buf;
	out.pos = 0;
	out.size = out_len;

	/* We need to know the window size to allocate the ZSTD_DStream.
	 * Since we are streaming, we need to allocate a buffer for the sliding
	 * window. The window size varies from 1 KB to ZSTD_WINDOWSIZE_MAX
	 * (8 MB), so it is important to use the actual value so as not to
	 * waste memory when it is smaller.
	 */
	ret = ZSTD_getFrameParams(&params, in.src, in.size);
	err = handle_zstd_error(ret, error);
	if (err)
		goto out;
	if (ret != 0) {
		error("ZSTD-compressed data has an incomplete frame header");
		err = -1;
		goto out;
	}
	if (params.windowSize > ZSTD_WINDOWSIZE_MAX) {
		error("ZSTD-compressed data has too large a window size");
		err = -1;
		goto out;
	}

	/* Allocate the ZSTD_DStream now that we know how much memory is
	 * required.
	 */
	wksp_size = ZSTD_DStreamWorkspaceBound(params.windowSize);
	wksp = large_malloc(wksp_size);
	dstream = ZSTD_initDStream(params.windowSize, wksp, wksp_size);
	if (dstream == NULL) {
		error("Out of memory while allocating ZSTD_DStream");
		err = -1;
		goto out;
	}
	/* Decompression loop:
	 * Read more data if necessary (error if no more data can be read).
	 * Call the decompression function, which returns 0 when finished.
	 * Flush any data produced if using flush().
	 */
	if (in_pos != NULL)
		*in_pos = 0;
	do {
		/* If we need to reload data, either we have fill() and can
		 * try to get more data, or we don't and the input is truncated.
		 */
		if (in.pos == in.size) {
			if (in_pos != NULL)
				*in_pos += in.pos;
			in_len = fill ? fill(in_buf, ZSTD_IOBUF_SIZE) : -1;
			if (in_len < 0) {
				error("ZSTD-compressed data is truncated");
				err = -1;
				goto out;
			}
			in.pos = 0;
			in.size = in_len;
		}
		/* Returns zero when the frame is complete */
		ret = ZSTD_decompressStream(dstream, &out, &in);
		err = handle_zstd_error(ret, error);
		if (err)
			goto out;
		/* Flush all of the data produced if using flush() */
		if (flush != NULL && out.pos > 0) {
			if (out.pos != flush(out.dst, out.pos)) {
				error("Failed to flush()");
				err = -1;
				goto out;
			}
			out.pos = 0;
		}
	} while (ret != 0);

	if (in_pos != NULL)
		*in_pos += in.pos;

	err = 0;
out:
	if (in_allocated != NULL)
		free(in_allocated);
	if (out_allocated != NULL)
		free(out_allocated);
	if (wksp != NULL)
		large_free(wksp);
	return err;
}

#ifndef PREBOOT
STATIC int INIT unzstd(unsigned char *buf, long len,
		       long (*fill)(void*, unsigned long),
		       long (*flush)(void*, unsigned long),
		       unsigned char *out_buf,
		       long *pos,
		       void (*error)(char *x))
{
	return __unzstd(buf, len, fill, flush, out_buf, 0, pos, error);
}
#else
STATIC int INIT __decompress(unsigned char *buf, long len,
			     long (*fill)(void*, unsigned long),
			     long (*flush)(void*, unsigned long),
			     unsigned char *out_buf, long out_len,
			     long *pos,
			     void (*error)(char *x))
{
	return __unzstd(buf, len, fill, flush, out_buf, out_len, pos, error);
}
#endif
