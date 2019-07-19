/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Author: Dariem Pérez Herrera (dariemp [at] uci.cu)
 *         Proyecto Nova, Centro de Soluciones Libres
 *         Universidad de las Ciencias Informáticas
 *
 * Este código es una prueba de concepto con fines educativos, se priorizó en
 * la claridad por encima de las "mejores prácticas". Espero le sea útil ;-)
 */

#include <gio/gio.h>
#include <glib/gprintf.h>

GMainLoop *loop;
GDBusConnection *system_bus;
GDBusConnection *session_bus;
GHashTable *already_scanned;

// Forward declarations
void connect_to_udisks();
void onDeviceChanged(GDBusConnection *connection,
                            const gchar *sender_name,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *signal_name,
                            GVariant *parameters,
                            gpointer user_data);
void onDeviceRemoved(GDBusConnection *connection,
                            const gchar *sender_name,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *signal_name,
                            GVariant *parameters,
                            gpointer user_data);
gchar* get_device_mount_path(gchar* device_object_path);
GList* get_playlist(gchar* directory_path);
void play(GList *playlist);


void init_dbus()
{
    // Se inicializa el sistema de tipos de Glib
    g_type_init();

    /* Se inicializa un GHashTable para controlar la repetición de escaneos
     * innecesarios a los dispositivos   */
    already_scanned = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    // Se obtiene una conexión al bus de sistema para usar con UDisks
    system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);

    // Se obtiene una conexión al bus de sesión para usar con Rhythmbox
    session_bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

    // Conectar a las señales que emite UDisks
    connect_to_udisks();
}

void connect_to_udisks()
{
    // Información de D-Bus para UDisks
    gchar* udisks_bus_name = "org.freedesktop.UDisks";
    gchar* udisks_object_path = "/org/freedesktop/UDisks";
    gchar* udisks_dbus_interface = "org.freedesktop.UDisks";
    

    /* Se conecta la señal "DeviceChanged" exportada por el objeto remoto
     * a la función local "onDeviceChanged" */
    g_dbus_connection_signal_subscribe(system_bus,
                                       udisks_bus_name,
                                       udisks_dbus_interface,
                                       "DeviceChanged",
                                       udisks_object_path,
                                       NULL, 0,
                                       (GDBusSignalCallback)onDeviceChanged,
                                       NULL, NULL);

    /* Se conecta la señal "DeviceRemoved" exportada por el objeto remoto
     * a la función local "onDeviceRemoved" */
    g_dbus_connection_signal_subscribe(system_bus,
                                       udisks_bus_name,
                                       udisks_dbus_interface,
                                       "DeviceRemoved",
                                       udisks_object_path,
                                       NULL, 0,
                                       (GDBusSignalCallback)onDeviceRemoved,
                                       NULL, NULL);
}

void onDeviceChanged(GDBusConnection *connection,
                            const gchar *sender_name,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *signal_name,
                            GVariant *parameters,
                            gpointer user_data)
{
    /* La señal 'DeviceChanged' es emitida cuando se cambia el estado de un
     * dispositivo (por ejemplo, cuando se monta o desmonta). UDisks envía
     * el camino del objeto remoto del dispositivo que cambió de estado. */
    gchar* device_object_path;
    g_variant_get(parameters, "(o)", &device_object_path);
    g_printf("Un dispositivo cambió: %s\n", device_object_path);
    fflush(stdout);
    gchar* mount_path = get_device_mount_path(device_object_path);
    // Si no está montado o ya fue escaneado, no hacer nada y retornar
    if (!mount_path)
        return;
    GList* playlist = get_playlist(mount_path);
    // Si no hay música en el dispositivo, no hacer nada y retornar
    if (!playlist) {
        return;
    }
    play(playlist);
}

