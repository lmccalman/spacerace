import zmq

def main():
    context = zmq.Context()
    control_socket = context.socket(zmq.REQ)
    # control_socket.identity = b'sockid'
    print("connecting...")
    control_socket.connect("tcp://localhost:5557")
    print("connected!")
    print("sending...")
    control_socket.send_multipart([b"i am an id"])
    print("message sent!")
    print("receiving...")
    rep = control_socket.recv_multipart()
    print("message received:", rep)

if __name__ == '__main__':
    main()
