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
  j["state"] = "running";
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
        {"id",p},
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

void initialiseState(StateMatrix& state, const Map& map)
{
  state = StateMatrix::Random(state.rows(),state.cols());
  state.col(0) = state.col(0).array() + 50;
  state.col(1) = state.col(1).array() + 50;
}

std::string winner(const PlayerSet& players,
                   const StateMatrix& state,
                   const Map& map)
{
  //TODO
  return "";
}

void runGame(PlayerSet& players, 
             ControlData& control, 
             const GameState& gameState, 
             const Map& map, 
             const SimulationParameters& params,
             zmq::socket_t& stateSocket, 
             const json& settings, 
             InfoLogger& logger)
{
  
  uint integrationSteps = settings["simulation"]["integrationSteps"];
  uint targetFPS = settings["simulation"]["targetFPS"];
  uint totalGameTimeSeconds = settings["gameTime"];

  uint nShips = players.ids.size();
  uint targetMicroseconds = 1000000 / targetFPS;
  StateMatrix state(nShips, STATE_LENGTH);
  initialiseState(state, map);
  ControlMatrix inputs = control.inputs;
  bool running = true;
  
  auto gameStart = hrclock::now();
  logger("game", "status","{\"state\":\"running\"}"_json);
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
      rk4TimeStep(state, inputs, params, map);
    
    //check we don't need to end the game
    bool timeout = hasRoughIntervalPassed(gameStart, totalGameTimeSeconds, targetFPS);
    bool raceWon = winner(players, state, map) != "";
    running = (!timeout) && (!raceWon);

    // make sure we target a particular frame rate
    waitPreciseInterval(frameStart, targetMicroseconds);
  }
  // Game over, so tell the clients
  logger("game", "status","{\"state\":\"finished\"}"_json);
  send(stateSocket, {gameState.name,"{\"state\":\"finished\"}"});

}