void onDeviceRemoved(GDBusConnection *connection,
                            const gchar *sender_name,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *signal_name,
                            GVariant *parameters,
                            gpointer user_data)
{
    /* La señal 'DeviceRemoved' es emitida cuando un dispositivo es
     * desconectado por hardware de la PC. UDisks envía el camino del
     * objeto remoto del dispositivo.  */
    gchar* device_object_path;
    g_variant_get(parameters, "(o)", &device_object_path);
    g_printf("Un dispositivo fue desconectado: %s\n", device_object_path);
    fflush(stdout);
    if (g_hash_table_contains(already_scanned, device_object_path))
        g_hash_table_remove(already_scanned, device_object_path);
}

gchar* get_device_mount_path(gchar* device_object_path)
{
    // Información necesaria para procesar los dispositivos de UDisks
    gchar* udisks_bus_name = "org.freedesktop.UDisks";
    gchar* device_dbus_interface = "org.freedesktop.UDisks.Device";

    /* Se obtiene el objeto remoto del dispositivo insertado a través
     * de la conexión al bus de sistema del demonio UDisks. Este ya viene
     * enlazado con la interfaz "org.freedesktop.DBus.Properties" lo cual
     * permite acceder a sus propiedades */
    GDBusProxy *device = g_dbus_proxy_new_sync(system_bus,
                                                     0, NULL,
                                                     udisks_bus_name,
                                                     device_object_path,
                                                     device_dbus_interface,
                                                     NULL, NULL);

    /* La API GDBus devuelve todo lo que viene desde el objeto remoto
     * encapsulado en un tipo GVariant, lo cual implica un procesamiento
     * extra para extraer los valores con sus tipos de datos reales */

    /* La propiedad 'DeviceFile' devuelve el archivo de dispositivo en /dev
     * En nuestro caso no lo necesitamos, se pone a modo de enseñanza.  */
    GVariant *property_container = g_dbus_proxy_get_cached_property(device, "DeviceFile");
    const gchar* device_file = g_variant_get_string(property_container, NULL);
    g_variant_unref(property_container);

    // La propiedad 'DeviceIsMounted' devuelve si está montado
    property_container = g_dbus_proxy_get_cached_property(device, "DeviceIsMounted");
    gboolean is_mounted = g_variant_get_boolean(property_container);
    g_variant_unref(property_container);

    g_printf("¿Está montado %s? : %s\n", device_file, (is_mounted ? "True": "False"));
    fflush(stdout);

    /* Si el dispositivo ya fue escaneado y no está montado, significa que
     * recién se desmontó, hay que olvidarlo. */
    gchar* mount_path = NULL;
    if (g_hash_table_contains(already_scanned, device_object_path)) {
            if (!is_mounted)
                g_hash_table_remove(already_scanned, device_object_path);
    }
    /* Si no ha sido escaneado y está montado, significa que recién se
     * ha montado y hay que escanearlo y recordarlo, se devuelve el punto
     * de montaje. En otros casos, no se hace nada y no se devuelve nada. */
    else if(is_mounted) {
        g_hash_table_insert(already_scanned, device_object_path, NULL);
        /* La propiedad 'DeviceMountPaths' devuelve todos los directorios
         * del sistema de archivos donde se ha montado este dispositivo. */
        property_container = g_dbus_proxy_get_cached_property(device, "DeviceMountPaths");
        GVariantIter *iter;
        g_variant_get(property_container, "as", &iter);
        g_variant_iter_loop(iter, "s", &mount_path);
        g_variant_iter_free(iter);
        g_variant_unref(property_container);
    }
    return mount_path;
}

GList *get_playlist(gchar* directory_path)
{
    g_printf("Buscando música en el directorio %s\n", directory_path);
    fflush(stdout);
    GList *playlist = NULL;
    GFile* directory = g_file_new_for_path(directory_path);
    GFileEnumerator* children_enum = g_file_enumerate_children(
                directory,
                "standard::name,standard::type,standard::content-type",
                0, NULL, NULL);
    GFileInfo* file_info;
    while (file_info = g_file_enumerator_next_file(children_enum, NULL, NULL)) {
        if (g_file_info_get_file_type(file_info) != G_FILE_TYPE_DIRECTORY) {
            const gchar* mime_type = g_file_info_get_content_type(file_info);
            gchar** tokens = g_strsplit(mime_type, "/", 2);
            gchar* category = tokens[0];
            if (!g_strcmp0(category, "audio")) {
                const char *filename = g_file_info_get_name(file_info);
                char *filepath = g_strconcat(directory_path, "/", filename, NULL);
                gchar *uri = g_filename_to_uri(filepath, NULL, NULL);
                playlist = g_list_prepend(playlist, uri);
                g_free(filepath);
            }
            g_strfreev(tokens);
        }
        g_object_unref(file_info);
    }
    g_object_unref(children_enum);
    g_object_unref(directory);
    return playlist;
}

