#!/usr/bin/env python

from dbus import SessionBus, Interface

bus = SessionBus()
mpris_obj = bus.get_object('org.gnome.Rhythmbox3',
                           '/org/mpris/MediaPlayer2')
rb_plm_obj = bus.get_object('org.gnome.Rhythmbox3',
                              '/org/gnome/Rhythmbox3/PlaylistManager')
mpris_introspectable = Interface(mpris_obj,
                                 'org.freedesktop.DBus.Introspectable')

rb_plm_introspectable = Interface(rb_plm_obj,
                                  'org.freedesktop.DBus.Introspectable')

with open('org.mpris.MediaPlayer2.rhythmbox.xml', 'w') as mpris_xml:
    print >>mpris_xml, mpris_introspectable.Introspect()

with open('org.gnome.Rhythmbox3.PlaylistManager.xml', 'w') as rb_plm_xml:
    print >>rb_plm_xml, rb_plm_introspectable.Introspect()
