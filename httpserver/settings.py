import os
import json

with open('/spacerace/config/spacerace.json') as infile:
    config = json.load(infile)

SPACERACE_SERVER = os.environ.get('SPACERACE_SERVER', 'server')
SPACERACE_STATE_PORT = config['stateSocket'].get('port', 5556)
SPACERACE_CONTROL_PORT = config['controlSocket'].get('port', 5557)
SPACERACE_LOBBY_PORT = config['lobbySocket'].get('port', 5558)
