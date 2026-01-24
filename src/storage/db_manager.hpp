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
#include <set>
#include <string>

#include <spdlog/spdlog.h>

#include <aries_base/definitions/macro.hpp>
#include <aries_base/database/db_factory.hpp>

#include <storage/shared/db_definitions.hpp>

#include "common/settings_manager.hpp"
#include "entities/db_group.hpp"
#include "entities/db_role.hpp"
#include "entities/db_user.hpp"
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
  bool AddDefaultData();
  bool UpdateDatabase();

  bool AddUser(const User& user);
  bool UpdateUser(const User& user);
  bool UpdateUserLastOnlineTime(User& user);
  bool GetUser(const std::string& username, DbUser& user);
  bool GetUsers(std::vector<DbUser> &users,
                std::string filter_name = "",
                SortBy sort_type = SortBy::kNone,
                std::uint64_t last_id = 0,
                std::uint64_t max_count = 0);
  bool AuthUser(const std::string& username, const std::string& password, DbUser& user);
  bool GetUserPermissions(const std::string& username, std::set<std::string>& permissions);

  bool AddUserRole(const std::string& username, const std::string& role_name);

  bool AddGroup(const Group& group);
  bool AddGroupUser(const std::string& group_name, const std::string& user_name);
  bool AddGroupRole(const std::string& group_name, const std::string& role_name);

  bool AddRole(const Role& role);
  bool AddRolePermissions(const std::string& role_name, const std::set<std::string_view> &permissions);

  bool AddAllPermissions();

 private:
  int GetDbVersion();
  bool UpdateDatabaseV2();
  std::string StandalizeQueryCreateTable(const std::string& query, DBType db_type) const;
  std::string ConvertTime(time_t utc_time);
  time_t ConvertTime(std::string str_time);

  std::string NormalizeUsername(const std::string& username);
  bool VerifyUsername(const std::string& username, std::string &rules);
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
