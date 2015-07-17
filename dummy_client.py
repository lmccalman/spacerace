import zmq

def main():
    context = zmq.Context()
    control_socket = context.socket(zmq.REQ)


if __name__ == '__main__':
    main()
