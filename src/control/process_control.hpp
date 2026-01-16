/********************************************************************
  Copyright 2025, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/11/17 16:57
  filename:  ElefantBlasterServer/src/control/process_control.hpp

  purpose:   Header file for the main application class
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ELEFANT_BLASTER_SERVER_CONTROL_PROCESS_CONTROL_HPP
#define ELEFANT_BLASTER_SERVER_CONTROL_PROCESS_CONTROL_HPP
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <cstdint>
#include <string>

#include <spdlog/spdlog.h>

#include <aries_base/definitions/macro.hpp>
#include <aries_base/process/ipc/mpmc_bounded_queue/ipc_server.hpp>

#include "common/settings_manager.hpp"
#include "network/admin/admin_server.hpp"
#include "storage/db_manager.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
using namespace aries_base::process::ipc::mpmc_bounded_queue;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

class ProcessControl : public IpcServer {
 public:
  ProcessControl(int argc, char *argv[]);
  ~ProcessControl();

  void Create();
  void Repawn();

 private:
  virtual void OnDestroy();
  virtual void OnMessageReceived(uint16_t id, const uint8_t* message, uint32_t length);

 private:
  DBManager db_manager_;
  AdminServer admin_server_;

  bool is_restarting_;
  std::vector<std::string> startup_args_;

 private:
  SettingsManager* settings_;
  std::shared_ptr<spdlog::logger> logger_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProcessControl);
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ELEFANT_BLASTER_SERVER_CONTROL_PROCESS_CONTROL_HPP
// -----------------------------------------------------------------------------
