import csrd

# holds a car state
class Car:
    def __init__(self):
        self.speed = 0
        self.id = 0
        self.front_light_state = csrd.STATES.OFF
        self.break_light_state = csrd.STATES.OFF
        self.left_light_state = csrd.STATES.OFF
        self.right_light_state = csrd.STATES.OFF
        self.sirene_light_state = csrd.STATES.OFF
        self.board_state = csrd.STATES.NORMAL
        self.reed_state = csrd.STATES.OFF
        self.updated = False

    def set_update(self, update):
        self.updated = update

    def get_update(self):
        return self.updated

    def set_id(self, id):
        self.id = id

    def get_id(self):
        return self.id

    def set_speed(self, speed):
        self.speed = speed

    def get_speed(self):
        return self.speed

    def set_front_light_state(self, state):
        self.front_light_state = state

    def get_front_light_state(self):
        return self.front_light_state

    def set_break_light_state(self, state):
        self.break_light_state = state

    def get_break_light_state(self):
        return self.break_light_state

    def set_left_light_state(self, state):
        self.left_light_state = state

    def get_left_light_state(self):
        return self.left_light_state

    def set_right_light_state(self, state):
        self.right_light_state = state

    def get_right_light_state(self):
        return self.right_light_state

    def set_sirene_light_state(self, state):
        self.sirene_light_state = state

    def get_sirene_light_state(self):
        return self.sirene_light_state

    def set_board_state(self, state):
        self.board_state = state

    def get_board_state(self):
        return self.board_state

    def set_reed_state(self, state):
        self.reed_state = state

    def get_reed_state(self):
        return self.reed_state