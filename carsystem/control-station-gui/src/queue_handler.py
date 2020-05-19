import threading
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
import csrd
import car


class QueueHandler:

    def __init__(self, facilities, input_queue, output_queue):
        self.builder = facilities["builder"]
        self.logger = facilities["logger"]
        self.input_queue = input_queue
        self.output_queue = output_queue
        self.running = 0
        self.threads = []
        self.event = threading.Event()
        self.registered = facilities["cars"]
        self.wait_seconds = 0.001

    def run_input_queue(self, event):
        self.logger.info("[run_input_queue] Running input queue consumer.")
        while self.running:
            while not self.input_queue.empty():
                self.logger.debug(f"[run_input_queue] Input queue size is {self.input_queue.qsize()}")
                msg = self.input_queue.get()
                msg = msg.decode('ascii')
                self.logger.debug(f"[run_input_queue] Received message [{msg}]")
                messages = msg.split('\n')
                for m in messages:
                    self.handle_message(m)
                if self.input_queue.empty():
                    self.logger.debug("[run_input_queue] Queue is empty.")

            event.wait(self.wait_seconds)
        self.logger.info("[run_input_queue] Stopping input queue consumer.")

    def handle_message(self, message):
        if str(message) == '\n' or len(message) < 1:
            return

        csrd_message = csrd.CSRD(self.logger)
        if csrd_message.setMessageFromHexaString(0, message) == 0:
            self.logger.debug(f"[handle_message] Message is not in hexastring format [{message}]")

        self.add_radio_to_list(csrd_message.bufferToHexString() + " - " + csrd_message.getNiceMessage())

        csrd_message.printNice()

        if csrd_message.isInitialRegister():

            label = self.builder.get_object("label_total_registrations")

            self.logger.debug(f"[handle_message] Handling register message {message}.")
            node_number = csrd_message.getNodeNumber()
            out_message = csrd.CSRD(self.logger)
            self.logger.debug(f"[handle_message] Answer register message {message}.")
            out_message.createInitialRegisterMessage(node_number,
                                                     csrd.STATUS.ACTIVE,
                                                     255,
                                                     255,
                                                     255)
            self.output_queue.put(out_message)
            if str(node_number) not in self.registered.keys():
                self.add_message_to_list(str(node_number) + "-ACTIVE")
                c = car.Car()
                c.set_id(node_number)
                self.registered[str(node_number)] = c
            else:
                self.logger.debug("[handle_message] Car registered already.")

            # request all states
            out_message_2 = csrd.CSRD(self.logger)
            out_message_2.createQueryAllStates(node_number)
            self.output_queue.put(out_message_2)
            label.set_text(str(len(self.registered)))

        if csrd_message.isAnswerState():
            self.logger.debug(f"[handle_message] Handling answer state message {message}.")
            node_number = csrd_message.getNodeNumber()
            element = csrd_message.getElement()
            state = csrd_message.getState()
            self.setElementState(node_number, element, state)

    def setElementState(self, node_number, element, state):
        if node_number in self.registered.keys():

            self.logger.debug(f"Setting state. ID [{id}] state [{state}] element [{element}]")
            self.registered[node_number].set_update(True)
            if element == csrd.CARPARTS.BREAK_LIGHT:
                self.registered[node_number].set_break_light_state(state)
            elif element == csrd.CARPARTS.FRONT_LIGHT:
                self.registered[node_number].set_front_light_state(state)
            elif element == csrd.CARPARTS.IR_RECEIVE:
                pass
            elif element == csrd.CARPARTS.BOARD:
                self.registered[node_number].set_board_state(state)
            elif element == csrd.CARPARTS.IR_SEND:
                pass
            elif element == csrd.CARPARTS.LEFT_LIGHT:
                self.registered[node_number].set_left_light_state(state)
            elif element == csrd.CARPARTS.RIGHT_LIGHT:
                self.registered[node_number].set_right_light_state(state)
            elif element == csrd.CARPARTS.MOTOR:
                pass
            elif element == csrd.CARPARTS.REED:
                self.registered[node_number].set_reed_state(state)

    def add_message_to_list(self, message):
        listbox = self.builder.get_object("app_registered_cars_listbox")
        item = Gtk.ListBoxRow()
        label = Gtk.Label(message)
        label.set_justify(Gtk.Justification.LEFT)
        label.set_halign(Gtk.Align.START)
        item.add(label)
        listbox.add(item)
        listbox.show_all()

    def add_radio_to_list(self, message):
        listbox = self.builder.get_object("list_radio_messages")
        item = Gtk.ListBoxRow()
        label = Gtk.Label(message)
        label.set_justify(Gtk.Justification.LEFT)
        label.set_halign(Gtk.Align.START)
        item.add(label)
        listbox.add(item)
        listbox.show_all()

    def put_message_to_output_queue(self, message):
        self.output_queue.put(message)

    def start(self):
        self.logger.info("Starting queue consumer threads")
        self.running = 1
        t = threading.Thread(target=self.run_input_queue, args=(self.event,))
        self.threads.append(t)
        t.start()

    def stop(self):
        self.running = 0
        self.event.set()

