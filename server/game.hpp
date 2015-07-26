#include "types.hpp"
#include "time.hpp"


void broadcastState(const StateMatrix& state, zmq::socket_t& socket)
{
  // LOG(INFO) << state;
  //convert the state into JSON
  //send that shit!
}

void initialiseState(StateMatrix& state)
{
  state = StateMatrix::Zero(state.rows(),state.cols());
}

void runGame(PlayerData& playerData, MapData& mapData)
{
  
  // uint integrationSteps = 10;
  // uint targetFPS = 60;
  // uint totalGameTimeSeconds = 120;
  // float linearDrag = 1.0;
  // float rotationalDrag = 1.0;
  // float timeStep = 0.0001;

  // uint nShips = players.size();
  // uint targetMicroseconds = 1000000 / targetFPS;
  // StateMatrix state(nShips, STATE_LENGTH);
  // initialiseState(state);
  // ControlMatrix control = Eigen::MatrixXf::Zero(nShips, 2);
  // ControlMatrix controlLocal = Eigen::MatrixXf::Zero(nShips,2);
  // bool running = true;
  // std::mutex mutex;
  
  // auto gameStart = hrclock::now();
  // while (running && !interruptedBySignal)
  // {
  //   auto frameStart = hrclock::now();
  //   broadcastState(state, sockets.state);
  //   {
  //     std::lock_guard<std::mutex> lock(mutex);
  //     controlLocal = control;
  //   }

  //   for (uint i=0; i<integrationSteps;i++)
  //     eulerTimeStep(state, controlLocal, linearDrag, rotationalDrag, timeStep);
    
  //   //check we don't need to end the game
  //   running = !hasRoughIntervalPassed(gameStart, totalGameTimeSeconds, targetFPS);

  //   // make sure we target a particular frame rate
  //   waitPreciseInterval(frameStart, targetMicroseconds);
  // }

  // LOG(INFO) << "Waiting for control thread to terminate...";
  // controlThread.wait();
  // LOG(INFO) << "Control thread terminated";

}

