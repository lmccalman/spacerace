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
  for (auto const& p : players.fromId)
  {
    uint idx = control.idx.at(p.second.secretKey);
    float x = state(idx, 0);
    float y = state(idx, 1);
    float vx = state(idx, 2);
    float vy = state(idx, 3);
    float theta = state(idx, 4);
    float omega = state(idx, 5);
    float Tl = control.inputs(idx, 0);
    float Tr = control.inputs(idx, 1);
    j["data"].push_back({
        {"id",p.first},
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

void initialiseState(StateMatrix& state, const Map& map, const SimulationParameters& params)
{
  //TODO Al?
}

std::string winner(const PlayerSet& players,
                   const StateMatrix& state,
                   const Map& map,
                   const SimulationParameters& params)
{
  //TODO Al?
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

  uint nShips = players.fromId.size();
  uint targetMicroseconds = 1000000 / targetFPS;
  StateMatrix state(nShips, STATE_LENGTH);
  initialiseState(state, map, params);
  ControlMatrix inputs = control.inputs;
  bool running = true;
  
  auto gameStart = hrclock::now();
  logger("game", "status",
        {{"state","running"},{"map",map.name}, {"game",gameState.name}}); 
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

    //TODO send an info message with who is in the lead etc etc.
    
    //check we don't need to end the game
    bool timeout = hasRoughIntervalPassed(gameStart, totalGameTimeSeconds, targetFPS);
    bool raceWon = winner(players, state, map, params) != "";
    running = (!timeout) && (!raceWon);

    // make sure we target a particular frame rate
    waitPreciseInterval(frameStart, targetMicroseconds);
  }
  // Game over, so tell the clients
  logger("game", "status",
        {{"state","finished"},{"map",map.name}, {"game",gameState.name}}); 
  send(stateSocket, {gameState.name,"{\"state\":\"finished\"}"});

}

