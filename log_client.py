import zmq

info_port = 5559

def main():
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    socket.setsockopt(zmq.SUBSCRIBE, b"")
    address = "tcp://localhost:{}".format(info_port)
    socket.connect(address)
    while True:
        msg = socket.recv_multipart()
        print("LOG: ", [k.decode() for k in msg])


if __name__ == '__main__':
    main()
