import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
from gi.repository import Gio
import sys

class Handler:
    def onDestroy(self, *args):
        Gtk.main_quit()

    def onSendActionButtonClicked(self, button):
        print("Hello World!")

builder = Gtk.Builder()
builder.add_from_file("../resources/ui.glade")
builder.connect_signals(Handler())

window = builder.get_object("main_window")
window.show_all()

Gtk.main()