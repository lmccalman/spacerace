import zmq

info_port = 5559

def main():
    with zmq.Context() as context:
        with context.socket(zmq.SUB) as socket:
            socket.setsockopt_string(zmq.SUBSCRIBE, "")
            address = "tcp://localhost:{}".format(info_port)
            socket.connect(address)
            while True:
                msg = socket.recv_json()
                print(msg)


if __name__ == '__main__':
    main()
