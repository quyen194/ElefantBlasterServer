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

#include "common/definitions.hpp"
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

  // Check if launched with --restart flag
  bool is_restart_mode = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == ARG_RESTART) {
      is_restart_mode = true;
      break;
    }
  }

  SingleProcess single_process("ElefantBlasterServer.lock", false);

  if (!is_restart_mode) {
    // Try to acquire lock once
    if (!single_process.AcquireLock()) {
      std::cerr << "Failed to acquire lock on normal startup." << std::endl;
      std::cerr << "Another instance of ElefantBlasterServer is already running. Exiting." << std::endl;
      return 1;
    }
  }
  else {
    std::cout << "Restart mode enabled — will retry acquiring the lock" << std::endl;

    bool lock_acquired = false;

    // Retry loop: try 10 times, wait 1 second between attempts
    for (int attempt = 1; attempt <= 10; ++attempt) {
      if (single_process.AcquireLock()) {
        lock_acquired = true;
        std::cout << "Lock acquired successfully after " << attempt << " attempt(s)" << std::endl;
        break;
      }

      std::cout << "Another instance still holding lock (attempt " << attempt
                << "/10), retrying in 1s..." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (!lock_acquired) {
      std::cerr << "Failed to acquire lock after 10 attempts." << std::endl;
      std::cerr << "Cannot restart: another instance is still running. Exiting." << std::endl;
      return 1;
    }
  }

  // Initialize global logger manager
  LoggerManager::CreateInstance();

  // Initialize settings manager
  SettingsManager::CreateInstance();

  logger = SettingsManager::Instance()->GetLogger("app", "entrypoint", true);

  ProcessControl process_control(argc, argv);
  if (!process_control.Create()) {
    logger->error("Failed to create ProcessControl. Exiting.");
    return 1;
  }

  if (!is_restart_mode) {
    logger->info("Elefant Blaster Server App started successfully");
  }
  else {
    logger->info("Elefant Blaster Server App restarted successfully");
  }

  ExitProcessHandle exit_handle([&process_control, logger]() {
    logger->info("ElefantBlasterServer is exiting...");
    process_control.Destroy();
  });

  process_control.RunWorker();

  logger->info("ElefantBlasterServer has exited.");

  process_control.Repawn();

  SettingsManager::DestroyInstance();
  LoggerManager::DestroyInstance();

  return 0;
}
// -----------------------------------------------------------------------------
