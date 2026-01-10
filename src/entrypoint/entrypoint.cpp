/********************************************************************
  Copyright 2014, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/11/15 05:13
  filename:  ElefantBlaster/ElefantBlasterServer/entrypoint/entrypoint.cpp

  purpose:   Application entry point
*********************************************************************/


// -----------------------------------------------------------------------------
#include <aries_base/logger/logger_manager.hpp>
#include <aries_base/process/thread_pool/thread_pool.hpp>
#include <aries_base/process/utils/exit_process_handle.hpp>
#include <aries_base/process/utils/single_process.hpp>

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
using namespace aries_base::process;
using namespace aries_base::process::utils;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  std::shared_ptr<spdlog::logger> logger;

  SingleProcess single_process("ElefantBlasterServer.lock");
  if (single_process.AnotherInstanceIsRunning()) {
    // Another instance is already running
    std::cerr << "Another instance of ElefantBlasterServer is already running." << std::endl;
    return 1;
  }

  // Initialize global logger manager
  LoggerManager::CreateInstance();

  // Initialize settings manager
  SettingsManager::CreateInstance();

  logger = SettingsManager::Instance()->GetLogger("app", "entrypoint", true);

  ProcessControl process_control;
  process_control.Create();

  ExitProcessHandle exit_handle([&process_control, logger]() {
    logger->info("ElefantBlasterServer is exiting...");
    process_control.Destroy();
  });

  process_control.RunWorker();

  logger->info("ElefantBlasterServer has exited.");

  SettingsManager::DestroyInstance();
  LoggerManager::DestroyInstance();

  return 0;
}
// -----------------------------------------------------------------------------
