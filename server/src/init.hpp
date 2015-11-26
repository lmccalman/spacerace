#pragma once

#include <csignal>
#include <boost/program_options.hpp>
#include <sys/stat.h>
#include <stdlib.h>

namespace po = boost::program_options;

std::atomic<bool> interruptedBySignal;

void handleSignal(int sig)
{
  LOG(INFO) << "signal " << sig << " caught...";
  interruptedBySignal = true;
}

void initialiseSignalHandler()
{
  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);
}

po::variables_map commandLineArgs(int ac, char* av[])
{
  po::options_description desc("spacerace-server options");
  desc.add_options()
    ("help,h", "produce help message")
    ("settings,s", po::value<std::string>()->default_value("spacerace.json"), "path to settings file")
    ("verbose,v", po::bool_switch()->default_value(false), "verbose logging")
  ;
  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, desc), vm);
  po::notify(vm);    
  if (vm.count("help")) 
  {
    std::cout << desc << "\n";
    exit(EXIT_SUCCESS);
  }
  return vm;
}

bool fileExists(const std::string& name)
{
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

json loadSettingsFile(const std::string& filename)
{
  std::ifstream t(filename);
  std::string str((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
  json j = json::parse(str);
  return j;
}
