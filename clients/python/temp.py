# import zmq
import client

c = client.Client('localhost', 5556, 5557, 5558)

s = c.register('ship_name', 'team_name')

while True:
    s.theta
    s.omega
    
    s.x
    x.y
    
    s.vx
    s.vy

    s.Tl
    s.Tr

    s.game.state
    s.game.participants[0].x
    s.game.participant

    s.send(1, 1)
    
