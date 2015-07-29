import zmq
import random
import string
import json
import sys

server = 'localhost' if len(sys.argv) <= 1 else sys.argv[1]
state_port = 5556
control_port = 5557
lobby_port = 5558


def random_id():
    return "".join(random.choice(string.ascii_letters)
            for i in range(10)).encode()

def connect(context, my_id):
    lobby_socket = context.socket(zmq.REQ)
    lobby_address = "tcp://{}:{}".format(server, lobby_port)
    print("connecting to lobby at {}".format(lobby_address))
    lobby_socket.connect(lobby_address)
    print("sending hello")
    lobby_socket.send(my_id)
    rep = lobby_socket.recv()
    data = json.loads(rep.decode())
    print(data)
    game_name = data["game"]
    secret_code = data["secret"]
    map_data = data["map"]
    return secret_code, map_data, game_name

def play_game(context, my_id, my_secret_code, map_data, game_name):
    print("Starting to play game")
    control_socket = context.socket(zmq.PUSH)
    state_socket = context.socket(zmq.SUB)
    state_socket.setsockopt(zmq.SUBSCRIBE, game_name.encode())
    control_address = "tcp://{}:{}".format(server, control_port)
    state_address = "tcp://{}:{}".format(server, state_port)
    print("Connecting to control and state sockets")
    control_socket.connect(control_address)
    state_socket.connect(state_address)
    print("State and control sockets connected")
    while True:
        print("receiving state info...")
        state_info = state_socket.recv_multipart()
        j = json.loads(state_info[1].decode())
        if j['status'] == "GAME OVER":
            break
        print("state_info: {}".format(state_info))
        print("sending control...")
        linear_control = str(random.choice([1,1,1,1,0]))
        rotational_control = str(random.choice([-1,-1,1,1,0,0,0,0,0,0,0]))
        control_socket.send("{},{},{}".format(my_secret_code,
            linear_control, rotational_control).encode())
        print("control sent")
    print("Game Over!")

def main():
    context = zmq.Context()
    context.linger = 0 # applies to subsequent sockets
    my_id = random_id()
    print("my id: {}".format(my_id))
    while True:
        secret_code, map_data, game_name = connect(context, my_id)
        play_game(context, my_id, secret_code, map_data, game_name)


if __name__ == '__main__':
    main()
