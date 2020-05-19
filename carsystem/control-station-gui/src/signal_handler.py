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
        # maps the radio button labels to csrd states
        self.states = {"ON": csrd.STATES.ON,
                       "OFF": csrd.STATES.OFF,
                       "BLINK": csrd.STATES.BLINKING,
                       "NORMAL": csrd.STATES.NORMAL,
                       "NIGHT": csrd.STATES.NIGHT,
                       "DAY": csrd.STATES.DAY,
                       "EMERGENCY": csrd.STATES.EMERGENCY,
                       "ALL BLINKING": csrd.STATES.ALL_BLINKING,
                       "ACCELERATING": csrd.STATES.ACCELERATING,
                       "STOPPING": csrd.STATES.STOPPING}
        self.selected_car = None

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
        result = self.connection.connect()
        label = self.builder.get_object("label_status_1")
        if result:
            self.queue_consumer.start()
            label.set_text("Connected to " +
                           self.connection.get_host() +
                           " on port " +
                           str(self.connection.get_port()))
        else:
            label.set_text("Failed to connected to " +
                           self.connection.get_host() +
                           " on port " +
                           str(self.connection.get_port()))

    def validate_radio_button_light(self, radio):
        if not radio.get_active():
            return False

        if self.selected_car is None:
            self.logger.debug("No car selected")
            return False

        state = None
        state = self.states[radio.get_label()]
        if state is None:
            self.logger.error("Radio button label does not match state dictionary")
            return False

        return True

    def onMotorChanged(self, radio):

        if not self.validate_radio_button_light(radio):
            return

        state = self.states[radio.get_label()]

        if self.selected_car.get_motor_state() == state:
            self.logger.debug("Motor state didn't change. Nothing to do")
            return False

        nodeid = self.selected_car.get_id()
        self.selected_car.set_motor_state(state)
        self.cars[str(nodeid)].set_motor_state(state)
        message = csrd.CSRD(self.logger)
        if state == csrd.STATES.ON:
            message.createMotorOn(nodeid)
        elif state == csrd.STATES.OFF:
            message.createMotorOff(nodeid)
        else:
            self.logger.debug(f"Invalid state {state}")
            return

        self.queue_consumer.put_message_to_output_queue(message)

    def onSireneChanged(self, radio):

        if not self.validate_radio_button_light(radio):
            return

        state = self.states[radio.get_label()]

        if self.selected_car.get_sirene_light_state() == state:
            self.logger.debug("Light state didn't change. Nothing to do")
            return False

        nodeid = self.selected_car.get_id()
        self.selected_car.set_sirene_light_state(state)
        self.cars[str(nodeid)].set_sirene_light_state(state)
        message = csrd.CSRD(self.logger)
        if state == csrd.STATES.ON:
            message.createSireneLightOn(nodeid)
        elif state == csrd.STATES.OFF:
            message.createSireneLightOff(nodeid)
        elif state == csrd.STATES.BLINKING:
            message.createSireneLightBlink(nodeid)
        else:
            self.logger.debug(f"Invalid state {state}")
            return

        self.queue_consumer.put_message_to_output_queue(message)

    def onFrontLightChanged(self, radio):

        if not self.validate_radio_button_light(radio):
            return

        state = self.states[radio.get_label()]

        if self.selected_car.get_front_light_state() == state:
            self.logger.debug("Light state didn't change. Nothing to do")
            return False

        nodeid = self.selected_car.get_id()
        self.selected_car.set_front_light_state(state)
        self.cars[str(nodeid)].set_front_light_state(state)
        message = csrd.CSRD(self.logger)
        if state == csrd.STATES.ON:
            message.createFrontLightOn(nodeid)
        elif state == csrd.STATES.OFF:
            message.createFrontLightOff(nodeid)
        elif state == csrd.STATES.BLINKING:
            message.createFrontLightBlink(nodeid)
        else:
            self.logger.debug(f"Invalid state {state}")
            return

        self.queue_consumer.put_message_to_output_queue(message)

    def onBreakLightChanged(self, radio):
        if not self.validate_radio_button_light(radio):
            return

        state = self.states[radio.get_label()]

        if self.selected_car.get_break_light_state() == state:
            self.logger.debug("Light state didn't change. Nothing to do")
            return False

        nodeid = self.selected_car.get_id()
        self.selected_car.set_break_light_state(state)
        self.cars[str(nodeid)].set_break_light_state(state)
        message = csrd.CSRD(self.logger)
        if state == csrd.STATES.ON:
            message.createBreakLightOn(nodeid)
        elif state == csrd.STATES.OFF:
            message.createBreakLightOff(nodeid)
        elif state == csrd.STATES.BLINKING:
            message.createBreakLightBlink(nodeid)
        else:
            self.logger.debug(f"Invalid state {state}")
            return

        self.queue_consumer.put_message_to_output_queue(message)

    def onLeftLightChanged(self, radio):
        if not self.validate_radio_button_light(radio):
            return

        state = self.states[radio.get_label()]

        if self.selected_car.get_left_light_state() == state:
            self.logger.debug("Light state didn't change. Nothing to do")
            return False

        nodeid = self.selected_car.get_id()
        self.selected_car.set_left_light_state(state)
        self.cars[str(nodeid)].set_left_light_state(state)
        message = csrd.CSRD(self.logger)
        if state == csrd.STATES.ON:
            message.createLeftLightOn(nodeid)
        elif state == csrd.STATES.OFF:
            message.createLeftLightOff(nodeid)
        elif state == csrd.STATES.BLINKING:
            message.createLeftLightBlink(nodeid)
        else:
            self.logger.debug(f"Invalid state {state}")
            return

        self.queue_consumer.put_message_to_output_queue(message)

    def onRightLightChanged(self, radio):
        if not self.validate_radio_button_light(radio):
            return

        state = self.states[radio.get_label()]

        if self.selected_car.get_right_light_state() == state:
            self.logger.debug("Light state didn't change. Nothing to do")
            return False

        nodeid = self.selected_car.get_id()
        self.selected_car.set_right_light_state(state)
        self.cars[str(nodeid)].set_right_light_state(state)
        message = csrd.CSRD(self.logger)
        if state == csrd.STATES.ON:
            message.createRightLightOn(nodeid)
        elif state == csrd.STATES.OFF:
            message.createRightLightOff(nodeid)
        elif state == csrd.STATES.BLINKING:
            message.createRightLightBlink(nodeid)
        else:
            self.logger.debug(f"Invalid state {state}")
            return

        self.queue_consumer.put_message_to_output_queue(message)

    def onBoardChanged(self, radio):
        if not self.validate_radio_button_light(radio):
            return

        state = self.states[radio.get_label()]

        if self.selected_car.get_board_state() == state:
            self.logger.debug("Board state didn't change. Nothing to do")
            return False

        nodeid = self.selected_car.get_id()
        self.selected_car.set_board_state(state)
        self.cars[str(nodeid)].set_board_state(state)
        message = csrd.CSRD(self.logger)
        if state == csrd.STATES.NORMAL:
            message.createBoardNormal(nodeid)
        elif state == csrd.STATES.DAY:
            message.createBoardDay(nodeid)
        elif state == csrd.STATES.NIGHT:
            message.createBoardNight(nodeid)
        elif state == csrd.STATES.ALL_BLINKING:
            message.createBoardAllBlinking(nodeid)
        elif state == csrd.STATES.EMERGENCY:
            message.createBoardEmergency(nodeid)
        else:
            self.logger.debug(f"Invalid state {state}")
            return

        self.queue_consumer.put_message_to_output_queue(message)

    def onCarsListBoxRowActivated(self, listbox, listboxrow):
        # listboxrow = listbox.get_selected_row()
        if listboxrow is None:
            self.logger.debug("No item selected")
            return

        item = listboxrow.get_child().get_text()
        pos = item.find('-')
        carcode = item[:pos]
        carobj = self.cars[carcode]
        self.selected_car = carobj
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

    def onSaveNodeidButtonClicked(self, button):

        new_nodeid = self.builder.get_object("txt_nodeid").get_text()
        if len(new_nodeid) < 1:
            self.logger.debug("No node id informed")
            return

        self.logger.debug(f"New node id {new_nodeid}")
        new_nodeid = int(new_nodeid)

        nodeid = self.selected_car.get_id()
        if nodeid == new_nodeid:
            self.logger.debug("New node id is the same as actual node id. Nothing to do.")
            return

        # remove the current car from the list
        self.selected_car = None
        self.cars.pop(str(nodeid))

        listbox = self.builder.get_object("app_registered_cars_listbox")
        listboxrow = listbox.get_selected_row()
        listbox.remove(listboxrow)

        # send the message
        message = csrd.CSRD(self.logger)
        message.createAddressedWriteMessage(nodeid,
                                            csrd.CARPARTS.BOARD,
                                            0,
                                            message.lowByte(new_nodeid),
                                            message.highByte(new_nodeid))
        self.queue_consumer.put_message_to_output_queue(message)
