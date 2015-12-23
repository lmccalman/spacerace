#include "types.hpp"
#include "time.hpp"
#include <math.h>
#include "argsort.hpp"
/* #include <unordered_map> */


std::string gameName(uint n)
{
  return "game" + std::to_string(n);
}

void broadcastState(const PlayerSet& players, const StateMatrix& state, 
    const GameState& gameState, const ControlData& control, 
    const SimulationParameters& params, const std::string& mapName, zmq::socket_t& socket)
{
  json j; 
  j["name"] = gameState.name;
  j["map"] = mapName;
  j["state"] = "running";
  j["data"] = json::array();
  float mapScale = params.mapScale;
  for (auto const& p : players.fromId)
  {
    uint idx = control.idx.at(p.second.secretKey);
    float x = state(idx, 0) * mapScale;
    float y = state(idx, 1) * mapScale;
    float vx = state(idx, 2) * mapScale;
    float vy = state(idx, 3) * mapScale;
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

void broadcastNextState(const std::string& gameName, const std::string& mapName, zmq::socket_t& socket)
{
  json j; 
  j["name"] = gameName;
  j["map"] = mapName;
  j["state"] = "queued";
  send(socket, {gameName, j.dump()});
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
    state(i, 4) = dist(gen) * 2.0 * M_PI; //ensure we're between 0 and 2PI
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


void playerScore(const StateMatrix& state,
                 const ControlData& control,
                 const Map& map,
                 const SimulationParameters& params,
                 GameStats& gameStats)
{
  std::vector<float> distance(state.rows());

  for (uint i=0; i<state.rows();i++)
  {
    std::pair<uint,uint> coords = indices(state(i,0), state(i,1), map, params);
    distance[i] = map.endDistance(coords.first, coords.second);
    gameStats.playerDists[control.ids.at(i)] = distance[i];
  }

  std::vector<int> indices = argsort(distance);
  std::vector<int> ranks(indices.size());
  for (int i=0;i<indices.size();i++)
    ranks[indices[i]] = i;
  for (uint i=0; i<state.rows();i++)
  {
    gameStats.playerRanks[control.ids.at(i)] = ranks[i];
  }

}

void updateRanks(const PlayerSet& players,
                 const ControlData& control,
                 const Map& map,
                 GameStats& gameStats)
{

  // Get number of players in a team
  std::map<std::string, unsigned int> playersPerTeam;
  for (auto const& pair : players.fromId)
  {
    const Player& player = pair.second;

    if (playersPerTeam.count(player.team))
        playersPerTeam[player.team]++;
    else
        playersPerTeam[player.team] = 1;
  }


  // Tally scores
  for (auto const& pair : players.fromId)
  {
    const Player& player = pair.second;
    float score = (map.maxDistance - gameStats.playerDists[player.id])
        / map.maxDistance * 100.0;
    if (gameStats.totalPlayerScore.count(player.id))
      gameStats.totalPlayerScore[player.id] += score;
    else
      gameStats.totalPlayerScore[player.id] = score;
       
    if (gameStats.totalTeamScore.count(player.team))
      gameStats.totalTeamScore[player.team] += score 
          / (float) playersPerTeam[player.team];
    else
      gameStats.totalTeamScore[player.id] = score
          / (float) playersPerTeam[player.team];
  }
}

void runGame(PlayerSet& players, 
             ControlData& control, 
             const GameState& gameState, 
             const Map& map, 
             const SimulationParameters& params,
             GameStats& gameStats,
             zmq::socket_t& stateSocket, 
             const json& settings, 
             InfoLogger& logger,
             const std::string& nextGameName,
             const std::string& nextMapName)
{
  
  uint targetFPS = settings["simulation"]["targetFPS"];
  uint totalGameTimeSeconds = settings["gameTime"];
  uint integrationSteps = uint(1. / float(targetFPS) / params.timeStep);

  // Make sure the per-game stats are cleared
  gameStats.playerRanks.clear();
  gameStats.playerDists.clear();

  uint nShips = players.fromId.size();
  uint targetMicroseconds = 1000000 / targetFPS;
  StateMatrix state(nShips, STATE_LENGTH);
  initialiseState(state, map, params);
  ControlMatrix inputs = control.inputs;
  bool running = true;
  unsigned int fpscounter = 0;
  
  auto gameStart = hrclock::now();
  logger("game", "status",
        {{"state","running"},{"map",map.name}, {"game",gameState.name}}); 
  while (running && !interruptedBySignal)
  {
    auto frameStart = hrclock::now();

    // Threadsafe copy of control inputs (unlock managed by scope)
    {
      std::lock_guard<std::mutex> lock(control.mutex);
      inputs = control.inputs;
    }

    for (uint i=0; i<integrationSteps;i++)
      rk4TimeStep(state, inputs, params, map);

    // Calculate game stats every second
    fpscounter++;
    if (fpscounter >= targetFPS)
    {
        fpscounter = 0;
        uint elapsedTime = ch::duration_cast<ch::seconds>(hrclock::now() - gameStart).count();
        uint timeRemaining = totalGameTimeSeconds - elapsedTime;
        playerScore(state, control, map, params, gameStats);
        logger("game", "status",
            {{"state","running"},{"map",map.name}, {"game",gameState.name},
            {"time_remaining", timeRemaining},
             {"ranking", gameStats.playerRanks}}); 
        // Make sure the people in the lobby know whats going on
        broadcastNextState(nextGameName, nextMapName, stateSocket);
    }
    
    //check we don't need to end the game
    bool timeout = hasRoughIntervalPassed(gameStart, totalGameTimeSeconds, targetFPS);
    bool raceWon = winner(players, state,control, map, params) != "";
    running = (!timeout) && (!raceWon);

    // get control inputs from control thread
    broadcastState(players, state, gameState, control, params, map.name, stateSocket);


    // make sure we target a particular frame rate
    waitPreciseInterval(frameStart, targetMicroseconds);
  }

  // Game over, so tell the clients
  playerScore(state, control, map, params, gameStats); // get final score
  updateRanks(players, control,map, gameStats);
  logger("game", "status",
         {{"state","finished"},{"map",map.name}, {"game",gameState.name},
          {"ranking", gameStats.playerRanks}, {"totalScore", gameStats.totalPlayerScore},
          {"teamScore", gameStats.totalTeamScore}}); 
  send(stateSocket, {gameState.name,"{\"state\":\"finished\"}"});

}

