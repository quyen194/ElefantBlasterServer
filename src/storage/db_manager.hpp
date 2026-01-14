/********************************************************************
  Copyright 2025, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/12/27 14:15
  filename:  ElefantBlaster/ElefantBlasterServer/storage/db_manager.hpp

  purpose:   Header file for the database manager
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ELEFANT_BLASTER_SERVER_STORAGE_DB_MANAGER_HPP
#define ELEFANT_BLASTER_SERVER_STORAGE_DB_MANAGER_HPP
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <string>

#include <spdlog/spdlog.h>

#include <aries_base/definitions/macro.hpp>
#include <aries_base/database/db_factory.hpp>

#include "common/settings_manager.hpp"
#include "entities/user.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
using namespace aries_base::database;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

class DBManager {
 public:
  DBManager();
  virtual ~DBManager();

  static DBManager* Instance() { return instance_; }

  bool LoadConfig();

  bool ConnectAsRoot();
  bool ConnectAsUser();
  bool Connect(const std::string& connection_string);
  void Disconnect();

  bool IsConnected() const;

  bool CreateDatabase();
  bool UpdateDatabase();

  std::string NormalizeUsername(const std::string& username);
  bool VerifyUsername(const std::string& username, std::string &rules);
  bool AuthUser(const std::string& username, const std::string& password, User& user);

 private:
  int GetDbVersion();
  bool UpdateDatabaseV2();
  std::string StandalizeQueryCreateTable(const std::string& query, DBType db_type) const;
  std::string HashPassword(const std::string& password);
  std::string ConvertTime(time_t utc_time);
  time_t ConvertTime(std::string str_time);

 private:
  DBType db_type_;
  std::string initial_connection_string_;
  std::string running_connection_string_;
  std::string db_name_;
  std::unique_ptr<Database> db_;

 private:
  SettingsManager* settings_;
  std::shared_ptr<spdlog::logger> logger_;

 private:
  static DBManager* instance_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DBManager);
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ELEFANT_BLASTER_SERVER_STORAGE_DB_MANAGER_HPP
// -----------------------------------------------------------------------------
