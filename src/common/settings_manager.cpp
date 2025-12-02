/********************************************************************
  Copyright 2025, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/12/01
  filename:  ElefantBlasterServer/src/common/settings_manager.cpp

  purpose:
*********************************************************************/


// -----------------------------------------------------------------------------
#include <fstream>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <aries_base/logger/logger_manager.hpp>

#include "common/settings_manager.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
namespace common {
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
using namespace aries_base::common;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
SettingsManager* SettingsManager::instance_ = nullptr;
const std::string SettingsManager::kSettingApp = "app.json";
const std::string SettingsManager::kSettingLogging = "logging.json";
const std::string SettingsManager::kSettingServer = "server.json";
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

SettingsManager::SettingsManager() {
  // Creation temporary logger
  logger_ = LoggerManager::Instance()->CreateLoggerWithSharedFile(
      "SettingsManager", "startup.json.log");

  Load("logging.json");
  InitializeLogging("logging.json");

  // Replace temporary logger with real one
  logger_->flush();
  spdlog::drop("SettingsManager");
  logger_ = GetLogger("app", "SettingsManager", true);

  Load(kSettingApp);
  Load(kSettingServer);

  bool auto_save_enabled = GetNestedFrom<bool>(kSettingApp, "/setting_auto_save/enabled", true);
  if (auto_save_enabled) {
    auto_save_interval_ = GetNestedFrom<int>(kSettingApp, "/setting_auto_save/interval", 60);
    StartAutoSave();
  }
}
// -----------------------------------------------------------------------------

SettingsManager::~SettingsManager() {
  StopAutoSave();
  // Save any dirty files
  for (const auto& [name, dirty] : file_dirty_) {
    if (dirty) {
      logger_->info("Auto-saving settings file '{}' on shutdown", name);
      SaveInternal(name);
    }
  }
}
// -----------------------------------------------------------------------------

SettingsManager* SettingsManager::Instance() {
  if (!instance_) {
    CreateInstance();
  }
  return instance_;
}
// -----------------------------------------------------------------------------

void SettingsManager::CreateInstance() {
  if (!instance_) {
    instance_ = new SettingsManager();
  }
}
// -----------------------------------------------------------------------------

bool SettingsManager::SetCurrentConfig(const std::string& config_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = settings_map_.find(config_name);
  if (it == settings_map_.end()) {
    logger_->warn("SetCurrentConfig: Config '{}' not loaded", config_name);
    return false;
  }
  current_config_name_ = config_name;
  logger_->info("SetCurrentConfig: Current config set to '{}'", config_name);
  return true;
}
// -----------------------------------------------------------------------------

bool SettingsManager::Load(const std::string& config_name) {
  return LoadInternal(config_name);
}
// -----------------------------------------------------------------------------

bool SettingsManager::Save(const std::string& config_name) {
  if (config_name.empty()) {
    return SaveInternal(current_config_name_);
  }
  return SaveInternal(config_name);
}
// -----------------------------------------------------------------------------

bool SettingsManager::LoadList(const std::vector<std::string>& config_names) {
  bool ok = true;
  for (const auto& name : config_names)
    ok &= LoadInternal(name);
  return ok;
}
// -----------------------------------------------------------------------------

bool SettingsManager::IsDirty() const {
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& [k,v] : file_dirty_)
    if (v) return true;
  return false;
}
// -----------------------------------------------------------------------------

bool SettingsManager::IsDirty(const std::string& config_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = file_dirty_.find(config_name);
  return it != file_dirty_.end() && it->second;
}
// -----------------------------------------------------------------------------

bool SettingsManager::LoadInternal(const std::string& config_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  current_config_name_ = config_name; // switch current
  std::filesystem::path config_path = config_dir_ / config_name;

  if (!std::filesystem::exists(config_path)) {
    logger_->warn("Config file '{}' not found, copying default", config_path.string());
    if (!CopyDefaultConfig(config_name)) {
      logger_->error("Failed to copy default for '{}'", config_name);
      return false;
    }
  }

  try {
    std::ifstream file(config_path);
    if (!file.is_open()) {
      logger_->error("Failed to open '{}" , config_path.string());
      return false;
    }
    nlohmann::json j; file >> j;
    settings_map_[config_name] = std::move(j);
    file_dirty_[config_name] = false;
    logger_->info("Loaded settings '{}' (keys: {})", config_name, settings_map_[config_name].size());
    return true;
  } catch (const std::exception& e) {
    logger_->error("Parse error in '{}': {}", config_name, e.what());
    return false;
  }
}
// -----------------------------------------------------------------------------

bool SettingsManager::SaveInternal(const std::string& config_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = settings_map_.find(config_name);
  if (it == settings_map_.end()) {
    logger_->warn("Requested save for unknown config '{}'", config_name);
    return false;
  }
  std::filesystem::path config_path = config_dir_ / config_name;
  try {
    std::filesystem::create_directories(config_dir_);
    std::ofstream file(config_path);
    if (!file.is_open()) {
      logger_->error("Failed opening '{}' for write", config_path.string());
      return false;
    }
    file << it->second.dump(4);
    file_dirty_[config_name] = false;
    logger_->info("Saved settings '{}'", config_name);
    return true;
  } catch (const std::exception& e) {
    logger_->error("Save error '{}' : {}", config_name, e.what());
    return false;
  }
}
// -----------------------------------------------------------------------------

