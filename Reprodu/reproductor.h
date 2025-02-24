#ifndef REPRODUCTOR_H
#define REPRODUCTOR_H

#include <gtk/gtk.h>
#include <vector>
#include <string>
#include <thread>

void on_activate(GtkApplication *app, gpointer user_data);
void on_play_button_clicked(GtkButton *button, gpointer user_data);
void on_pause_button_clicked(GtkButton *button, gpointer user_data);
void on_next_button_clicked(GtkButton *button, gpointer user_data);
void on_prev_button_clicked(GtkButton *button, gpointer user_data);
void on_add_song_button_clicked(GtkButton *button, gpointer user_data);
void play_next_song();
void add_song_to_playlist_view(const char* song);

extern std::vector<std::string> playlist;
extern size_t current_song_index;
extern bool is_playing;
extern std::thread play_thread;

#endif // REPRODUCTOR_H
