/********************************************************************
  Copyright 2025, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/12/01 06:24
  filename:  ElefantBlasterServer/src/common/settings_manager.hpp

  purpose:   Centralized settings management with auto-save and
             default config fallback
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ELEFANTBLASTERSERVER_SRC_COMMON_SETTINGS_MANAGER_HPP
#define ELEFANTBLASTERSERVER_SRC_COMMON_SETTINGS_MANAGER_HPP
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <aries_base/definitions/macro.hpp>
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
namespace common {
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

class SettingsManager {
 private:
  SettingsManager();
  virtual ~SettingsManager();

 public:
  static SettingsManager* Instance();
  static void CreateInstance();

  bool SetCurrentConfig(const std::string& config_name);

  // Load a single settings file (if not exists, copy from default)
  bool Load(const std::string& config_name);
  // Save a single settings file (in assets/configs/)
  bool Save(const std::string& config_name = "");
  // Load multiple configs at once
  bool LoadList(const std::vector<std::string>& config_names);

  // Check if settings have unsaved changes
  bool IsDirty() const;
  bool IsDirty(const std::string& config_name) const;

  // Get entire settings as JSON
  nlohmann::json GetAll() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = settings_map_.find(current_config_name_);
    if (it == settings_map_.end()) return {};
    return it->second;
  }

  nlohmann::json GetAllFrom(const std::string& config_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = settings_map_.find(config_name);
    if (it == settings_map_.end()) return {};
    return it->second;
  }

  // Set auto-save interval (in seconds, 0 = disable)
  void SetAutoSaveInterval(uint32_t seconds) {
    auto_save_interval_ = seconds;
  }

  // Start/Stop auto-save thread
  void StartAutoSave();
  void StopAutoSave();

  // Logging support ---------------------------------------------------------
  // Initialize logging based on a loaded config (expects logging section)
  bool InitializeLogging(const std::string& config_name = "app.json");
  std::shared_ptr<spdlog::logger> GetLogger(const std::string& profile_name,
                                            const std::string& instance_name,
                                            bool shared = false);

  // Get setting value with default fallback
  template <typename T>
  T Get(const std::string& key, const T& default_value = T()) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = settings_map_.find(current_config_name_);
    if (it == settings_map_.end()) return default_value;
    const auto& j = it->second;
    if (j.contains(key)) {
      try {
        return j.at(key).get<T>();
      } catch (...) {
        return default_value;
      }
    }
    return default_value;
  }

  template <typename T>
  T GetFrom(const std::string& config_name,
            const std::string& key,
            const T& default_value = T()) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = settings_map_.find(config_name);
    if (it == settings_map_.end()) return default_value;
    const auto& j = it->second;
    if (j.contains(key)) {
      try {
        return j.at(key).get<T>();
      } catch (...) {
        return default_value;
      }
    }
    return default_value;
  }

  // Set setting value (marks dirty for auto-save)
  template <typename T>
  void Set(const std::string& key, const T& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& j = settings_map_[current_config_name_];
    j[key] = value;
    file_dirty_[current_config_name_] = true;
  }

  template <typename T>
  void SetIn(const std::string& config_name,
             const std::string& key,
             const T& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& j = settings_map_[config_name];
    j[key] = value;
    file_dirty_[config_name] = true;
  }

  // Get nested value using JSON pointer (e.g., "/network/port")
  template <typename T>
  T GetNested(const std::string& json_pointer,
              const T& default_value = T()) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = settings_map_.find(current_config_name_);
    if (it == settings_map_.end()) return default_value;
    try {
      return it->second.at(nlohmann::json::json_pointer(json_pointer)).get<T>();
    } catch (...) {
      return default_value;
    }
  }

  template <typename T>
  T GetNestedFrom(const std::string& config_name,
                  const std::string& json_pointer,
                  const T& default_value = T()) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = settings_map_.find(config_name);
    if (it == settings_map_.end()) return default_value;
    try {
      return it->second.at(nlohmann::json::json_pointer(json_pointer)).get<T>();
    } catch (...) {
      return default_value;
    }
  }

  // Set nested value using JSON pointer
  template <typename T>
  void SetNested(const std::string& json_pointer, const T& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& j = settings_map_[current_config_name_];
    j[nlohmann::json::json_pointer(json_pointer)] = value;
    file_dirty_[current_config_name_] = true;
  }

  template <typename T>
  void SetNestedIn(const std::string& config_name,
                   const std::string& json_pointer,
                   const T& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& j = settings_map_[config_name];
    j[nlohmann::json::json_pointer(json_pointer)] = value;
    file_dirty_[config_name] = true;
  }

 private:
  // Copy default config to user config location
  bool CopyDefaultConfig(const std::string& config_name);
  bool LoadInternal(const std::string& config_name);
  bool SaveInternal(const std::string& config_name);

  // Auto-save worker thread
  void AutoSaveWorker();

 public:
  static const std::string kSettingApp;
  static const std::string kSettingLogging;
  static const std::string kSettingServer;

 private:
  // settings per file
  std::unordered_map<std::string, nlohmann::json> settings_map_;
  mutable std::mutex mutex_;
  std::unordered_map<std::string, bool> file_dirty_;
  std::string current_config_name_;

  // Auto-save thread
  std::atomic<bool> auto_save_running_{false};
  std::atomic<uint32_t> auto_save_interval_{60};  // Default 60 seconds
  std::unique_ptr<std::thread> auto_save_thread_;

  // Logger
  std::shared_ptr<spdlog::logger> logger_;

  // Paths
  std::filesystem::path default_config_dir_{"assets/configs/default"};
  std::filesystem::path config_dir_{"assets/configs"};

 private:
  struct LoggerProfile {
    bool enabled = true;
    std::string log_name;      // logical display name
    std::string file_name;     // file stem
    spdlog::level::level_enum console_level = spdlog::level::info;
    spdlog::level::level_enum file_level = spdlog::level::debug;
    size_t max_file_size_bytes = 10 * 1024 * 1024;
    size_t max_files = 5;
  };

  std::unordered_map<std::string, LoggerProfile> logger_profiles_; // profile -> config
  std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> logger_cache_; // instance_name -> logger

 private:
  static SettingsManager* instance_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SettingsManager);
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
}  // namespace common
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ELEFANTBLASTERSERVER_SRC_COMMON_SETTINGS_MANAGER_HPP
// -----------------------------------------------------------------------------
