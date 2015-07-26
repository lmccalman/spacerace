import zmq
import random
import string

state_port = 5556
control_port = 5557
lobby_port = 5558
info_port = 5559

id_length = 10

def random_id():
    return "".join(random.choice(string.ascii_letters) 
            for i in range(id_length)).encode()

def connect(context):
    lobby_socket = context.socket(zmq.REQ)
    lobby_address = "tcp://localhost:{}".format(lobby_port)
    print("connecting to lobby at {}".format(lobby_address))
    lobby_socket.connect(lobby_address)
    print("sending hello")
    my_id = random_id()
    print("my id: {}".format(my_id))
    lobby_socket.send(my_id)
    rep = lobby_socket.recv_multipart()
    secret_code = rep[0]
    map_data = rep[1]
    print("my secret code: {}".format(secret_code))
    print("map data: {}".format(map_data))
    return secret_code, map_data

def play_game():
    control_socket = context.socket(zmq.PUSH);
    state_socket = context.socket(zmq.SUB)
    control_address = "tcp://localhost:{}".format(control_port)
    state_address = "tcp://localhost:{}".format(state_port)
    control_socket.connect(control_address)
    state_socket.connect(control_address)

def main():
    context = zmq.Context()
    secret_code, map_data = connect(context)


if __name__ == '__main__':
    main()
