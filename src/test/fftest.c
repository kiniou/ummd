/*
 *  ummd ( Micro MultiMedia Daemon )
 *
 *  Copyright (C) 2010 Nicolas Thill <nicolas.thill@gmail.com>
 */

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "util/log.h"

#include <fcntl.h>
#include <unistd.h>

#include <libavformat/avformat.h>
#include <linux/soundcard.h>


typedef struct my_demuxer_s my_demuxer_t;
struct my_demuxer_s
{
	uint8_t *io_buffer;
	int io_buffer_size;
	ByteIOContext *ff_io;
	AVInputFormat  *ff_if;
	AVFormatContext *ff_fc;
	AVCodecContext *ff_cc;
	int64_t pos;
};


static char *me;
static my_demuxer_t demuxer;
static int source_fd, target_fd;


static int my_io_read(void *opaque, uint8_t *buf, int buf_size)
{
	my_demuxer_t *d = opaque;

	int n;
	my_log(MY_LOG_NOTICE, "my_io_read(..., ..., %d) called", buf_size);
	n = read(source_fd, buf, buf_size);
	d->pos += n;
	return n;
}

static int64_t my_io_seek(void *opaque, int64_t offset, int whence)
{
	my_demuxer_t *d = opaque;

	my_log(MY_LOG_NOTICE, "my_io_seek(..., %lld, %d) called", offset, whence);
	if (whence == AVSEEK_SIZE) {
		my_log(MY_LOG_NOTICE, "my_io_seek: AVSEEK_SIZE");
		return -1;
	}
	if (whence == SEEK_SET) {
		my_log(MY_LOG_NOTICE, "my_io_seek: AVSEEK_SET");
	}
	if (whence == SEEK_CUR) {
		my_log(MY_LOG_NOTICE, "my_io_seek: AVSEEK_CUR");
	}
	if (whence == SEEK_END) {
		my_log(MY_LOG_NOTICE, "my_io_seek: AVSEEK_END");
	}
	my_log(MY_LOG_NOTICE, "my_io_seek: pos=%lld", d->pos);
	return d->pos;
}

