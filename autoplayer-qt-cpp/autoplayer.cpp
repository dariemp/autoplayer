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
 
#include "autoplayer.h"
#include "udisks.h"
#include <iostream>
#include <magic.h>

using namespace std;

/*
 * Se obtiene una conexión al bus de sistema para usar con UDisks y otra al de
 * sesión para usar con Rhythmbox. Se inicializa un QMap para controlar la
 * repitición de escaneos innecesarios a los dispositivos.
 */
Autoplayer::Autoplayer()
    :already_scanned(QMap<QDBusObjectPath, bool>()),
     system_bus(QDBusConnection::systemBus()),
     session_bus(QDBusConnection::sessionBus())
{
    // Conectar a las señales que emite de UDisks
    connect_to_udisks();
}

void Autoplayer::connect_to_udisks()
{
    // Información de D-Bus para UDisks
    QString udisks_bus_name = "org.freedesktop.UDisks";
    QString udisks_object_path = "/org/freedesktop/UDisks";
    QString udisks_dbus_interface = "org.freedesktop.UDisks";

    /* Se conecta la señal "DeviceChanged" exportada por el objeto remoto
     * al SLOT local "onDeviceChanged" */
    system_bus.connect(udisks_bus_name,
                       udisks_object_path,
                       udisks_dbus_interface,
                       "DeviceChanged",
                       this,
                       SLOT(onDeviceChanged(QDBusObjectPath)));

    /* Se conecta la señal "DeviceRemoved" exportada por el objeto remoto
     * al SLOT local "onDeviceRemoved" */
    system_bus.connect(udisks_bus_name,
                       udisks_object_path,
                       udisks_dbus_interface,
                       "DeviceRemoved",
                       this,
                       SLOT(onDeviceRemoved(QDBusObjectPath)));
}

void Autoplayer::onDeviceChanged(QDBusObjectPath device_object_path)
{
    /* La señal 'DeviceChanged' es emitida cuando se cambia el estado de un
     * dispositivo (por ejemplo, cuando se monta o desmonta). UDisks envía
     * el camino del objeto remoto del dispositivo que cambió de estado. */
    cout << "Un dispositivo cambió: " << device_object_path.path().toStdString() << endl;
    QString mount_path = get_device_mount_path(device_object_path);
    // Si no está montado o ya fue escaneado, no hacer nada y retornar
    if (mount_path == NULL)
        return;
    QStringList *playlist = get_playlist(mount_path);
    // Si no hay música en el dispositivo, no hacer nada y retornar
    if (playlist->isEmpty()) {
        delete playlist;
        return;
    }
    play(playlist);
}

void Autoplayer::onDeviceRemoved(QDBusObjectPath device_object_path)
{
    /* La señal 'DeviceRemoved' es emitida cuando un dispositivo es
     * desconectado por hardware de la PC. UDisks envía el camino del
     * objeto remoto del dispositivo.  */
    cout << "Un dispositivo fue desconectado: " << device_object_path.path().toStdString() << endl;
    if (already_scanned.contains(device_object_path))
        already_scanned.remove(device_object_path);
}

QString Autoplayer::get_device_mount_path(QDBusObjectPath device_object_path)
{
    // Información necesaria para procesar los dispositivos de UDisks
    QString udisks_bus_name = "org.freedesktop.UDisks";
    QString device_dbus_interface = "org.freedesktop.UDisks.Device";

    /* Para acceder a las propiedades de un objeto, es más cómo hacerlo a través
     * del método property() de QDBusInterface. */
    QDBusInterface device(udisks_bus_name, device_object_path.path(), device_dbus_interface, system_bus);

    /* La propiedad 'DeviceFile' devuelve el archivo de dispositivo en /dev
     * En nuestro caso no lo necesitamos, se pone a modo de enseñanza.  */
    QString dev_file = device.property("DeviceFile").toString();

    // La propiedad 'DeviceIsMounted' devuelve si está montado
    bool is_mounted =  device.property("DeviceIsMounted").toBool();
    cout << "¿Está montado " << dev_file.toStdString() << "? : " << (is_mounted ? "True": "False") << endl;
    
    /* Si el dispositivo ya fue escaneado y no está montado, significa que
     * recién se desmontó, hay que olvidarlo. */
    QString mount_path = NULL;
    if (already_scanned.contains(device_object_path)) {
            if (!is_mounted)
                already_scanned.remove(device_object_path);
    }
    /* Si no ha sido escaneado y está montado, significa que recién se
     * ha montado y hay que escanearlo y recordarlo, se devuelve el punto
     * de montaje. En otros casos, no se hace nada y no se devuelve nada. */
    else if(is_mounted) {
        already_scanned[device_object_path] = true;
        /* La propiedad 'DeviceMountPaths' devuelve todos los directorios
         * del sistema de archivos donde se ha montado este dispositivo. */
        mount_path = device.property("DeviceMountPaths").toStringList()[0];
    }
    return mount_path;
}

