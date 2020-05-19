import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

import logging
import connection
import queue
import config
import time
import queue_handler
import signal_handler

# config the logger
# create logger for the application
logger = logging.getLogger('control_station')
logger.setLevel(logging.DEBUG)
# create file handler which logs even debug messages
fh = logging.FileHandler('control_station.log')
fh.setLevel(logging.DEBUG)
# create console handler with a higher log level
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
# create formatter and add it to the handlers
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
fh.setFormatter(formatter)
ch.setFormatter(formatter)
# add the handlers to the logger
logger.addHandler(fh)
logger.addHandler(ch)

# load the GUI

object_facilities = {}
object_facilities["logger"] = logger

# holds the car states
cars = {}
object_facilities["cars"] = cars

config = config.Config("config.yaml", logger)
config.load()
object_facilities["configuration"] = config

input_queue = queue.Queue()
output_queue = queue.Queue()

builder = Gtk.Builder()
object_facilities["builder"] = builder

connection = connection.TcpClient(object_facilities, input_queue, output_queue)
object_facilities["connection"] = connection

queue_consumer = queue_handler.QueueHandler(object_facilities, input_queue, output_queue)
object_facilities["queue_consumer"] = queue_consumer
builder.add_from_file("../resources/ui.glade")
builder.connect_signals(signal_handler.Handler(object_facilities))

listbox = builder.get_object("app_registered_cars_listbox")
listbox.set_selection_mode(Gtk.SelectionMode.SINGLE)

window = builder.get_object("main_window")
window.show_all()

Gtk.main()
