/********************************************************************
  Copyright 2025, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/11/30 08:04
  filename:  ElefantBlasterServer/src/control/process_control.cpp

  purpose:   Implementation file for the main application class
*********************************************************************/


// -----------------------------------------------------------------------------
#include <spdlog/spdlog.h>

#include <aries_base/logger/logger_manager.hpp>
#include <aries_base/process/thread_pool/thread_pool.hpp>

#include "common/definitions.hpp"
#include "common/events.hpp"
#include "common/settings_manager.hpp"
#include "control/process_control.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
using namespace aries_base::common;
using namespace aries_base::process;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

ProcessControl::ProcessControl(int argc, char *argv[])
    : settings_(SettingsManager::Instance()),
      admin_server_(&db_manager_),
      is_restarting_(false) {
  for (int i = 0; i < argc; i++) {
    std::string arg(argv[i]);
    if (arg != ARG_RESTART) {
      startup_args_.push_back(arg);
    }
  }

  logger_ = settings_->GetLogger("app", "ProcessControl", true);
}
// -----------------------------------------------------------------------------

ProcessControl::~ProcessControl() {
}
// -----------------------------------------------------------------------------

void ProcessControl::Create() {
  settings_->SetCurrentConfig(SettingsManager::kSettingApp);

  // Thread pool settings from app config (section thread_pool)
  uint32_t idle_threads = settings_->GetNested<uint32_t>("/thread_pool/idle_threads", 4);
  uint32_t max_threads = settings_->GetNested<uint32_t>("/thread_pool/max_threads", 16);
  logger_->info("ProcessControl: Initializing ThreadPool... (idle_threads={}, max_threads={})", idle_threads, max_threads);
  ThreadPool::CreateInstance(idle_threads, max_threads);

  // IPC settings from app config (section ipc)
  uint32_t ipc_buffer_size = settings_->GetNested<uint32_t>("/ipc/buffer_size", 1024);
  uint32_t ipc_block_count = settings_->GetNested<uint32_t>("/ipc/block_count", 1024);
  logger_->info("ProcessControl: Initializing IPC server... (buffer={}, blocks={})", ipc_buffer_size, ipc_block_count);
  IpcServer::SetName("ElefantBlasterServer_ProcessControl");
  IpcServer::SetIpcType(IpcType::kMPSC);
  IpcServer::Create(ipc_buffer_size, ipc_block_count);

  logger_->info("IPC server initialized successfully");

  db_manager_.LoadConfig();
  db_manager_.CreateDatabase();

  db_manager_.ConnectAsUser();

  admin_server_.IpcClient::SetNameEx(IpcServer::GetName());
  admin_server_.IpcClient::Connect();
  admin_server_.Start();
}
// -----------------------------------------------------------------------------

void ProcessControl::OnDestroy() {
  admin_server_.Stop();
  db_manager_.Disconnect();
}
// -----------------------------------------------------------------------------

void ProcessControl::OnMessageReceived(uint16_t id,
                                       const uint8_t* message,
                                       uint32_t length) {
  switch (id) {
    case EventId::kAdminServer_ShutdownServer: {
      Destroy();
    } break;

    case EventId::kAdminServer_RestartServer: {
      is_restarting_ = true;
      Destroy();
    } break;

    case EventId::kAdminServer_ActiveGameServer: {
    } break;

    case EventId::kAdminServer_DeactiveGameServer: {
    } break;

    case EventId::kAdminServer_DisconnectAllGameClients: {
    } break;
  }
}
// -----------------------------------------------------------------------------

void ProcessControl::Repawn() {
  if (!is_restarting_) {
    return;
  }

  logger_->info("ProcessControl::Repawn: Restarting Elefant Blaster Server ...");

  std::string cmd;

#if defined(_WIN32)
  cmd = "START \"\" \"" + startup_args_[0] + "\"";
#elif defined(__APPLE__) || defined(__linux__)
  cmd = "\"" + startup_args_[0] + "\"";
#endif

  for (int i = 1; i < startup_args_.size(); i++) {
    cmd += " " + startup_args_[i];
  }

  cmd += " " + std::string(ARG_RESTART);

#if defined(__APPLE__) || defined(__linux__)
  cmd += " &";
#endif

  std::system(cmd.c_str());
}
// -----------------------------------------------------------------------------