GDBusProxy *get_rhythmbox_playlist_manager()
{
    // Para modificar la lista de reproducción no hay MPRIS que valga :-(

    // Información de D-Bus para el Playlist Manager de Rhythmbox
    gchar* rhythmbox_bus_name = "org.gnome.Rhythmbox3";
    gchar* playlist_mgr_object_path = "/org/gnome/Rhythmbox3/PlaylistManager";
    gchar* playlist_mgr_dbus_interface = "org.gnome.Rhythmbox3.PlaylistManager";

    // Se obtiene el objeto proxy del Playlist Manager con su interfaz por defecto
    return g_dbus_proxy_new_sync(session_bus,
                                 0, NULL,
                                 rhythmbox_bus_name,
                                 playlist_mgr_object_path,
                                 playlist_mgr_dbus_interface,
                                 NULL, NULL);
}

GDBusProxy* get_rhythmbox_player()
{
    /* Para controlar el reproductor, usaremos la interfaz estándar que
     * define FreeDestkop.org, es decir MPRIS */
    gchar* rhythmbox_bus_name = "org.mpris.MediaPlayer2.rhythmbox";
    gchar* rhythmbox_object_path = "/org/mpris/MediaPlayer2";
    gchar* rhythmbox_dbus_interface_player = "org.mpris.MediaPlayer2.Player";

    // Se obtiene el objeto proxy Player con su interfaz por defecto
    return g_dbus_proxy_new_sync(session_bus,
                                 0, NULL,
                                 rhythmbox_bus_name,
                                 rhythmbox_object_path,
                                 rhythmbox_dbus_interface_player,
                                 NULL, NULL);
}


GDBusProxy* get_rhythmbox_playlists()
{
    /* Se obtiene la interfaz Playerlists definida por MPRIS necesaria para
     * establecer la lista de reproducción actual antes de reproducir */
    gchar* rhythmbox_bus_name = "org.mpris.MediaPlayer2.rhythmbox";
    gchar* rhythmbox_object_path = "/org/mpris/MediaPlayer2";
    gchar* rhythmbox_dbus_interface_playlists = "org.mpris.MediaPlayer2.Playlists";

    // Se obtiene el objeto proxy Playlists con su interfaz por defecto
    return g_dbus_proxy_new_sync(session_bus,
                                 0, NULL,
                                 rhythmbox_bus_name,
                                 rhythmbox_object_path,
                                 rhythmbox_dbus_interface_playlists,
                                 NULL, NULL);
}


