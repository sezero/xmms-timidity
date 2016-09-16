/*
    xmms-timidity - MIDI Plugin for XMMS
    Copyright (C) 2004 Konstantin Korikov <lostclus@ua.fm>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <xmms/configfile.h>
#include <xmms/titlestring.h>
#include <xmms/util.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <string.h>
#include <timidity.h>

#include "xmms-timidity.h"
#include "interface.h"

InputPlugin xmmstimid_ip = {
	NULL,
	NULL,
	NULL,
	xmmstimid_init,
	xmmstimid_about,
	xmmstimid_configure,
	xmmstimid_is_our_file,
	NULL,
	xmmstimid_play_file,
	xmmstimid_stop,
	xmmstimid_pause,
	xmmstimid_seek,
	NULL,
	xmmstimid_get_time,
	NULL,
	NULL,
	xmmstimid_cleanup,
	NULL,
	NULL,
	NULL,
	NULL,
	xmmstimid_get_song_info,
	NULL,
	NULL
};

static struct {
	gchar *config_file;
	gint rate;
	gint bits;
	gint channels;
	gint buffer_size;
} xmmstimid_cfg;

static gboolean xmmstimid_initialized = FALSE;
static pthread_t xmmstimid_decode_thread;
static gboolean xmmstimid_audio_error = FALSE;
static MidSongOptions xmmstimid_opts;
static MidSong *xmmstimid_song;
static gboolean xmmstimid_going;
static gboolean xmmstimid_eof;
static gint xmmstimid_seek_to;

static GtkWidget *xmmstimid_conf_wnd = NULL, *xmmstimid_about_wnd = NULL;
static GtkEntry
	*xmmstimid_conf_config_file;
static GtkToggleButton
	*xmmstimid_conf_rate_11000,
	*xmmstimid_conf_rate_22000,
	*xmmstimid_conf_rate_44100;
static GtkToggleButton
	*xmmstimid_conf_bits_8,
	*xmmstimid_conf_bits_16;
static GtkToggleButton
	*xmmstimid_conf_channels_1,
	*xmmstimid_conf_channels_2;

InputPlugin *get_iplugin_info(void) {
	xmmstimid_ip.description = g_strdup_printf(
			"TiMidity Player %s", VERSION);
	return &xmmstimid_ip;
}

void xmmstimid_init(void) {
	ConfigFile *cf;

	xmmstimid_cfg.config_file = NULL;
	xmmstimid_cfg.rate = 44100;
	xmmstimid_cfg.bits = 16;
	xmmstimid_cfg.channels = 2;
	xmmstimid_cfg.buffer_size = 512;

	cf = xmms_cfg_open_default_file();
	if (cf != NULL) {
		xmms_cfg_read_string(cf, "TIMIDITY", "config_file",
				&xmmstimid_cfg.config_file);
		xmms_cfg_read_int(cf, "TIMIDITY", "rate",
				&xmmstimid_cfg.rate);
		xmms_cfg_read_int(cf, "TIMIDITY", "bits",
				&xmmstimid_cfg.bits);
		xmms_cfg_read_int(cf, "TIMIDITY", "channels",
				&xmmstimid_cfg.channels);
		xmms_cfg_free(cf);
	}

	if (xmmstimid_cfg.config_file == NULL)
		xmmstimid_cfg.config_file = g_strdup("/etc/timidity.cfg");

	if (mid_init(xmmstimid_cfg.config_file) != 0) {
		xmmstimid_initialized = FALSE;
		return;
	}
	xmmstimid_initialized = TRUE;
}

void xmmstimid_about(void) {
	if (xmmstimid_about_wnd == NULL) {
		gchar *name_version;
		xmmstimid_about_wnd = create_xmmstimid_about_wnd();
		name_version = g_strdup_printf(
				"TiMidity Plugin %s", VERSION);
		gtk_label_set_text(
				GTK_LABEL(gtk_object_get_data(
				GTK_OBJECT(xmmstimid_about_wnd),
				 "name_version")), name_version);
		g_free(name_version);
	}

	gtk_widget_show(xmmstimid_about_wnd);
	gdk_window_raise(xmmstimid_about_wnd->window);
}

void xmmstimid_conf_ok(GtkButton *button, gpointer user_data);

void xmmstimid_configure(void) {
	GtkToggleButton *tb;
	if (xmmstimid_conf_wnd == NULL) {
		xmmstimid_conf_wnd = create_xmmstimid_conf_wnd();

#define get_conf_wnd_item(type, name) \
	type (gtk_object_get_data(GTK_OBJECT(xmmstimid_conf_wnd), name))
	
		xmmstimid_conf_config_file = get_conf_wnd_item(
				GTK_ENTRY, "config_file");
		xmmstimid_conf_rate_11000 = get_conf_wnd_item(
				GTK_TOGGLE_BUTTON, "rate_11000");
		xmmstimid_conf_rate_22000 = get_conf_wnd_item(
				GTK_TOGGLE_BUTTON, "rate_22000");
		xmmstimid_conf_rate_44100 = get_conf_wnd_item(
				GTK_TOGGLE_BUTTON, "rate_44100");
		xmmstimid_conf_bits_8 = get_conf_wnd_item(
				GTK_TOGGLE_BUTTON, "bits_8");
		xmmstimid_conf_bits_16 = get_conf_wnd_item(
				GTK_TOGGLE_BUTTON, "bits_16");
		xmmstimid_conf_channels_1 = get_conf_wnd_item(
				GTK_TOGGLE_BUTTON, "channels_1");
		xmmstimid_conf_channels_2 = get_conf_wnd_item(
				GTK_TOGGLE_BUTTON, "channels_2");

		gtk_signal_connect_object(
				get_conf_wnd_item(GTK_OBJECT, "conf_ok"),
				"clicked", GTK_SIGNAL_FUNC(xmmstimid_conf_ok),
				NULL);
	}

	gtk_entry_set_text(xmmstimid_conf_config_file,
			xmmstimid_cfg.config_file);
	switch (xmmstimid_cfg.rate) {
		case 11000: tb = xmmstimid_conf_rate_11000; break;
		case 22000: tb = xmmstimid_conf_rate_22000; break;
		case 44100: tb = xmmstimid_conf_rate_44100; break;
		default: tb = NULL;
	}
	if (tb != NULL) gtk_toggle_button_set_active(tb, TRUE);
	switch (xmmstimid_cfg.bits) {
		case 8: tb = xmmstimid_conf_bits_8; break;
		case 16: tb = xmmstimid_conf_bits_16; break;
		default: tb = NULL;
	}
	if (tb != NULL) gtk_toggle_button_set_active(tb, TRUE);
	switch (xmmstimid_cfg.channels) {
		case 1: tb = xmmstimid_conf_channels_1; break;
		case 2: tb = xmmstimid_conf_channels_2; break;
		default: tb = NULL;
	}
	if (tb != NULL) gtk_toggle_button_set_active(tb, TRUE);

	gtk_widget_show(xmmstimid_conf_wnd);
	gdk_window_raise(xmmstimid_conf_wnd->window);
}

void xmmstimid_conf_ok(GtkButton *button, gpointer user_data) {
	gchar *config_file, *filename;
	ConfigFile *cf;

	g_free(xmmstimid_cfg.config_file);
	xmmstimid_cfg.config_file = g_strdup(
			gtk_entry_get_text(xmmstimid_conf_config_file));

	if (gtk_toggle_button_get_active(xmmstimid_conf_rate_11000))
		xmmstimid_cfg.rate = 11000;
	else if (gtk_toggle_button_get_active(xmmstimid_conf_rate_22000))
		xmmstimid_cfg.rate = 22000;
	else if (gtk_toggle_button_get_active(xmmstimid_conf_rate_44100))
		xmmstimid_cfg.rate = 44100;
	if (gtk_toggle_button_get_active(xmmstimid_conf_bits_8))
		xmmstimid_cfg.bits = 8;
	else if (gtk_toggle_button_get_active(xmmstimid_conf_bits_16))
		xmmstimid_cfg.bits = 16;
	if (gtk_toggle_button_get_active(xmmstimid_conf_channels_1))
		xmmstimid_cfg.channels = 1;
	else if (gtk_toggle_button_get_active(xmmstimid_conf_channels_2))
		xmmstimid_cfg.channels = 2;

	filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
	cf = xmms_cfg_open_file(filename);
	if (cf == NULL) cf = xmms_cfg_new();

	xmms_cfg_write_string(cf, "TIMIDITY", "config_file",
			xmmstimid_cfg.config_file);
	xmms_cfg_write_int(cf, "TIMIDITY", "rate",
			xmmstimid_cfg.rate);
	xmms_cfg_write_int(cf, "TIMIDITY", "bits",
			xmmstimid_cfg.bits);
	xmms_cfg_write_int(cf, "TIMIDITY", "channels",
			xmmstimid_cfg.channels);

	xmms_cfg_write_file(cf, filename);
	xmms_cfg_free(cf);
	g_free(filename);

	gtk_widget_hide(xmmstimid_conf_wnd);
}

int xmmstimid_is_our_file(char *filename) {
	gchar *ext;

	ext = strrchr(filename, '.');
	if (ext && !(
	    strcasecmp(ext, ".mid") &&
	    strcasecmp(ext, ".midi"))) return 1;

	return 0;
}

static void *xmmstimid_play_loop(void *arg) {
	size_t buffer_size;
	void *buffer;
	size_t bytes_read;
	AFormat fmt;

	buffer_size = ((xmmstimid_opts.format == MID_AUDIO_S16LSB) ? 16 : 8) * 
			xmmstimid_opts.channels / 8 *
			xmmstimid_opts.buffer_size;

	buffer = g_malloc(buffer_size);
	if (buffer == NULL) pthread_exit(NULL);

	fmt = (xmmstimid_opts.format == MID_AUDIO_S16LSB) ? FMT_S16_LE : FMT_S8;

	while (xmmstimid_going) {
		bytes_read = mid_song_read_wave(xmmstimid_song,
				buffer, buffer_size);

		if (bytes_read != 0)
			xmmstimid_ip.add_vis_pcm(
					mid_song_get_time(xmmstimid_song),
					fmt, xmmstimid_opts.channels,
					bytes_read, buffer);
		else xmmstimid_eof = TRUE;

		while (xmmstimid_going && xmmstimid_seek_to == -1 &&
				(bytes_read == 0 ||
				 xmmstimid_ip.output->buffer_free() < bytes_read))
			xmms_usleep(10000);

		if (xmmstimid_seek_to != -1) {
			mid_song_seek(xmmstimid_song, xmmstimid_seek_to * 1000);
			xmmstimid_ip.output->flush(xmmstimid_seek_to * 1000);
			xmmstimid_seek_to = -1;
			bytes_read = 0;
		}
		
		if (xmmstimid_going && bytes_read != 0)
			xmmstimid_ip.output->write_audio(buffer, bytes_read);
	}

	g_free(buffer);

	pthread_exit(NULL);
}

static gchar *xmmstimid_get_title(gchar *filename) {
	TitleInput *input;
	gchar *temp, *ext, *title, *path, *temp2;

	XMMS_NEW_TITLEINPUT(input);

	path = g_strdup(filename);
	temp = g_strdup(filename);
	ext = strrchr(temp, '.');
	if (ext)
		*ext = '\0';
	temp2 = strrchr(path, '/');
	if (temp2)
		*temp2 = '\0';

	input->file_name = g_basename(filename);
	input->file_ext = ext ? ext+1 : NULL;
	input->file_path = g_strdup_printf("%s/", path);

	title = xmms_get_titlestring(xmms_get_gentitle_format(), input);
	if (title == NULL)
		title = g_strdup(input->file_name);

	g_free(temp);
	g_free(path);
	g_free(input);

	return title;
}

void xmmstimid_play_file(char *filename) {
	MidIStream *stream;
	gchar *title;

	if (!xmmstimid_initialized) {
		xmmstimid_init();
		if (!xmmstimid_initialized) return;
	}

	if (xmmstimid_song != NULL) {
		mid_song_free(xmmstimid_song);
		xmmstimid_song = NULL;
	}

	stream = mid_istream_open_file(filename);
	if (stream == NULL) return;

	xmmstimid_audio_error = FALSE;

	xmmstimid_opts.rate = xmmstimid_cfg.rate;
	xmmstimid_opts.format = (xmmstimid_cfg.bits == 16) ?
		MID_AUDIO_S16LSB : MID_AUDIO_S8;
	xmmstimid_opts.channels = xmmstimid_cfg.channels;
	xmmstimid_opts.buffer_size = xmmstimid_cfg.buffer_size;

	xmmstimid_song = mid_song_load(stream, &xmmstimid_opts);
	mid_istream_close(stream);

	if (xmmstimid_song == NULL) {
		xmmstimid_ip.set_info_text("Couldn't load MIDI file");
		return;
	}

	if (xmmstimid_ip.output->open_audio(
				(xmmstimid_opts.format == MID_AUDIO_S16LSB) ?
				FMT_S16_LE : FMT_S8,
				xmmstimid_opts.rate, xmmstimid_opts.channels) == 0) {
		xmmstimid_audio_error = TRUE;
		mid_song_free(xmmstimid_song);
		xmmstimid_song = NULL;
		return;
	}

	title = xmmstimid_get_title(filename);
	xmmstimid_ip.set_info(title,
			mid_song_get_total_time(xmmstimid_song),
			0, xmmstimid_opts.rate, xmmstimid_opts.channels);
	g_free(title);

	mid_song_start(xmmstimid_song);
	xmmstimid_going = TRUE;
	xmmstimid_eof = FALSE;
	xmmstimid_seek_to = -1;

	if (pthread_create(&xmmstimid_decode_thread,
				NULL, xmmstimid_play_loop, NULL) != 0) {
		mid_song_free(xmmstimid_song);
		xmmstimid_stop();
	}
}

void xmmstimid_stop(void) {
	if (xmmstimid_song != NULL && xmmstimid_going) {
		xmmstimid_going = FALSE;
		pthread_join(xmmstimid_decode_thread, NULL);
		xmmstimid_ip.output->close_audio();
		mid_song_free(xmmstimid_song);
		xmmstimid_song = NULL;
	}
}

void xmmstimid_pause(short p) {
	xmmstimid_ip.output->pause(p);
}

void xmmstimid_seek(int time) {
	xmmstimid_seek_to = time;
	xmmstimid_eof = FALSE;

	while (xmmstimid_seek_to != -1)
		xmms_usleep(10000);
}

int xmmstimid_get_time(void) {
	if (xmmstimid_audio_error)
		return -2;
	if (xmmstimid_song == NULL)
		return -1;
	if (!xmmstimid_going || (xmmstimid_eof &&
				xmmstimid_ip.output->buffer_playing()))
		return -1;

	return mid_song_get_time(xmmstimid_song);
}

void xmmstimid_cleanup(void) {
	if (xmmstimid_initialized)
		mid_exit();
}

void xmmstimid_get_song_info(char *filename, char **title, int *length) {
	MidIStream *stream;
	MidSongOptions opts;
	MidSong *song;

	if (!xmmstimid_initialized) {
		xmmstimid_init();
		if (!xmmstimid_initialized) return;
	}

	stream = mid_istream_open_file(filename);
	if (stream == NULL) return;

	opts.rate = xmmstimid_cfg.rate;
	opts.format = (xmmstimid_cfg.bits == 16) ?
		MID_AUDIO_S16LSB : MID_AUDIO_S8;
	opts.channels = xmmstimid_cfg.channels;
	opts.buffer_size = 8;

	song = mid_song_load(stream, &opts);
	mid_istream_close(stream);

	if (song == NULL) return;

	*length = mid_song_get_total_time(song);
	*title = xmmstimid_get_title(filename);

	mid_song_free(song);
}

