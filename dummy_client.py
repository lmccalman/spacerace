import zmq
import random
import string
import json

state_port = 5556
control_port = 5557
lobby_port = 5558

def random_id():
    return "".join(random.choice(string.ascii_letters) 
            for i in range(10)).encode()

def connect(context, my_id):
    lobby_socket = context.socket(zmq.REQ)
    lobby_address = "tcp://localhost:{}".format(lobby_port)
    print("connecting to lobby at {}".format(lobby_address))
    lobby_socket.connect(lobby_address)
    print("sending hello")
    lobby_socket.send(my_id)
    rep = lobby_socket.recv_multipart()
    secret_code = rep[0]
    map_data = rep[1]
    print("my secret code: {}".format(secret_code))
    print("map data: {}".format(map_data))
    return secret_code, map_data

def play_game(context, my_id, my_secret_code, map_data):
    print("Starting to play game")
    control_socket = context.socket(zmq.PUSH);
    state_socket = context.socket(zmq.SUB)
    state_socket.setsockopt(zmq.SUBSCRIBE, b"")
    control_address = "tcp://localhost:{}".format(control_port)
    state_address = "tcp://localhost:{}".format(state_port)
    print("Connecting to control and state sockets")
    control_socket.connect(control_address)
    state_socket.connect(state_address)
    print("State and control sockets connected")
    while True:
        print("receiving state info...")
        state_info = state_socket.recv_multipart()
        j = json.loads(state_info[0].decode())
        if j['status'] == "GAME OVER":
            break;
        print("state_info: {}".format(state_info))
        print("sending control...")
        linear_control = str(random.choice([1,1,1,1,0]))
        rotational_control = str(random.choice([-1,-1,1,1,0,0,0,0,0,0,0]))
        control_socket.send("{},{},{}".format(my_secret_code.decode(),
            linear_control, rotational_control).encode());
        print("control sent")
    print("Game Over!")

def main():
    context = zmq.Context()
    context.linger = 0 # applies to subsequent sockets
    my_id = random_id()
    print("my id: {}".format(my_id))
    while True:
        secret_code, map_data = connect(context, my_id)
        play_game(context, my_id, secret_code, map_data)


if __name__ == '__main__':
    main()