void play(GList *playlist)
{
    /* Nombre estático para la lista de reproducción,
     * se llamará siempre "Autoplayer"  */
    gchar* playlist_name = "Autoplayer";

    // Obtener el Playlist Manager de Rhythmbox
    GDBusProxy* playlist_manager = get_rhythmbox_playlist_manager();

    // Borrar la lista de reproducción si ya existe
    g_dbus_proxy_call_sync(playlist_manager, "DeletePlaylist", g_variant_new("(s)", playlist_name), 0, -1, NULL, NULL);

    // Crear una lista de reproducción nueva
    g_dbus_proxy_call_sync(playlist_manager, "CreatePlaylist", g_variant_new("(s)", playlist_name), 0, -1, NULL, NULL);

    /* Se adiciona cada archivo de música obtenido del dispositvo
     * de almacenamiento a la nueva lista de reproducción. */
    gint length = g_list_length(playlist);
    GList* cursor = playlist;
    int i;
    for (i=0; i < length; i++) {
        g_dbus_proxy_call_sync(playlist_manager, "AddToPlaylist", g_variant_new("(ss)", playlist_name, cursor->data), 0, -1, NULL, NULL);
        cursor = g_list_next(cursor);
    }


    // Ya se puede liberar el objeto proxy del Playlist Manager y la lista de reproducción
    g_object_unref(playlist_manager);
    g_list_free_full(playlist, g_free);

    /* Esperar a que levante Rhythmbox... sí, ok, ya sé que esto es una
     * mala práctica. Lo correcto sería conectar a alguna señal remota que
     * avise de que está listo el reproductor, pero mejor no complicarse :-P */
    sleep(5);

    // Obtener la interfaz de listas de reproducción definida por MPRIS
    GDBusProxy* playlists_interface = get_rhythmbox_playlists();
    GVariant* ret_val = g_dbus_proxy_get_cached_property(playlists_interface, "PlaylistCount");
    guint playlists_count;
    g_variant_get(ret_val, "u", &playlists_count);
    g_variant_unref(ret_val);

    /* En un mundo ideal, Rhythmbox sería buenito y ordenaría las listas de
     * reproducción por fecha de creación en orden reverso como indica el
     * API de MPRIS, pero desgraciadamente no soporta esta funcionalidad,
     * así que hay que buscar a lo bestia... :-P :-(  */
    ret_val = g_dbus_proxy_call_sync(playlists_interface,
                                     "GetPlaylists",
                                     g_variant_new("(uusb)", 0, playlists_count, "CreationDate", TRUE),
                                     0, -1, NULL, NULL);

    // Buscando nuestra lista de reproducción a lo bestia... :-P
    struct
    {
        gchar* Id;
        gchar* Name;
        gchar* IconUri;
    } pl;
    GVariantIter *iter;
    g_variant_get(ret_val, "(a(oss))", &iter);
    gboolean not_found = TRUE;
    while (not_found && g_variant_iter_loop (iter, "(&o&s&s)", &pl.Id, &pl.Name, &pl.IconUri))
        not_found = g_strcmp0(pl.Name, playlist_name);
    gchar* playlist_id = g_strdup(pl.Id);
    g_variant_iter_free(iter);
    g_variant_unref(ret_val);
    if (not_found) {
        g_fprintf(stderr, "Rhythmbox no cargó la lista correctamente.\n");
        return;
    }

    /* Si se llega aquí, es que se encontró nuestra lista de reproducción */
    // Obtener el reproductor
    GDBusProxy* rhythmbox_player = get_rhythmbox_player();

    // Detenemos cualquier reproducción previa para evitar mareadera :-P
    g_dbus_proxy_call_sync(rhythmbox_player, "Stop", NULL, 0, -1, NULL, NULL);

    // Activar la lista de reproducción como lista de reproducción actual
    g_dbus_proxy_call_sync(playlists_interface, "ActivatePlaylist", g_variant_new("(o)", playlist_id), 0, -1, NULL, NULL);


    // Ya se puede liberar la interfaz Playlists de MPRIS
    g_object_unref(playlists_interface);

    // Reproducir... :-)
    g_dbus_proxy_call_sync(rhythmbox_player, "Play", NULL, 0, -1, NULL, NULL);

    // Liberar el objeto proxy del reproductor
    g_object_unref(rhythmbox_player);
}

void free_dbus()
{
    // Liberar la memoria que se ha reservado dinámicamente
    g_object_unref(system_bus);
    g_object_unref(session_bus);
    g_hash_table_destroy(already_scanned);
}

int main(void)
{
    // Crear todas las conexiones con D-Bus y conectar a las señales UDisks
    init_dbus();

    // Crear el ciclo de eventos de Glib
    loop = g_main_loop_new(NULL, FALSE);

    // Poner a correr el ciclo de eventos, donde se procesarán los mensajes
    g_main_loop_run(loop);

    // Liberar toda la memoria reservada dinámicamente
    free_dbus();
    g_main_loop_unref(loop);

    return 0;
}
