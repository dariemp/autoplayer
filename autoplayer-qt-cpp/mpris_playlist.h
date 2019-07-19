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


/* Este archivo existe porque qdbusxml2cpp es incapaz de generar código para tipos
 * de datos relativamente complejos, como este caso de una lista de estructuras.
 * Entonces es responsabilidad del programador declarar y registrar estos tipos,
 * así como definir como se hará el marshalling de los mismos para transmitir a
 * través de DBus (ver la implementación de los operadores "<<" y ">>" en
 * "mpris_playlist.cpp") y el registro como metatipos en "main.cpp"  */

#ifndef MPRIS_TYPES_H
#define MPRIS_TYPES_H

#include <QtDBus>
#include <QtCore/QMetaType>

struct Playlist
{
     QDBusObjectPath Id;
     QString Name;
     QString IconUri;
};
Q_DECLARE_METATYPE(Playlist)

QDBusArgument &operator<<(QDBusArgument &argument, const Playlist &playlist);
const QDBusArgument &operator>>(const QDBusArgument &argument, Playlist &playlist);

typedef QList<Playlist> QPlaylists;
Q_DECLARE_METATYPE(QPlaylists)

#endif // MPRIS_TYPES_H
