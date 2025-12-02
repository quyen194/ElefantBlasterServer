/********************************************************************
  Copyright 2025, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/11/17 16:57
  filename:  ElefantBlasterServer/src/control/process_control.hpp

  purpose:
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ELEFANTBLASTERSERVER_SRC_CONTROL_PROCESS_CONTROL_HPP
#define ELEFANTBLASTERSERVER_SRC_CONTROL_PROCESS_CONTROL_HPP
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <cstdint>
#include <string>

#include <spdlog/spdlog.h>

#include <aries_base/definitions/macro.hpp>
#include <aries_base/process/ipc/mpmc_bounded_queue/ipc_server.hpp>

#include "common/settings_manager.hpp"
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
using namespace aries_base::process::ipc::mpmc_bounded_queue;
using namespace common;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

class ProcessControl : public IpcServer {
 public:
  ProcessControl();
  ~ProcessControl();

  void Create();

 private:
  SettingsManager* settings_;

 private:
  std::shared_ptr<spdlog::logger> logger_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProcessControl);
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ELEFANTBLASTERSERVER_SRC_CONTROL_PROCESS_CONTROL_HPP
// -----------------------------------------------------------------------------
