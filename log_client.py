import zmq
import json

info_port = 5559

def main():
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    socket.setsockopt(zmq.SUBSCRIBE, b"")
    address = "tcp://localhost:{}".format(info_port)
    socket.connect(address)
    while True:
        msg = json.loads(socket.recv().decode())
        print(msg)


if __name__ == '__main__':
    main()