static int my_loop(void)
{
	char dbuf[AVCODEC_MAX_AUDIO_FRAME_SIZE];
	char ibuf[AVCODEC_MAX_AUDIO_FRAME_SIZE];
	int ilen;
	char obuf[AVCODEC_MAX_AUDIO_FRAME_SIZE];
	int olen;
	AVCodec *av_dec;
	AVProbeData av_pd;
	AVInputFormat *av_if;
	AVFormatParameters av_fp;
	AVPacket av_pk;
	int rc;
	int i, audio_stream_index;

	av_register_all();

	bzero(ibuf, sizeof(ibuf));

	read(source_fd, ibuf, sizeof(ibuf));

	av_pd.buf = ibuf;
	av_pd.buf_size = sizeof(ibuf);
	av_pd.filename = NULL;

	my_log(MY_LOG_NOTICE, "probing input format");
	av_if = av_probe_input_format(&av_pd, 1);
	if (av_if == NULL) {
		av_if = av_find_input_format("s16le");
		av_fp.sample_rate = 44000;
		av_fp.channels = 2;
	}
	if (av_if == NULL) {
		my_log(MY_LOG_ERROR, "probing input format");
		goto _MY_ERR_av_probe_input_format;
	}
	demuxer.ff_if = av_if;

	my_log(MY_LOG_NOTICE, "input format found: %s", av_if->name);

	lseek(source_fd, 0, SEEK_SET);

	demuxer.io_buffer = ibuf;
	demuxer.io_buffer_size = sizeof(ibuf);

	my_log(MY_LOG_NOTICE, "creating I/O buffer");
	demuxer.ff_io = av_alloc_put_byte(demuxer.io_buffer, demuxer.io_buffer_size, 0, &demuxer, my_io_read, NULL, my_io_seek);
	if (demuxer.ff_io == NULL) {
		my_log(MY_LOG_ERROR, "creating I/O buffer");
		goto _MY_ERR_av_alloc_put_byte;
	}

	demuxer.ff_io->is_streamed = 1;
	demuxer.pos = 0;

	my_log(MY_LOG_NOTICE, "opening input stream");
	rc = av_open_input_stream(&(demuxer.ff_fc), demuxer.ff_io, "", demuxer.ff_if, &av_fp);
	if( rc < 0 ) {
		my_log(MY_LOG_ERROR, "opening input stream");
		goto _MY_ERR_av_open_input_stream;
	}

	my_log(MY_LOG_NOTICE, "finding stream info");
	rc = av_find_stream_info(demuxer.ff_fc);
	if( rc < 0 ) {
		my_log(MY_LOG_ERROR, "finding stream info");
		goto _MY_ERR_av_find_stream_info;
	}

	my_log(MY_LOG_NOTICE, "finding audio stream");
	audio_stream_index = -1;
	for (i = 0; i < demuxer.ff_fc->nb_streams; i++) {
		if (demuxer.ff_fc->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO && audio_stream_index < 0) {
			audio_stream_index = i;
			break;
		}
	}
	if (audio_stream_index < 0) {
		my_log(MY_LOG_ERROR, "finding audio stream");
		goto _MY_ERR_finding_audio_stream;
	}

	demuxer.ff_cc = demuxer.ff_fc->streams[audio_stream_index]->codec;

	my_log(MY_LOG_NOTICE, "finding audio codec");
	av_dec = avcodec_find_decoder(demuxer.ff_cc->codec_id);
	if (!av_dec) {
		my_log(MY_LOG_NOTICE, "finding audio codec");
		goto _MY_ERR_avcodec_find_decoder;
	}

	my_log(MY_LOG_NOTICE, "audio stream found, codec: %s, channels: %d, sample-rate: %d Hz", av_dec->name, demuxer.ff_cc->channels, demuxer.ff_cc->sample_rate);

	i = demuxer.ff_cc->channels;
	rc = ioctl(target_fd, SNDCTL_DSP_CHANNELS, &i);
	if (rc == -1) {
		my_log(MY_LOG_ERROR, "setting channels for output device (%d: %s)", errno, strerror(errno));
	}

	i = demuxer.ff_cc->sample_rate;
	rc = ioctl(target_fd, SNDCTL_DSP_SPEED, &i);
	if (rc == -1) {
		my_log(MY_LOG_ERROR, "setting sample rate for output device (%d: %s)", errno, strerror(errno));
	}

	i = AFMT_S16_NE;
	rc = ioctl(target_fd, SNDCTL_DSP_SETFMT, &i);
	if (rc == -1) {
		my_log(MY_LOG_ERROR, "setting audio format for output device (%d: %s)", errno, strerror(errno));
	}

	my_log(MY_LOG_NOTICE, "opening audio codec");
	if (avcodec_open(demuxer.ff_cc, av_dec) < 0) {
		my_log(MY_LOG_ERROR, "opening audio codec");
		goto _MY_ERR_avcodec_open;
	}

	my_log(MY_LOG_NOTICE, "reading audio frames");

	int osize;
	int isize;
	
	while (av_read_frame(demuxer.ff_fc, &av_pk) >= 0) {
		if (av_pk.stream_index == audio_stream_index) {
			my_log(MY_LOG_NOTICE, "read an audio frame, pts: %lld, dts: %lld, size: %d, duration: %d, pos: %lld", av_pk.pts, av_pk.dts, av_pk.size, av_pk.duration, av_pk.pos);
			osize = sizeof(obuf);
			isize = avcodec_decode_audio2(demuxer.ff_cc, (int16_t *)obuf, &osize, av_pk.data, av_pk.size);
			av_free_packet(&av_pk);
			if (isize < 0) {
				my_log(MY_LOG_ERROR, "decoding audio frame");
				goto _MY_ERR_avcodec_decode_audio2;
			}
			my_log(MY_LOG_NOTICE, "decoded an audio frame, isize: %d, osize: %d", isize, osize);
			i = write(target_fd, obuf, osize);
		}
	}
out:

	return 0;

_MY_ERR_avcodec_decode_audio2:
_MY_ERR_avcodec_open:
_MY_ERR_avcodec_find_decoder:
_MY_ERR_finding_audio_stream:
_MY_ERR_av_find_stream_info:
_MY_ERR_av_open_input_stream:
_MY_ERR_av_alloc_put_byte:
_MY_ERR_av_probe_input_format:
	return -1;
}

int main(int argc, char **argv)
{
	me = strrchr(argv[0], '/');
	if (me == NULL ) {
		me = argv[0];
	} else {
		me++;
	}

	my_log_init(me);

	if (my_log_open("stderr", MY_LOG_DEBUG) != 0) {
		goto _MY_ERR_log_open;
	}

	if (argc < 2) {
		my_log(MY_LOG_ERROR, "not enough arguments");
		goto _MY_ERR_argc;
	}

	my_log(MY_LOG_NOTICE, "opening '%s' for reading", argv[1]);
	source_fd = open(argv[1], O_RDONLY);
	if (source_fd == -1) {
		my_log(MY_LOG_ERROR, "error opening '%s' for reading", argv[1]);
		goto _MY_ERR_open_source;
	}

	my_log(MY_LOG_NOTICE, "opening '%s' for writing", argv[2]);
	target_fd = open(argv[2], O_WRONLY);
	if (target_fd == -1) {
		my_log(MY_LOG_ERROR, "error opening '%s' for writing", argv[2]);
		goto _MY_ERR_open_target;
	}

	my_loop();

	my_log_close();

	return 0;

_MY_ERR_open_target:
_MY_ERR_open_source:
_MY_ERR_argc:
	my_log_close();
_MY_ERR_log_open:
	return 1;
}
