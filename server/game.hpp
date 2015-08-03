#include "types.hpp"
#include "time.hpp"
#include <math.h>


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

void initialiseState(StateMatrix& state, const Map& map, 
        const SimulationParameters& params)
{
  static std::default_random_engine gen;
  static std::uniform_real_distribution<float> dist(0.0, 1.0);

  std::vector<std::pair<uint,uint>> startPixels;
  std::copy(map.start.begin(), map.start.end(), std::back_inserter(startPixels));
  std::random_shuffle(startPixels.begin(), startPixels.end());
  uint pixelIdx = 0;
  for (uint i=0; i<state.rows();i++)
  {
    auto coords = startPixels[pixelIdx];
    state(i, 0) = (float(coords.second) + dist(gen)) / params.mapScale;
    state(i, 1) = (float(coords.first) + dist(gen)) / params.mapScale;
    state(i, 2) = 0.0;
    state(i, 3) = 0.0;
    state(i, 4) = dist(gen) * 2.0 * M_PI;
    state(i, 5) = 0.0;
    pixelIdx = (pixelIdx + 1) % startPixels.size();
  }
}

std::string winner(const PlayerSet& players,
                   const StateMatrix& state,
                   const ControlData& control,
                   const Map& map,
                   const SimulationParameters& params)
{
  std::string winnerName = "";
  for (uint i=0; i<state.rows();i++)
  {
    std::pair<uint,uint> coords = indices(state(i,0), state(i,1), map, params);
    bool winner = map.finish.count(coords);
    if (winner)
    {
      winnerName = control.ids.at(i);
      break;
    }
  }
  return winnerName;
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
  
  uint targetFPS = settings["simulation"]["targetFPS"];
  uint totalGameTimeSeconds = settings["gameTime"];
  uint integrationSteps = uint(1. / float(targetFPS) / params.timeStep);

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
    bool raceWon = winner(players, state,control, map, params) != "";
    running = (!timeout) && (!raceWon);

    // make sure we target a particular frame rate
    waitPreciseInterval(frameStart, targetMicroseconds);
  }
  // Game over, so tell the clients
  logger("game", "status",
        {{"state","finished"},{"map",map.name}, {"game",gameState.name}}); 
  send(stateSocket, {gameState.name,"{\"state\":\"finished\"}"});

}

