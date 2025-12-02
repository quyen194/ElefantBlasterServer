/********************************************************************
  Copyright 2025, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/11/30 08:04
  filename:  ElefantBlasterServer/src/control/process_control.cpp

  purpose:
*********************************************************************/


// -----------------------------------------------------------------------------
#include <spdlog/spdlog.h>

#include <aries_base/logger/logger_manager.hpp>
#include <aries_base/process/thread_pool/thread_pool.hpp>

#include "common/settings_manager.hpp"
#include "control/process_control.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
using namespace aries_base::common;
using namespace aries_base::process;
using namespace common;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

ProcessControl::ProcessControl()
    : settings_(SettingsManager::Instance()) {
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
}
// -----------------------------------------------------------------------------
