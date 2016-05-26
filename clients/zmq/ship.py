import logging

logger = logging.getLogger(__name__)
logging.basicConfig(
    level=logging.INFO,
    datefmt='%I:%M:%S %p',
    format='%(asctime)s [%(levelname)s]: %(message)s'
)


class Ship:

    def __init__(self, client, ship_name, ship_team, secret_key):
        self.client = client
        self.name = ship_name
        self.team = ship_team
        self.secret_key = secret_key
        self.pressed = set()

    def key_press(self, event):
        self.pressed.add(event.key)
        self.update_control()

    def key_release(self, event):
        self.pressed.discard(event.key)
        self.update_control()

    def update_control(self):
        linear = int('up' in self.pressed)
        rotation = 0
        rotation += int('left' in self.pressed)
        rotation -= int('right' in self.pressed)
        self.send_control(linear, rotation)

    def send_control(self, linear, rotational):
        control_lst = [self.secret_key, linear, rotational]
        control_str = ','.join(map(str, control_lst))
        logger.info('Sending control string "{}"'.format(control_str))
        self.client.control.sock.send_string(control_str)
