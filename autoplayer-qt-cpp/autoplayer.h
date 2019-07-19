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

#ifndef AUTOPLAYER_H
#define AUTOPLAYER_H

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtDBus/QtDBus>
#include "rhythmbox_mpris.h"
#include "playlist_manager.h"

// Estos son los namespaces de las interfaces de Rhythmbox
using namespace org::gnome::Rhythmbox3;
using namespace org::mpris::MediaPlayer2;

class Autoplayer : QObject
{
    Q_OBJECT
public:
    Autoplayer();
protected slots:
    void onDeviceChanged(QDBusObjectPath device_object_path);
    void onDeviceRemoved(QDBusObjectPath device_object_path);
protected:
    QMap<QDBusObjectPath, bool> already_scanned;
    QDBusConnection system_bus;
    QDBusConnection session_bus;
    void connect_to_udisks();
    QString get_device_mount_path(QDBusObjectPath device_object_path);
    QString get_mime_type(QString file_path);
    QStringList* get_playlist(QString directory_path);
    PlaylistManager *get_rhythmbox_playlist_manager();
    Player* get_rhythmbox_player();
    Playlists* get_rhythmbox_playlists();
    int get_rhythmbox_playlists_count();
    void play(QStringList *playlist);
};

#endif // AUTOPLAYER_H