QString Autoplayer::get_mime_type(QString file_path)
{
    /* Este método es para obtener el tipo MIME de un fichero en el sistema
     * de archivos a través de libmagic, ya que al parecer Qt4 (Core) no tiene
     * una forma rápida y relativamente segura de hacerlo */
    QString result("application/octet-stream");
    magic_t magicMimePredictor;
    magicMimePredictor = magic_open(MAGIC_MIME_TYPE);
    if (magicMimePredictor) {
        if (!magic_load(magicMimePredictor, 0)) {
            char *file = file_path.toAscii().data();
            const char *mime = magic_file(magicMimePredictor, file);
            result = QString(mime);
        }
        magic_close(magicMimePredictor);
    }
    return result;
}

QStringList *Autoplayer::get_playlist(QString directory_path)
{
    cout << "Buscando música en el directorio " << directory_path.toStdString() << endl;
    QStringList *playlist = new QStringList();
    QDir directory(directory_path);
    // Filtrar: sólo quiero obtener archivos, no directorios, incluye los ocultos.
    directory.setFilter(QDir::Files | QDir::Hidden);
    QFileInfoList list = directory.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo file_info = list.at(i);
        QString file_path = file_info.absoluteFilePath();
        // Se obtiene el tipo MIME, ejemplo "audio/mpeg"
        QString mime_type = get_mime_type(file_path);
        // Lo que interesa es la primera parte del tipo MIME
        QString category = mime_type.section("/", 0, 0);
        if (category == "audio") {
            // El Playlist Manager de Rhythmbox espera URIs, por tanto...
            QString uri = QUrl::fromLocalFile(file_path).toEncoded();
            playlist->append(uri);
        }
    }
    return playlist;
}

PlaylistManager* Autoplayer::get_rhythmbox_playlist_manager(){
    // Para modificar la lista de reproducción no hay MPRIS que valga :-(

    // Información de D-Bus para el Playlist Manager de Rhythmbox
    QString rhythmbox_bus_name = "org.gnome.Rhythmbox3";
    QString playlist_mgr_object_path = "/org/gnome/Rhythmbox3/PlaylistManager";

    // Se invoca el constructor de la interfaz "org.gnome.Rhythmbox3.PlaylistManager"
    return new PlaylistManager(rhythmbox_bus_name, playlist_mgr_object_path, session_bus);
}

Player* Autoplayer::get_rhythmbox_player()
{
    /* Para controlar el reproductor, usaremos la interfaz estándar que
     * define FreeDestkop.org, es decir MPRIS */
    QString rhythmbox_bus_name = "org.mpris.MediaPlayer2.rhythmbox";
    QString rhythmbox_object_path = "/org/mpris/MediaPlayer2";

    // Se invoca el constructor de la interfaz "org.mpris.MediaPlayer2.Player"
    return new Player(rhythmbox_bus_name, rhythmbox_object_path, session_bus);
}

Playlists* Autoplayer::get_rhythmbox_playlists()
{
    /* Se obtiene la interfaz Playerlists definida por MPRIS necesaria para
     * establecer la lista de reproducción actual antes de reproducir */
    QString rhythmbox_bus_name = "org.mpris.MediaPlayer2.rhythmbox";
    QString rhythmbox_object_path = "/org/mpris/MediaPlayer2";

    // Se invoca el constructor de la interfaz "org.mpris.MediaPlayer2.Playlists
    return new Playlists(rhythmbox_bus_name, rhythmbox_object_path, session_bus);
}

