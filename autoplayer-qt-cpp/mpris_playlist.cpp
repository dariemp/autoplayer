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

#include "mpris_playlist.h"

QDBusArgument &operator<<(QDBusArgument &argument, const Playlist &playlist)
{
    argument.beginStructure();
    argument << playlist.Id << playlist.Name << playlist.IconUri;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Playlist &playlist)
{
    argument.beginStructure();
    argument >> playlist.Id >> playlist.Name >> playlist.IconUri;
    argument.endStructure();
    return argument;
}
