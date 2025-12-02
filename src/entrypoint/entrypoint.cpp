/********************************************************************
  Copyright 2014, Cong Quyen Knight. All rights reserved

  project:   Aries Games Project
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/11/15 05:13
  filename:  AriesGames\BomberManServer\entrypoint\entrypoint.cpp

  purpose:
*********************************************************************/


// -----------------------------------------------------------------------------
#include <aries_base/logger/logger_manager.hpp>

#include "common/settings_manager.hpp"
#include "control/process_control.hpp"
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#if defined(_WIN32)
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#endif  // _WIN32
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
using namespace aries_base::common;
using namespace common;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  // Initialize global logger manager
  LoggerManager::CreateInstance();

  // Initialize settings manager
  SettingsManager::CreateInstance();

  ProcessControl process_control;

  return 0;
}
// -----------------------------------------------------------------------------