bool SettingsManager::CopyDefaultConfig(const std::string& config_name) {
  std::filesystem::path default_path = default_config_dir_ / config_name;
  std::filesystem::path target_path = config_dir_ / config_name;

  if (!std::filesystem::exists(default_path)) {
    logger_->error("Default config not found: {}", default_path.string());
    return false;
  }

  try {
    // Create target directory
    std::filesystem::create_directories(config_dir_);

    // Copy file
    std::filesystem::copy_file(
        default_path, target_path,
        std::filesystem::copy_options::overwrite_existing);

    logger_->info("Copied default config: {} -> {}", default_path.string(),
                  target_path.string());
    logger_->debug(R"({{"event":"config_copy","from":"{}","to":"{}"}})",
                   default_path.string(), target_path.string());

    return true;
  } catch (const std::exception& e) {
    logger_->error("Failed to copy default config: {}", e.what());
    return false;
  }
}
// -----------------------------------------------------------------------------

void SettingsManager::StartAutoSave() {
  if (auto_save_running_) {
    logger_->warn("Auto-save already running");
    return;
  }

  auto_save_running_ = true;
  auto_save_thread_ =
      std::make_unique<std::thread>(&SettingsManager::AutoSaveWorker, this);

  logger_->info("Auto-save started (interval: {}s)", auto_save_interval_.load());
  logger_->debug(R"({{"event":"autosave_start","interval":{}}})",
                 auto_save_interval_.load());
}
// -----------------------------------------------------------------------------

void SettingsManager::StopAutoSave() {
  if (!auto_save_running_) {
    return;
  }

  auto_save_running_ = false;
  if (auto_save_thread_ && auto_save_thread_->joinable()) {
    auto_save_thread_->join();
  }

  logger_->info("Auto-save stopped");
  logger_->debug(R"({{"event":"autosave_stop"}})");
}
// -----------------------------------------------------------------------------

void SettingsManager::AutoSaveWorker() {
  while (auto_save_running_) {
    uint32_t interval = auto_save_interval_.load();

    // Sleep in chunks to be responsive to stop signal
    for (uint32_t i = 0; i < interval && auto_save_running_; ++i) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (!auto_save_running_) {
      break;
    }

    // Check if dirty and save
    // Save all dirty configs
    std::vector<std::string> to_save;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (const auto& [name, dirty] : file_dirty_)
        if (dirty)
          to_save.push_back(name);
    }
    for (const auto& n : to_save) {
      logger_->debug("Auto-saving '{}'", n);
      SaveInternal(n);
    }
  }
}
// -----------------------------------------------------------------------------

bool SettingsManager::InitializeLogging(const std::string& config_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = settings_map_.find(config_name);
  if (it == settings_map_.end()) {
    logger_->error("Cannot initialize logging: config '{}' not loaded", config_name);
    return false;
  }
  const auto& root = it->second;

  // Parse profiles
  for (auto& [key, values] : root.items()) {
    if (!values.is_object()) continue;
    LoggerProfile profile;
    profile.enabled = values.value("enabled", true);
    profile.log_name = values.value("log_name", key);
    profile.file_name = values.value("file_name", key);
    profile.console_level = spdlog::level::from_str(values.value("console_level", "info"));
    profile.file_level = spdlog::level::from_str(values.value("file_level", "debug"));
    size_t sz_mb = static_cast<size_t>(values.value("max_file_size_mb", 10));
    profile.max_file_size_bytes = sz_mb * 1024ull * 1024ull;
    profile.max_files = static_cast<size_t>(values.value("max_files", 5));
    logger_profiles_[key] = std::move(profile);
  }

  // Aggregate logger (profile 'all') if exists
  auto aggIt = logger_profiles_.find("all");
  if (aggIt != logger_profiles_.end() && aggIt->second.enabled) {
    const auto& profile = aggIt->second;

    LoggerManager::Instance()->CreateAggregateLogger(
        profile.file_name,
        profile.file_level,
        profile.max_file_size_bytes,
        profile.max_files);
  }

  logger_->info("Logging initialized (profiles: {})", logger_profiles_.size());
  return true;
}
// -----------------------------------------------------------------------------

std::shared_ptr<spdlog::logger> SettingsManager::GetLogger(
    const std::string& profile_name,
    const std::string& instance_name,
    bool shared) {

  // Return cached
  if (auto it = logger_cache_.find(instance_name); it != logger_cache_.end()) return it->second;
  std::lock_guard<std::mutex> lock(mutex_);
  auto pIt = logger_profiles_.find(profile_name);
  if (pIt == logger_profiles_.end() || !pIt->second.enabled) {
    logger_->warn("Requested logger profile '{}' not found or disabled", profile_name);
    return logger_; // fallback
  }
  const auto& profile = pIt->second;

  auto new_logger = shared ? LoggerManager::Instance()->CreateLoggerWithSharedFile(
                                instance_name,
                                profile.file_name,
                                profile.console_level,
                                profile.file_level,
                                profile.max_file_size_bytes,
                                profile.max_files)
                           : LoggerManager::Instance()->CreateClassLogger(
                                instance_name,
                                profile.file_name,
                                profile.console_level,
                                profile.file_level,
                                profile.max_file_size_bytes,
                                profile.max_files);
  logger_cache_[instance_name] = new_logger;
  return new_logger;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
}  // namespace common
// -----------------------------------------------------------------------------
