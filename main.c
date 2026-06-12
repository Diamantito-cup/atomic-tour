#include <adwaita.h>
#include <glib.h>
#include <glib/gstdio.h>

#define TOTAL_PAGES 5

typedef struct {
    GtkWidget *window;
    GtkWidget *carousel;
    GtkWidget *dots;
    GtkWidget *next_btn;
    GtkWidget *pages[TOTAL_PAGES];
    int total_pages;
} TourData;

// ============================================================================
// 1. CREADORES DE MULTIMEDIA
// ============================================================================

GtkWidget* create_image_page(const char *file_path) {
    GtkWidget *pic = gtk_picture_new_for_filename(file_path);
    gtk_picture_set_content_fit(GTK_PICTURE(pic), GTK_CONTENT_FIT_CONTAIN);
    gtk_widget_set_vexpand(pic, TRUE);
    gtk_widget_set_size_request(pic, -1, 260); 
    return pic;
}

GtkWidget* create_video_page(const char *file_path) {
    GFile *file = g_file_new_for_path(file_path);
    GtkMediaStream *stream = gtk_media_file_new_for_file(file);
    g_object_unref(file);
    
    gtk_media_stream_set_loop(stream, TRUE);
    gtk_media_stream_play(stream);
    
    GtkWidget *pic = gtk_picture_new_for_paintable(GDK_PAINTABLE(stream));
    gtk_picture_set_content_fit(GTK_PICTURE(pic), GTK_CONTENT_FIT_CONTAIN);
    gtk_widget_set_size_request(pic, -1, 260); 
    gtk_widget_set_halign(pic, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(pic, "card");
    
    return pic;
}

// ============================================================================
// 2. ENSAMBLADOR DE PÁGINAS
// ============================================================================
GtkWidget* create_tour_page(const char *file_path, const char *title, const char *desc) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_margin_start(box, 80);
    gtk_widget_set_margin_end(box, 80);
    gtk_widget_set_margin_top(box, 40); 
    gtk_widget_set_margin_bottom(box, 40);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    
    GtkWidget *media;

    if (g_str_has_suffix(file_path, ".png") || g_str_has_suffix(file_path, ".jpg")) {
        GFile *file = g_file_new_for_path(file_path);
        media = gtk_picture_new_for_file(file);
        g_object_unref(file);
        gtk_picture_set_content_fit(GTK_PICTURE(media), GTK_CONTENT_FIT_CONTAIN);
    } else {
        media = create_video_page(file_path); 
    }
    
    gtk_widget_set_size_request(media, 420, 240); 
    gtk_widget_add_css_class(media, "card"); 
    
    GtkWidget *title_label = gtk_label_new(title);
    gtk_widget_add_css_class(title_label, "title-1");
    gtk_label_set_justify(GTK_LABEL(title_label), GTK_JUSTIFY_CENTER);
    
    GtkWidget *desc_label = gtk_label_new(desc);
    gtk_label_set_wrap(GTK_LABEL(desc_label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(desc_label), 55); 
    gtk_label_set_justify(GTK_LABEL(desc_label), GTK_JUSTIFY_CENTER); 
    gtk_widget_add_css_class(desc_label, "body-text");

    gtk_box_append(GTK_BOX(box), media);
    gtk_box_append(GTK_BOX(box), title_label);
    gtk_box_append(GTK_BOX(box), desc_label);
    
    return box;
}

// ============================================================================
// 3. LÓGICA DE NAVEGACIÓN Y PRIMER INICIO
// ============================================================================

void mark_as_done() {
    // Crear un archivo oculto en ~/.config para indicar que ya se corrió
    const char *config_dir = g_get_user_config_dir();
    char *done_file = g_build_filename(config_dir, "atomic-tour-done", NULL);
    
    // Si no existe el archivo, lo creamos
    if (!g_file_test(done_file, G_FILE_TEST_EXISTS)) {
        FILE *f = fopen(done_file, "w");
        if (f) {
            fprintf(f, "tour_completed=1\n");
            fclose(f);
        }
    }
    g_free(done_file);
}

static void on_position_changed(GObject *object, GParamSpec *pspec, gpointer user_data) {
    TourData *data = (TourData*)user_data;
    double position = adw_carousel_get_position(ADW_CAROUSEL(object));
    int current_page = (int)(position + 0.5); 
    
    if (current_page == data->total_pages - 1) {
        gtk_button_set_label(GTK_BUTTON(data->next_btn), "Finalizar");
        gtk_widget_add_css_class(data->next_btn, "suggested-action"); 
    } else {
        gtk_button_set_label(GTK_BUTTON(data->next_btn), "Siguiente");
        gtk_widget_remove_css_class(data->next_btn, "suggested-action");
    }
}

static void on_next_clicked(GtkButton *btn, gpointer user_data) {
    TourData *data = (TourData*)user_data;
    double position = adw_carousel_get_position(ADW_CAROUSEL(data->carousel));
    int current_page = (int)(position + 0.5);
    
    if (current_page < data->total_pages - 1) {
        adw_carousel_scroll_to(ADW_CAROUSEL(data->carousel), data->pages[current_page + 1], TRUE);
    } else {
        mark_as_done(); // Marcar como completado al finalizar
        g_application_quit(g_application_get_default());
    }
}

// ============================================================================
// 4. INICIALIZACIÓN DE LA APLICACIÓN
// ============================================================================

static void on_activate(GtkApplication *app) {
    // 0. VERIFICAR SI YA SE EJECUTÓ ANTES
    const char *config_dir = g_get_user_config_dir();
    char *done_file = g_build_filename(config_dir, "atomic-tour-done", NULL);
    if (g_file_test(done_file, G_FILE_TEST_EXISTS)) {
        g_free(done_file);
        g_application_quit(G_APPLICATION(app));
        return;
    }
    g_free(done_file);

    TourData *data = g_new0(TourData, 1);
    data->total_pages = TOTAL_PAGES;
    data->window = adw_application_window_new(app);
    
    adw_style_manager_set_color_scheme(adw_style_manager_get_default(), ADW_COLOR_SCHEME_FORCE_DARK);
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "window { background-color: #050505; }" 
        ".title-1 { font-size: 32px; font-weight: bold; color: #3584e4; margin-bottom: 10px; }"
        ".body-text { color: #cccccc; font-size: 16px; }"
        ".pill { "
        "  background-color: #3584e4; "
        "  color: white; "
        "  border-radius: 99px; "
        "  padding: 12px 36px; "
        "  border: none; "
        "  font-weight: bold; "
        "  transition: all 200ms ease-in-out; "
        "}"
        ".pill:hover { background-color: #1c71d8; }"
        "carouselindicator { margin-bottom: 16px; }"
    );

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        
    gtk_window_set_title(GTK_WINDOW(data->window), "Welcome To The Atomic Linux Experience");
    gtk_window_set_default_size(GTK_WINDOW(data->window), 650, 500);
    
    // Si quieres esconder la barra de título (headerbar) para que se parezca más a GNOME Tour:
    // GtkWidget *header_bar = adw_header_bar_new();
    // gtk_widget_set_visible(header_bar, FALSE);
    
    data->carousel = adw_carousel_new();
    
    data->pages[0] = create_tour_page("assets/logo.png", 
        "¡Bienvenido a Atomic Linux!", 
        "Un sistema operativo de nueva generación, inmutable, rápido y diseñado a tu medida.");
        
    data->pages[1] = create_tour_page("assets/example-of-brain_shell.mp4", 
        "Entorno Brain_Shell", 
        "Gestión inteligente de espacios de trabajo dinámicos y paneles fluidos, nacido del desarrollo con herramientas QuickShell de última generación.");
        
    data->pages[2] = create_tour_page("assets/ejemplo-tiling.mp4", 
        "Navegación Tiling Eficiente", 
        "Controla todo tu flujo desde el teclado usando atajos rápidos. Super(Windows) + Q abre una terminal Kitty, Super + C cierra una ventana, Super + V convierte una ventana de mosaico en ventana normal, Super + Click Derecho redimensiona una ventana no-mosaico y Super + Click Izquierdo mueve cualquier tipo de ventana");
        
    data->pages[3] = create_tour_page("assets/fixing.mp4", 
        "Ecosistema Atomic", 
        "Siéntete libre de hacer lo que quieras en este lugar. Gracias a BTRFS y TimeShift tienes un entorno seguro y limpio, con snapshots automáticas tras cada actualización.");
        
    data->pages[4] = create_tour_page("assets/finished.mp4", 
        "¡Todo listo!", 
        "Cierra este asistente o presiona finalizar para comenzar tu nueva experiencia informática y ver el escritorio vivo.");

    for (int i = 0; i < TOTAL_PAGES; i++) {
        adw_carousel_append(ADW_CAROUSEL(data->carousel), data->pages[i]);
    }

    // INDICADORES DEL CARRUSEL (Puntitos estilo GNOME Tour)
    data->dots = adw_carousel_indicator_dots_new();
    adw_carousel_indicator_dots_set_carousel(ADW_CAROUSEL_INDICATOR_DOTS(data->dots), ADW_CAROUSEL(data->carousel));
    gtk_widget_set_halign(data->dots, GTK_ALIGN_CENTER);

    data->next_btn = gtk_button_new_with_label("Siguiente");
    gtk_widget_add_css_class(data->next_btn, "pill"); 
    gtk_widget_set_margin_bottom(data->next_btn, 30);
    gtk_widget_set_halign(data->next_btn, GTK_ALIGN_CENTER);
    
    g_signal_connect(data->next_btn, "clicked", G_CALLBACK(on_next_clicked), data);
    g_signal_connect(data->carousel, "notify::position", G_CALLBACK(on_position_changed), data);

    // Caja maestra
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(data->carousel, TRUE); 
    
    gtk_box_append(GTK_BOX(main_box), data->carousel);
    gtk_box_append(GTK_BOX(main_box), data->dots); // Añadir los puntitos debajo del carrusel
    gtk_box_append(GTK_BOX(main_box), data->next_btn);

    // Conectar el evento de cierre de ventana para asegurarnos de que se marque como visto
    // si el usuario cierra con la 'X'
    g_signal_connect_swapped(data->window, "close-request", G_CALLBACK(mark_as_done), NULL);

    adw_application_window_set_content(ADW_APPLICATION_WINDOW(data->window), main_box);
    gtk_window_present(GTK_WINDOW(data->window));
}

int main(int argc, char *argv[]) {
    AdwApplication *app = adw_application_new("org.atomic.tour", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app); 
    return status;
}
