/********************************************************************
  Copyright 2025, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/12/27 14:15
  filename:  ElefantBlaster/ElefantBlasterServer/storage/db_manager.hpp

  purpose:   Database manager for the Elefant Blaster server
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

  bool Connect(const std::string& connection_string);
  void Disconnect();

  bool IsConnected() const;

  bool CreateDatabase();
  bool UpdateDatabase();

 private:
  int GetDbVersion();
  bool UpdateDatabaseV2();
  std::string StandalizeQueryCreateTable(const std::string& query, DBType db_type) const;

  std::string HashPassword(const std::string& password);

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