int Autoplayer::get_rhythmbox_playlists_count()
{
    /* Se necesita obtener la cantidad de listas de reproducción contenidas
     * en el reproductor. Eso lo devuelve la propiedad "PlaylistCount" */
    QString rhythmbox_bus_name = "org.mpris.MediaPlayer2.rhythmbox";
    QString rhythmbox_object_path = "/org/mpris/MediaPlayer2";
    QString rhythmbox_dbus_interface_playlists = "org.mpris.MediaPlayer2.Playlists";
    QDBusInterface playlists(rhythmbox_bus_name, rhythmbox_object_path, rhythmbox_dbus_interface_playlists, session_bus);
    return playlists.property("PlaylistCount").toInt();
}

void Autoplayer::play(QStringList *playlist)
{
    /* Nombre estático para la lista de reproducción,
     * se llamará siempre "Autoplayer"  */
    QString playlist_name = "Autoplayer";
    
    // Obtener el Playlist Manager de Rhythmbox
    PlaylistManager *playlist_manager = get_rhythmbox_playlist_manager();
    try {
        // Borrar la lista de reproducción si ya existe
        playlist_manager->DeletePlaylist(playlist_name);
    }
    // Si no existe levanta excepción, la ignoramos
    catch (exception){}
    // Crear una lista de reproducción nueva
    playlist_manager->CreatePlaylist(playlist_name);

    /* Se adiciona cada archivo de música obtenido del dispositvo
     * de almacenamiento a la nueva lista de reproducción. */
    for (int i=0; i < playlist->count(); i++)
        playlist_manager->AddToPlaylist(playlist_name, (*playlist)[i]);

    // Ya se puede liberar el objeto proxy del Playlist Manager
    delete playlist_manager;

    /* Esperar a que levante Rhythmbox... sí, ok, ya sé que esto es una
     * mala práctica. Lo correcto sería conectar a alguna señal remota que
     * avise de que está listo el reproductor, pero mejor no complicarse :-P */
    sleep(5);

    // Obtener la interfaz de listas de reproducción definida por MPRIS
    Playlists* playlists_interface = get_rhythmbox_playlists();
    int playlists_count = get_rhythmbox_playlists_count();

    /* En un mundo ideal, Rhythmbox sería buenito y ordenaría las listas de
     * reproducción por fecha de creación en orden reverso como indica el
     * API de MPRIS, pero desgraciadamente no soporta esta funcionalidad,
     * así que hay que buscar a lo bestia... :-P :-(  */
    QDBusPendingReply<QPlaylists> playlists_reply =  playlists_interface->GetPlaylists(0, playlists_count, "CreationDate", true);
    
    /* La invocación de métodos remotos acá en Qt se hace siempre de forma asíncrona,
     * por tanto, si queremos un algoritmo secuencia, tenermos que esperar aquí por el
     * valor de retorno */
    playlists_reply.waitForFinished();

    // Ver "mpris_playlist.h" para más información sobre el tipo QPlaylists
    QPlaylists playlists = playlists_reply.value();

    // Buscando nuestra lista de reproducción a lo bestia... :-P
    QDBusObjectPath playlist_id;
    int i;
    for (i = 0; i < playlists_count && playlists[i].Name != playlist_name; i++);
    if (i < playlists_count)
        playlist_id = playlists[i].Id;
    else {
        cerr << "Rhythmbox no cargó la lista correctamente." << endl;
        return;
    }
    
    /* Si se llega aquí, es que se encontró nuestra lista de reproducción */
    // Obtener el reproductor
    Player* rhythmbox_player = get_rhythmbox_player();
    
    // Detenemos cualquier reproducción previa para evitar mareadera :-P
    rhythmbox_player->Stop();

    // Activar la lista de reproducción como lista de reproducción actual
    playlists_interface->ActivatePlaylist(playlist_id);

    // Ya se puede liberar la interfaz Playlists de MPRIS
    delete playlists_interface;

    // Reproducir... :-)
    rhythmbox_player->Play();

    // Liberar el objeto proxy del reproductor
    delete rhythmbox_player;
}
