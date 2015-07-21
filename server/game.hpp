#include "types.hpp"
#include "time.hpp"

void collectControlInput(zmq::context_t& context, 
    const std::set<std::string>& players, 
    ControlMatrix& control, std::mutex& mutex, bool& running)
{
  int linger = 0;
  zmq::socket_t socket(context, ZMQ_PULL);
  socket.setsockopt(ZMQ_LINGER, &linger, sizeof(int));
  socket.bind("tcp://*:5558");
  std::vector<std::string> messageTokens;
  
  // build index map
  std::map<std::string,uint> indexMap;
  uint i = 0;
  for (auto const& p : players)
  {
    indexMap[p] = i;
    i++;
  }

  while (running && !interruptedBySignal)
  {
    try
    {
      auto msg = receive(socket);
      // should be of form <id>,<0 or 1>,<-1, 0 or 1>
      boost::split(messageTokens, msg[0], boost::is_any_of(","));
      {
        uint idx = indexMap[messageTokens[0]];
        std::lock_guard<std::mutex> lock(mutex);
        control(idx,0) = float(messageTokens[1] == "1");
        control(idx,1) = float(messageTokens[2] == "1") - float(messageTokens[2] == "-1");
        LOG(INFO) << "Ship " << messageTokens[0] << " enacted control " << control(idx,0) << "," << control(idx,1);
      }
    }
    catch (...)
    {
      LOG(INFO) << "control thread shutting down after catching exception";
      break; 
    }
  }
}
  


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

void runGame(const std::set<std::string>& players, Sockets& sockets, zmq::context_t& context)
{
  
  uint integrationSteps = 10;
  uint targetFPS = 60;
  uint totalGameTimeSeconds = 120;
  float linearDrag = 1.0;
  float rotationalDrag = 1.0;
  float timeStep = 0.0001;

  uint nShips = players.size();
  uint targetMicroseconds = 1000000 / targetFPS;
  StateMatrix state(nShips, STATE_LENGTH);
  initialiseState(state);
  ControlMatrix control = Eigen::MatrixXf::Zero(nShips, 2);
  ControlMatrix controlLocal = Eigen::MatrixXf::Zero(nShips,2);
  bool running = true;
  std::mutex mutex;
  
  std::future<void> controlThread = std::async(std::launch::async, collectControlInput, 
      std::ref(context), 
      std::ref(players),
      std::ref(control),
      std::ref(mutex),
      std::ref(running));
  
  
  auto gameStart = hrclock::now();
  while (running && !interruptedBySignal)
  {
    auto frameStart = hrclock::now();
    broadcastState(state, sockets.state);
    {
      std::lock_guard<std::mutex> lock(mutex);
      controlLocal = control;
    }

    for (uint i=0; i<integrationSteps;i++)
      eulerTimeStep(state, controlLocal, linearDrag, rotationalDrag, timeStep);
    
    //check we don't need to end the game
    running = !hasRoughIntervalPassed(gameStart, totalGameTimeSeconds, targetFPS);

    // make sure we target a particular frame rate
    waitPreciseInterval(frameStart, targetMicroseconds);
  }

  LOG(INFO) << "Waiting for control thread to terminate...";
  controlThread.wait();
  LOG(INFO) << "Control thread terminated";

}

