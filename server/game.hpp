#include "types.hpp"
#include "time.hpp"


std::string gameName(uint n)
{
  return "game" + std::to_string(n);
}

void broadcastState(const PlayerSet& players, const StateMatrix& state, 
    const GameState& gameState, const ControlData& control, 
    zmq::socket_t& socket)
{
  json j; 
  j["status"] = "IN GAME";
  j["data"] = json::array();
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
    LOG(INFO) << p << ":" << state.row(idx) << "\n" << control.inputs.row(idx);
    j["data"].push_back({
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
  send(socket, {gameState.name, j.dump()});
}

void initialiseState(StateMatrix& state)
{
  state = StateMatrix::Random(state.rows(),state.cols());
  state.col(0) = state.col(0).array() + 50;
  state.col(1) = state.col(1).array() + 50;
}

void runGame(PlayerSet& players, ControlData& control, 
    const GameState& gameState, const Map& map, zmq::socket_t& stateSocket, 
    const json& settings, InfoLogger& logger)
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
    broadcastState(players, state, gameState, control, stateSocket);
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
  send(stateSocket, {gameState.name,"{\"status\":\"GAME OVER\"}"});

}

