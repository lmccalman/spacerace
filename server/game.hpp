#include "types.hpp"
#include "time.hpp"


void broadcastState(const PlayerSet& players, const StateMatrix& state, const ControlData& control, zmq::socket_t& socket)
{
  json j = json::array();
  for (auto const& p : players.ids)
  {
    uint idx = control.idx.at(players.secretKeys.at(p));
    float x = state(idx, 0);
    float y = state(idx, 1);
    float vx = state(idx, 2);
    float vy = state(idx, 3);
    float theta = state(idx, 4);
    float omega = state(idx, 5);
    float Tl = control.inputs(idx, 0);
    float Tr = control.inputs(idx, 1);
    j.push_back({
        {"x", x},
        {"y", y},
        {"vx", vx},
        {"vy", vy},
        {"theta", theta},
        {"omega", omega},
        {"Tl", Tl},
        {"Tr", Tr}
        });
  }
  send(socket, {j.dump()});
}

void initialiseState(StateMatrix& state)
{
  state = StateMatrix::Zero(state.rows(),state.cols());
}

void runGame(PlayerSet& players, ControlData& control, const Map& map,
    zmq::socket_t& stateSocket, const json& settings, InfoLogger& logger)
{
  
  uint integrationSteps = settings["integrationSteps"];
  float timeStep = settings["timeStep"];
  uint targetFPS = settings["targetFPS"];
  uint totalGameTimeSeconds = settings["gameTime"];
  float linearDrag = settings["physics"]["linearDrag"];
  float rotationalDrag = settings["physics"]["rotationalDrag"];
  float linearThrust = settings["physics"]["linearThrust"];
  float rotationalThrust = settings["physics"]["rotationalThrust"];

  uint nShips = players.ids.size();
  uint targetMicroseconds = 1000000 / targetFPS;
  StateMatrix state(nShips, STATE_LENGTH);
  initialiseState(state);
  ControlMatrix inputs = control.inputs;
  bool running = true;
  
  auto gameStart = hrclock::now();
  logger({"INFO","Game starting now!"});
  while (running && !interruptedBySignal)
  {
    auto frameStart = hrclock::now();
    //get control inputs from control thread
    broadcastState(players, state, control, stateSocket);
    {
      std::lock_guard<std::mutex> lock(control.mutex);
      inputs = control.inputs;
    }

    for (uint i=0; i<integrationSteps;i++)
      eulerTimeStep(state, inputs, linearDrag, rotationalDrag, timeStep);
    
    //check we don't need to end the game
    running = !hasRoughIntervalPassed(gameStart, totalGameTimeSeconds, targetFPS);

    // make sure we target a particular frame rate
    waitPreciseInterval(frameStart, targetMicroseconds);
  }
  // Game over, so tell the clients
  logger({"INFO","Game over!"});
  send(stateSocket, {"GAME OVER", "some_json_stats?"});

}

