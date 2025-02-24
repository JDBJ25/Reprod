#include "reproductor.h"
#include <iostream>
#include <fstream>
#include <mpg123.h>
#include <ao/ao.h>
#include <thread>
#include <mutex>

std::vector<std::string> playlist;
size_t current_song_index = 0;
bool is_playing = false;
bool should_stop = false;
mpg123_handle *mh = nullptr;
ao_device *dev = nullptr;
GtkListStore *store = nullptr;
GtkTreeIter iter;
std::thread play_thread;
std::mutex play_mutex;

void on_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *control_box;
    GtkWidget *play_button;
    GtkWidget *pause_button;
    GtkWidget *next_button;
    GtkWidget *prev_button;
    GtkWidget *add_song_button;
    GtkWidget *scroll_window;
    GtkWidget *tree_view;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Reproductor de Música");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);

    // Aplicar CSS
    GtkCssProvider *cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider,
        "window {"
        "   background-color: #2E2E2E;"
        "}"
        "treeview {"
        "   background-color: #1C1C1C;"
        "   color: #FFFFFF;"
        "}"
        "button {"
        "   background-color: #3E3E3E;"
        "   color: #FFFFFF;"
        "}", -1, NULL);

    GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(window));
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    store = gtk_list_store_new(1, G_TYPE_STRING);

    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Canciones", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
    scroll_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll_window), tree_view);
    gtk_box_pack_start(GTK_BOX(box), scroll_window, TRUE, TRUE, 0);

    control_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(box), control_box, FALSE, FALSE, 0);

    play_button = gtk_button_new_with_label("Reproducir");
    g_signal_connect(play_button, "clicked", G_CALLBACK(on_play_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), play_button, FALSE, FALSE, 0);

    pause_button = gtk_button_new_with_label("Pausar");
    g_signal_connect(pause_button, "clicked", G_CALLBACK(on_pause_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), pause_button, FALSE, FALSE, 0);

    prev_button = gtk_button_new_with_label("Anterior");
    g_signal_connect(prev_button, "clicked", G_CALLBACK(on_prev_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), prev_button, FALSE, FALSE, 0);

    next_button = gtk_button_new_with_label("Siguiente");
    g_signal_connect(next_button, "clicked", G_CALLBACK(on_next_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), next_button, FALSE, FALSE, 0);

    add_song_button = gtk_button_new_with_label("Agregar Canción");
    g_signal_connect(add_song_button, "clicked", G_CALLBACK(on_add_song_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(box), add_song_button, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
}

void on_play_button_clicked(GtkButton *button, gpointer user_data) {
    std::cout << "Reproducir botón presionado" << std::endl;
    if (!playlist.empty() && !is_playing) {
        is_playing = true;
        should_stop = false;
        play_thread = std::thread(play_next_song);
    }
}

void on_pause_button_clicked(GtkButton *button, gpointer user_data) {
    std::cout << "Pausar botón presionado" << std::endl;
    if (is_playing) {
        is_playing = false;
        should_stop = true;
        if (play_thread.joinable()) {
            play_thread.join();
        }
        // Aquí podrías agregar la lógica para pausar la música, si es necesario.
    }
}

void on_next_button_clicked(GtkButton *button, gpointer user_data) {
    std::cout << "Siguiente botón presionado" << std::endl;
    if (!playlist.empty()) {
        current_song_index = (current_song_index + 1) % playlist.size();
        if (is_playing) {
            should_stop = true;
            if (play_thread.joinable()) {
                play_thread.join();
            }
            should_stop = false;
            play_thread = std::thread(play_next_song);
        }
    }
}

void on_prev_button_clicked(GtkButton *button, gpointer user_data) {
    std::cout << "Anterior botón presionado" << std::endl;
    if (!playlist.empty()) {
        if (current_song_index == 0) {
            current_song_index = playlist.size() - 1;
        } else {
            current_song_index--;
        }
        if (is_playing) {
            should_stop = true;
            if (play_thread.joinable()) {
                play_thread.join();
            }
            should_stop = false;
            play_thread = std::thread(play_next_song);
        }
    }
}

void on_add_song_button_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new("Abrir Archivo",
                                         NULL,
                                         action,
                                         "_Cancelar",
                                         GTK_RESPONSE_CANCEL,
                                         "_Abrir",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
        playlist.push_back(filename);

        // Extraer el nombre del archivo de la ruta completa
        std::string filepath(filename);
        std::string song_name = filepath.substr(filepath.find_last_of("/\\") + 1);
        add_song_to_playlist_view(song_name.c_str());

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void add_song_to_playlist_view(const char* song) {
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, song, -1);
}

void play_next_song() {
    while (current_song_index < playlist.size() && is_playing) {
        const std::string &filepath = playlist[current_song_index];
        std::cout << "Reproduciendo: " << filepath << std::endl;

        unsigned char *buffer;
        size_t buffer_size;
        size_t done;
        int err;

        int driver;

        ao_sample_format format;
        int channels, encoding;
        long rate;

        /* Inicializar mpg123 */
        mpg123_init();
        mh = mpg123_new(NULL, &err);
        buffer_size = mpg123_outblock(mh);
        buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

        /* Inicializar ao */
        ao_initialize();
        driver = ao_default_driver_id();

        /* Abrir archivo MP3 */
        mpg123_open(mh, filepath.c_str());
        mpg123_getformat(mh, &rate, &channels, &encoding);

        /* Configurar formato de salida */
        format.bits = mpg123_encsize(encoding) * 8;
        format.rate = rate;
        format.channels = channels;
        format.byte_format = AO_FMT_NATIVE;
        format.matrix = 0;

        /* Abrir dispositivo de salida */
        dev = ao_open_live(driver, &format, NULL);

        /* Reproducir archivo MP3 */
        while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK) {
            {
                std::lock_guard<std::mutex> lock(play_mutex);
                if (should_stop) {
                    break;
                }
            }
            ao_play(dev, (char*) buffer, done);
        }

        /* Limpiar */
        ao_close(dev);
        free(buffer);
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        ao_shutdown();

        /* Reproducir la siguiente canción */
        {
            std::lock_guard<std::mutex> lock(play_mutex);
            if (should_stop) {
                break;
            }
            current_song_index++;
            if (current_song_index >= playlist.size()) {
                is_playing = false;
                current_song_index = 0; // Reiniciar el índice para la próxima reproducción
            }
        }
    }
}
