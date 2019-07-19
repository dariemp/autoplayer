#!/usr/bin/env python
# *- encoding: utf-8 -*
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
# Author: Dariem Pérez Herrera (dariemp [at] uci.cu)
#         Proyecto Nova, Centro de Soluciones Libres
#         Universidad de las Ciencias Informáticas
#
# Este código es una prueba de concepto con fines educativos, se priorizó en
# la claridad por encima de las "mejores prácticas". Espero le sea útil.  ;-)
#


from dbus import SessionBus, SystemBus, Interface
from dbus.mainloop.glib import DBusGMainLoop
from gobject import MainLoop
from os import listdir
from os.path import join, isfile
from mimetypes import guess_type
from urlparse import urljoin
from urllib import pathname2url
from sys import stderr
from time import sleep


class AutoPlayer:

    def __init__(self):
        # Diccionario para evitar los dobles escaneos producto de la emisión
        # de señales repetidas. Pasa porque Rhythmbox vuelve a escanear todos
        # los dispositivos cuando levanta por primera vez.
        self._already_scanned = {}

        # Se obtiene una conexión al bus de sistema
        self._system_bus = SystemBus()

        # Se obtiene una conexión al bus de sesión
        self._session_bus = SessionBus()

        # Conectar a las señales que emite de UDisks
        self._connect_to_udisks()

    def _connect_to_udisks(self):
        # Información de D-Bus para UDisks
        udisks_bus_name = 'org.freedesktop.UDisks'
        udisks_object_path = '/org/freedesktop/UDisks'
        udisks_dbus_interface = 'org.freedesktop.UDisks'

        # Se conecta la señal 'DeviceChanged' exportada por el objeto remoto
        # al manejador local de señales '_onDeviceChanged'.
        self._system_bus.add_signal_receiver(
                                        self._onDeviceChanged,
                                        signal_name='DeviceChanged',
                                        dbus_interface=udisks_dbus_interface,
                                        bus_name=udisks_bus_name,
                                        path=udisks_object_path)

        # Se conecta la señal 'DeviceRemoved' exportada por el objeto remoto
        # al manejador local de señales '_onDeviceRemoved'.
        self._system_bus.add_signal_receiver(
                                        self._onDeviceRemoved,
                                        signal_name='DeviceRemoved',
                                        dbus_interface=udisks_dbus_interface,
                                        bus_name=udisks_bus_name,
                                        path=udisks_object_path)

    def _onDeviceChanged(self, device_object_path):
        " La señal 'DeviceChanged' es emitida cuando se cambia el estado de un"
        " dispositivo (por ejemplo, cuando se monta o desmonta). UDisks envía "
        " el camino del objeto remoto del dispositivo que cambió de estado.   "
        print u'Un dispositivo cambió: %s' % device_object_path
        mount_path = self._get_device_mount_path(device_object_path)
        # Si no está montado o ya fue escaneado, no hacer nada y retornar
        if not mount_path:
            return
        playlist = self._get_playlist(mount_path)
        # Si no hay música en el dispositivo, no hacer nada y retornar
        if not playlist:
            return
        self._play(playlist)

    def _onDeviceRemoved(self, device_object_path):
        " La señal 'DeviceRemoved' es emitida cuando un dispositivo es   "
        " desconectado por hardware de la PC. UDisks envía el camino del "
        " objeto remoto del dispositivo."
        print u'Un dispositivo fue desconectado: %s' % device_object_path
        if device_object_path in self._already_scanned:
            del self._already_scanned[device_object_path]

    def _get_device_mount_path(self, device_object_path):
        # Información necesaria para procesar los dispositivos de UDisks
        udisks_bus_name = 'org.freedesktop.UDisks'
        device_dbus_interface = 'org.freedesktop.UDisks.Device'

        # Se obtiene el objeto remoto del dispositivo insertado a través
        # de la conexión al bus de sistema del demonio UDisks.
        device_object = self._system_bus.get_object(udisks_bus_name,
                                                    device_object_path)

        # Para acceder a las propiedades de un objeto FreeDesktop.org
        # establece una interfaz estándar: 'org.freedesktop.DBus.Properties'
        device = Interface(device_object, 'org.freedesktop.DBus.Properties')

        # Con el método 'Get' de la interfaz 'org.freedesktop.DBus.Properties'
        # se obtiene el valor de una propiedad en una interfaz determinada.

        # La propiedad 'DeviceFile' devuelve el archivo de dispositivo en /dev
        # En nuestro caso no lo necesitamos, se pone a modo de enseñanza.
        dev_file = device.Get(device_dbus_interface, 'DeviceFile')

        # La propiedad 'DeviceIsMounted' devuelve si está montado
        is_mounted = device.Get(device_dbus_interface, 'DeviceIsMounted')
        print u'¿Está montado %s? : %s' % (dev_file, str(bool(is_mounted)))

        # Si el dispositivo ya fue escaneado y no está montado, significa que
        # recién se desmontó, hay que olvidarlo.
        mount_path = None
        if device_object_path in self._already_scanned:
            if not is_mounted:
                del  self._already_scanned[device_object_path]
        # Si no ha sido escaneado y está montado, significa que recién se
        # ha montado y hay que escanearlo y recordarlo, se devuelve el punto
        # de montaje. En otros casos, no se hace nada y no se devuelve nada.
        elif is_mounted:
            self._already_scanned[device_object_path] = True
            # La propiedad 'DeviceMountPaths' devuelve todos los directorios
            # del sistema de archivos donde se ha montado este dispositivo.
            mount_paths = device.Get(device_dbus_interface, 'DeviceMountPaths')
            mount_path = mount_paths[0]
        return mount_path

    def _get_playlist(self, directory_path):
        print u'Buscando música en el directorio %s' % directory_path
        playlist = []
        filenames = listdir(directory_path)
        for filename in filenames:
            path = join(directory_path, filename)
            if isfile(path):
                # Se obtiene el tipo MIME, ejemplo "audio/mpeg"
                mime_type = guess_type(path)[0]
                # Lo que interesa es la primera parte del tipo MIME
                category = mime_type.split('/')[0]
                if category == 'audio':
                    # El Playlist Manager de Rhythmbox espera URIs, por tanto..
                    uri = urljoin('file:', pathname2url(path))
                    playlist.append(uri)
        return playlist

    def _get_rhythmbox_playlist_manager(self):
        # Para modificar la lista de reproducción no hay MPRIS que valga :-(
        # Información de D-Bus para el Playlist Manager de Rhythmbox
        rhythmbox_bus_name = 'org.gnome.Rhythmbox3'
        playlist_mgr_object_path = '/org/gnome/Rhythmbox3/PlaylistManager'
        playlist_mgr_dbus_interface = 'org.gnome.Rhythmbox3.PlaylistManager'

        # Se obtiene el objeto remoto de Rhythmbox desde el bus de sesión
        playlist_mgr_obj = self._session_bus.get_object(rhythmbox_bus_name,
                                                    playlist_mgr_object_path)

        # Esperar a que levante Rhythmbox... sí, ok, ya sé que esto es una
        # mala práctica. Lo correcto sería conectar a alguna señal remota que
        # avise de que está listo el reproductor, pero mejor no complicarse :-P
        sleep(5)

        playlist_manager = Interface(playlist_mgr_obj,
                                     playlist_mgr_dbus_interface)
        return playlist_manager

    def _get_rhythmbox_player(self):
        # Para controlar el reproductor, usaremos la interfaz estándar que
        # define FreeDestkop.org, es decir MPRIS

        # Información de D-Bus para Rhythmbox usando interfaz estándar MPRIS
        rhythmbox_bus_name = 'org.mpris.MediaPlayer2.rhythmbox'
        rhythmbox_object_path = '/org/mpris/MediaPlayer2'
        rhythmbox_dbus_interface_player = 'org.mpris.MediaPlayer2.Player'
        rhythmbox_dbus_interface_playlists = 'org.mpris.MediaPlayer2.Playlists'

        # Se obtiene el objeto remoto de Rhythmbox desde el bus de sesión
        rhythmbox_object = self._session_bus.get_object(rhythmbox_bus_name,
                                                        rhythmbox_object_path)

        # Se obtiene la interfaz Player del objeto remoto del reproductor
        rhythmbox_player = Interface(rhythmbox_object,
                                     rhythmbox_dbus_interface_player)

        # Se obtiene la interfaz Playerlists del objeto remoto del reproductor
        rhythmbox_playlists = Interface(rhythmbox_object,
                                        rhythmbox_dbus_interface_playlists)

        # Se necesita obtener la cantidad de listas de reproducción contenidas
        # en el reproductor. Eso lo devuelve la propiedad "PlaylistCount"
        playlists_properties = Interface(rhythmbox_object,
                                         'org.freedesktop.DBus.Properties')
        pl_count = playlists_properties.Get(rhythmbox_dbus_interface_playlists,
                                            'PlaylistCount')

        return rhythmbox_player, rhythmbox_playlists, pl_count

    def _play(self, playlist):
        # Obtener el Playlist Manager de Rhythmbox
        playlist_manager = self._get_rhythmbox_playlist_manager()

        # Nombre estático para la lista de reproducción,
        # se llamará siempre "Autoplayer"
        playlist_name = 'Autoplayer'
        try:
            # Borrar la lista de reproducción si ya existe
            playlist_manager.DeletePlaylist(playlist_name)
        except:
            # Si no existe levanta excepción, la ignoramos
            pass
        # Crear una lista de reproducción nueva
        playlist_manager.CreatePlaylist(playlist_name)

        # Se adiciona cada archivo de música obtenido del dispositvo
        # de almacenamiento a la nueva lista de reproducción
        for uri in playlist:
            playlist_manager.AddToPlaylist(playlist_name, uri)

        # Obtenemos el reproductor y sus listas desde la interfaz MPRIS
        (rhythmbox_player,
         playlists_interface, playlists_count) = self._get_rhythmbox_player()

        # En un mundo ideal, Rhythmbox sería buenito y ordenaría las listas de
        # reproducción por fecha de creación en orden reverso como indica el
        # API de MPRIS, pero desgraciadamente no soporta esta funcionalidad,
        # así que hay que buscar a lo bestia... :-P :-(
        playlists = playlists_interface.GetPlaylists(0, playlists_count,
                                                     'CreationDate', True)

        # Buscando nuestra lista de reproducción a lo bestia... :-P
        i = 0
        while i < playlists_count and playlists[i][1] != playlist_name:
            i += 1
        if i < playlists_count:
            playlist_id = playlists[i][0]
        else:
            print >>stderr, u'Rhythmbox no cargó la lista correctamente.'
            return

        # Si se llega aquí, es que se encontró nuestra lista de reproducción
        # Detenemos cualquier reproducción previa para evitar mareadera :-P
        rhythmbox_player.Stop()

        # Activar la lista de reproducción como lista de reproducción actual
        playlists_interface.ActivatePlaylist(playlist_id)

        # Reproducir... :-)
        rhythmbox_player.Play()


def main():
    # Se establece que D-Bus utilizará como ciclo de mensajes por defecto
    # al que provee Glib
    DBusGMainLoop(set_as_default=True)

    # Se instancia el objeto que procesará la señal proveniente de UDisks
    autoplayer = AutoPlayer()

    # Se instancia y pone a correr el ciclo de mensajes en espera de la emisión
    # de la señal de UDisks
    mainloop = MainLoop()
    mainloop.run()

if __name__ == '__main__':
    main()
