import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
import time
import car
import csrd

class Handler:

    def __init__(self, facilities):
        self.builder = facilities["builder"]
        self.connection = facilities["connection"]
        self.logger = facilities["logger"]
        self.queue_consumer = facilities["queue_consumer"]
        self.cars = facilities["cars"]

    def onDestroy(self, *args):
        self.connection.stop()
        self.queue_consumer.stop()
        time.sleep(1)
        Gtk.main_quit()

    def onSendActionButtonClicked(self, button):
        listbox = self.builder.get_object("app_registered_cars_listbox")
        listboxrow = listbox.get_selected_row()
        if listboxrow is None:
            self.logger.debug("No item selected")
            return

        item = listboxrow.get_child().get_text()
        pos = item.find('-')
        car = item[:pos]
        self.logger.debug(f"Car selected {car}")

    def onConnectButtonClicked(self, button):
        self.connection.connect()
        self.queue_consumer.start()

    def onComboChanged(self, combo):
        print(combo.get_active_id())

    def onCarsListBoxRowActivated(self, listbox, listboxrow):
        # listboxrow = listbox.get_selected_row()
        if listboxrow is None:
            self.logger.debug("No item selected")
            return

        item = listboxrow.get_child().get_text()
        pos = item.find('-')
        carcode = item[:pos]
        carobj = self.cars[carcode]
        if not carobj.get_update():
            # set the interface
            # front light
            state = carobj.get_front_light_state()
            if state == csrd.STATES.OFF:
                combo = self.builder.get_object("radio_off_front_light")
                combo.set_active(True)
            elif state == csrd.STATES.ON:
                combo = self.builder.get_object("radio_on_front_light")
                combo.set_active(True)
            elif state == csrd.STATES.BLINKING:
                combo = self.builder.get_object("radio_blink_front_light")
                combo.set_active(True)

            state = carobj.get_break_light_state()
            if state == csrd.STATES.OFF:
                combo = self.builder.get_object("radio_off_break_light")
                combo.set_active(True)
            elif state == csrd.STATES.ON:
                combo = self.builder.get_object("radio_on_break_light")
                combo.set_active(True)
            elif state == csrd.STATES.BLINKING:
                combo = self.builder.get_object("radio_blink_break_light")

            state = carobj.get_left_light_state()
            if state == csrd.STATES.OFF:
                combo = self.builder.get_object("radio_off_left_light")
                combo.set_active(True)
            elif state == csrd.STATES.ON:
                combo = self.builder.get_object("radio_on_left_light")
                combo.csrd(True)
            elif state == car.STATES.BLINKING:
                combo = self.builder.get_object("radio_blink_left_light")

            state = carobj.get_right_light_state()
            if state == csrd.STATES.OFF:
                combo = self.builder.get_object("radio_off_right_light")
                combo.set_active(True)
            elif state == csrd.STATES.ON:
                combo = self.builder.get_object("radio_on_right_light")
                combo.set_active(True)
            elif state == csrd.STATES.BLINKING:
                combo = self.builder.get_object("radio_blink_right_light")

            state = carobj.get_board_state()
            if state == csrd.STATES.NORMAL:
                combo = self.builder.get_object("radio_normal_board")
                combo.set_active(True)
            elif state == csrd.STATES.DAY:
                combo = self.builder.get_object("radio_day_board")
                combo.set_active(True)
            elif state == csrd.STATES.NIGHT:
                combo = self.builder.get_object("radio_night_board")
                combo.set_active(True)
            elif state == csrd.STATES.EMERGENCY:
                combo = self.builder.get_object("radio_emergency_board")
                combo.set_active(True)

        print(carobj.get_id())

    def onStopButtonClicked(self, button):
        listbox = self.builder.get_object("app_registered_cars_listbox")
        for i in range(444, 449):
            item = Gtk.ListBoxRow()
            label = Gtk.Label(str(i) + "-ACTIVE")
            label.set_justify(Gtk.Justification.LEFT)
            label.set_halign(Gtk.Align.START)
            item.add(label)
            listbox.add(item)
            carobj = car.Car()
            carobj.set_id(i)
            self.cars[str(i)] = carobj
        listbox.show_all()