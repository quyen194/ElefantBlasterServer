/********************************************************************
  Copyright 2025, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/12/27 14:23
  filename:  ElefantBlasterServer/src/storage/db_manager.cpp

  purpose:   Implementation file for the database manager
*********************************************************************/


// -----------------------------------------------------------------------------
#include <filesystem>
#include <string>

#include <aries_base/encryption/hash/hash_factory.hpp>
#include <aries_base/utils/scope_cleanup.hpp>

#include <entities/permission_list.hpp>
#include <entities/system_groups.hpp>
#include <entities/system_roles.hpp>

#include "storage/db_manager.hpp"
#include "db_manager.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
using namespace aries_base::utils;
using namespace aries_base::encryption::hash;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
DBManager* DBManager::instance_ = nullptr;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
DBManager::DBManager()
    : settings_(SettingsManager::Instance()) {
  logger_ = settings_->GetLogger("db", "DBManager", true);

  instance_ = this;
}
// -----------------------------------------------------------------------------

DBManager::~DBManager() {
  instance_ = nullptr;
}
// -----------------------------------------------------------------------------

bool DBManager::LoadConfig() {
  settings_->SetCurrentConfig(SettingsManager::kSettingDatabase);
  std::string str_type = settings_->GetNested<std::string>("/type", "SQLite");
  db_type_ = DatabaseFactory::GetDatabaseTypeFromName(str_type);

  if (db_type_ == DBType::SQLite) {
    initial_connection_string_ = settings_->GetNested<std::string>("/" + str_type + "/connection", "elefant_blaster.db");
    running_connection_string_ = initial_connection_string_;
    logger_->info("LoadConfig: Using SQLite database at '{}'", running_connection_string_);
    return !initial_connection_string_.empty() && !running_connection_string_.empty();
  }
  else if (db_type_ == DBType::PostgreSQL) {
    initial_connection_string_ = settings_->GetNested<std::string>("/" + str_type + "/initial_connection", "");
    running_connection_string_ = settings_->GetNested<std::string>("/" + str_type + "/running_connection", "");
    db_name_ = settings_->GetNested<std::string>("/" + str_type + "/db_name", "elefant_blaster");
    logger_->info("LoadConfig: Using PostgreSQL database with connection string '{}'", running_connection_string_);
    return !initial_connection_string_.empty() && !running_connection_string_.empty();
  }
  else {
    logger_->error("LoadConfig: Unsupported database type '{}'", str_type);
    return false;
  }

  if (initial_connection_string_.empty() || running_connection_string_.empty()) {
    logger_->error("LoadConfig: Empty database connection string");
    return false;
  }

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::ConnectAsRoot() {
  return Connect(initial_connection_string_);
}
// -----------------------------------------------------------------------------

bool DBManager::ConnectAsUser() {
  return Connect(running_connection_string_);
}
// -----------------------------------------------------------------------------

bool DBManager::Connect(const std::string& connection_string) {
  if (IsConnected()) {
    Disconnect();
  }

  if (db_type_ == DBType::SQLite) {
    std::filesystem::path db_path = connection_string;
    std::filesystem::path db_dir = db_path.parent_path();
    if (!db_dir.empty() && !std::filesystem::exists(db_dir)) {
      std::filesystem::create_directory(db_dir);
    }
  }

  db_ = DatabaseFactory::Create(db_type_);
  if (!db_) {
    logger_->error("Connect: Failed to create database instance");
    return false;
  }

  if (!db_->Connect(connection_string)) {
    logger_->error("Connect: Failed to connect to database with connection string '{}'", connection_string);
    return false;
  }

  logger_->info("Connect: Successfully connected to database");
  return true;
}
// -----------------------------------------------------------------------------

void DBManager::Disconnect() {
  if (db_ && db_->IsConnected()) {
    db_->Disconnect();
    logger_->info("Disconnect: Disconnected from database");
  }
}
// -----------------------------------------------------------------------------

bool DBManager::IsConnected() const {
  return db_ && db_->IsConnected();
}
// -----------------------------------------------------------------------------

bool DBManager::CreateDatabase() {
  std::string query;

  ConnectAsRoot();

  // defer disconnect
  ScopeCleanup disconnect_scope([this]() {
    Disconnect();
  });

  // create database if not exists (PostgreSQL)
  if (db_type_ == DBType::PostgreSQL) {
    db_->Execute("CREATE DATABASE IF NOT EXISTS " + db_name_);
  }

  int current_version = GetDbVersion();
  if (current_version > 0) {
    return true;
  }

  db_->Begin();

  // create users table
  query = R"(
      CREATE TABLE IF NOT EXISTS users (
          id            ID_TYPE_PLACEHOLDER,
          type          INTEGER NOT NULL DEFAULT 2,     -- 1=admin, 2=player
          username      TEXT UNIQUE NOT NULL,
          password      TEXT NOT NULL,
          display_name  TEXT NOT NULL,
          api_token     TEXT NOT NULL,
          is_banned     INTEGER DEFAULT 0,              -- BOOLEAN emulation
          ban_reason    TEXT,
          banned_until  TEXT,                           -- ISO 8601 or NULL
          is_actived    INTEGER DEFAULT 1,              -- BOOLEAN emulation
          created_at    TEXT DEFAULT (datetime('now'))  -- ISO 8601
      );
  )";
  query = StandalizeQueryCreateTable(query, db_type_);
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create users table: {}",
                   db_->GetLastError());
    return false;
  }

  // create groups table
  query = R"(
      CREATE TABLE IF NOT EXISTS groups (
          id            ID_TYPE_PLACEHOLDER,
          is_system     INTEGER DEFAULT 1,
          name          TEXT UNIQUE NOT NULL,           -- example: "Admins", "Moderators", "Players", "VIP"
          display_name  TEXT NOT NULL,
          description   TEXT,
          is_actived    INTEGER DEFAULT 1,              -- BOOLEAN emulation
          created_at    TEXT DEFAULT (datetime('now'))  -- ISO 8601
      );
  )";
  query = StandalizeQueryCreateTable(query, db_type_);
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create groups table: {}",
                   db_->GetLastError());
    return false;
  }

  // create roles table
  query = R"(
      CREATE TABLE IF NOT EXISTS roles (
          id            ID_TYPE_PLACEHOLDER,
          is_system     INTEGER DEFAULT 1,
          name          TEXT UNIQUE NOT NULL,
          display_name  TEXT NOT NULL,
          description   TEXT,
          is_actived    INTEGER DEFAULT 1,              -- BOOLEAN emulation
          created_at    TEXT DEFAULT (datetime('now'))  -- ISO 8601
      );
  )";
  query = StandalizeQueryCreateTable(query, db_type_);
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create roles table: {}",
                   db_->GetLastError());
    return false;
  }

  // create permissions table
  query = R"(
      CREATE TABLE IF NOT EXISTS permissions (
          id          ID_TYPE_PLACEHOLDER,
          risk_level  INTEGER,
          name        TEXT UNIQUE NOT NULL,
          description TEXT
      );
  )";
  query = StandalizeQueryCreateTable(query, db_type_);
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create permissions table: {}",
                   db_->GetLastError());
    return false;
  }

  // create maps table
  query = R"(
      CREATE TABLE IF NOT EXISTS maps (
          id          ID_TYPE_PLACEHOLDER,
          name        TEXT UNIQUE NOT NULL,
          data        TEXT NOT NULL,                     -- JSON string
          width       INTEGER NOT NULL,
          height      INTEGER NOT NULL,
          owner_id    INTEGER REFERENCES users(id),      -- NULL = official
          status      INTEGER NOT NULL DEFAULT 0,        -- 0=official, 10=private, 11=pending, 12=public
          uploaded_at TEXT DEFAULT (datetime('now')),
          approved_by INTEGER REFERENCES users(id),
          approved_at TEXT
      );
  )";
  query = StandalizeQueryCreateTable(query, db_type_);
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create maps table: {}",
                   db_->GetLastError());
    return false;
  }

  // create group_users table
  query = R"(
      CREATE TABLE IF NOT EXISTS group_users (
          group_id  INTEGER REFERENCES groups(id),
          user_id   INTEGER REFERENCES users(id),
          PRIMARY KEY (group_id, user_id)
      );
  )";
  query = StandalizeQueryCreateTable(query, db_type_);
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create user_groups table: {}",
                   db_->GetLastError());
    return false;
  }

  // create group_roles table
  query = R"(
      CREATE TABLE IF NOT EXISTS group_roles (
          group_id  INTEGER REFERENCES groups(id),
          role_id   INTEGER REFERENCES roles(id),
          PRIMARY KEY (group_id, role_id)
      );
  )";
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create group_roles table: {}",
                   db_->GetLastError());
    return false;
  }

  // create user_roles table
  query = R"(
      CREATE TABLE IF NOT EXISTS user_roles (
          user_id INTEGER REFERENCES users(id),
          role_id INTEGER REFERENCES roles(id),
          PRIMARY KEY (user_id, role_id)
      );
  )";
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create user_roles table: {}",
                   db_->GetLastError());
    return false;
  }

  // create role_permissions table
  query = R"(
      CREATE TABLE IF NOT EXISTS role_permissions (
          role_id       INTEGER REFERENCES roles(id),
          permission_id INTEGER REFERENCES permissions(id),
          PRIMARY KEY (role_id, permission_id)
      );
  )";
  if (!db_->Execute(query)) {
    logger_->error(
        "CreateDatabase: Failed to create role_permissions table: {}",
        db_->GetLastError());
    return false;
  }

  // create db_version table
  query = R"(
      CREATE TABLE IF NOT EXISTS db_version (
          version     ID_TYPE_PLACEHOLDER,
          uploaded_at TEXT DEFAULT (datetime('now'))
      );
  )";
  query = StandalizeQueryCreateTable(query, db_type_);
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create db_version table: {}",
                   db_->GetLastError());
    return false;
  }

  if (!AddDefaultData()) {
    logger_->error("CreateDatabase: Failed to add default data");
    return false;
  }

  // initial version
  query = R"(
      INSERT INTO db_version (version) VALUES (1);
  )";
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to insert initial db_version: {}",
                   db_->GetLastError());
    return false;
  }

  db_->Commit();

  logger_->info("CreateDatabase: Database created successfully");

  Disconnect();

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::AddDefaultData() {
  bool result = false;

  logger_->info("AddDefaultData: Start");

  // add all permissions
  if (!AddAllPermissions()) {
    logger_->error("AddDefaultData: Failed to add all permissions");
    return false;
  }

  // add all system roles
  Role role;
  role.is_system = true;

  role.name = game_system::role::super_admin;
  role.display_name = "Super Admin";
  role.desc = "Super Admin Controls Everything";
  if (!AddRole(role)) {
    return false;
  }
  if (!AddRolePermissions(role.name,
                          {permission::self::all,
                           permission::admin::all,
                           permission::usergroup::all,
                           permission::user::all,
                           permission::role::all,
                           permission::server::app::all,
                           permission::server::admin::all,
                           permission::server::game::all,
                           permission::match::all,
                           permission::map::all})) {
    return false;
  }

  role.name = game_system::role::server_admin;
  role.display_name = "Server Admin";
  role.desc = "Server Manager";
  if (!AddRole(role)) {
    return false;
  }

  role.name = game_system::role::game_admin;
  role.display_name = "Game Admin";
  role.desc = "Game Master";
  if (!AddRole(role)) {
    return false;
  }

  role.name = game_system::role::player;
  role.display_name = "Player";
  role.desc = "Join Match, Replay Match";
  if (!AddRole(role)) {
    return false;
  }
  if (!AddRolePermissions(role.name,
                          {permission::match::create,  //
                           permission::match::replay})) {
    return false;
  }

  role.name = game_system::role::guest;
  role.display_name = "Guest";
  role.desc = "View Match, Replay Match";
  if (!AddRole(role)) {
    return false;
  }
  if (!AddRolePermissions(
          role.name,
          {permission::match::replay})) {
    return false;
  }

  role.name = game_system::role::match_owner;
  role.display_name = "Match Owner";
  role.desc = "View Match, Replay Match";
  if (!AddRole(role)) {
    return false;
  }
  if (!AddRolePermissions(role.name,
                          {permission::match::update,
                           permission::match::start,
                           permission::match::pause,
                           permission::match::resume,
                           permission::match::add_member,
                           permission::match::kick_member,
                           permission::match::replay})) {
    return false;
  }

  // add all system groups
  Group group;
  group.is_system = true;

  group.name = game_system::group::server_admins;
  group.display_name = "Server Admins";
  group.desc = "Server Managers";
  if (!AddGroup(group)) {
    return false;
  }
  if (!AddGroupRole(std::string(game_system::group::server_admins),
                    std::string(game_system::role::server_admin))) {
    return false;
  }

  group.name = game_system::group::game_admins;
  group.display_name = "Game Admins";
  group.desc = "Game Masters";
  if (!AddGroup(group)) {
    return false;
  }
  if (!AddGroupRole(std::string(game_system::group::game_admins),
                    std::string(game_system::role::game_admin))) {
    return false;
  }

  group.name = game_system::group::players;
  group.display_name = "Players";
  group.desc = "Normal Players";
  if (!AddGroup(group)) {
    return false;
  }
  if (!AddGroupRole(std::string(game_system::group::players),
                    std::string(game_system::role::player))) {
    return false;
  }

  group.name = game_system::group::guests;
  group.display_name = "Guest";
  group.desc = "Anonymous Guest";
  if (!AddGroup(group)) {
    return false;
  }
  if (!AddGroupRole(std::string(game_system::group::guests),
                    std::string(game_system::role::guest))) {
    return false;
  }

  // add system users
  User admin_user = {
      UserType::kAdmin,  // type
      "admin",           // username
      "quyen194",        // password
      "Administrator",   // display_name
      "N/A",             // api_token
  };

  result = AddUser(admin_user);
  if (!result) {
    logger_->error(
        "AddDefaultData: Failed to create Administrator user({} / {}): {}",
        admin_user.username,
        admin_user.password,
        db_->GetLastError());
    return false;
  }

  logger_->info(
      "AddDefaultData: Create Administrator user with username: {} / password: {}",
      admin_user.username,
      admin_user.password);

  logger_->info("AddDefaultData: End successfully");

  return true;
 }
// -----------------------------------------------------------------------------

bool DBManager::UpdateDatabase() {
  Connect(initial_connection_string_);

  // defer disconnect
  ScopeCleanup disconnect_scope([this]() {
    Disconnect();
  });

  int current_version = GetDbVersion();
  logger_->info("UpdateDatabase: Current database version is {}", current_version);

  if (current_version < 2) {
    if (!UpdateDatabaseV2()) {
      logger_->error("UpdateDatabase: Failed to update database to version 2");
      return false;
    }
    current_version = 2;
    logger_->info("UpdateDatabase: Updated database to version 2");
  }

  Disconnect();

  return true;
}
// -----------------------------------------------------------------------------

int DBManager::GetDbVersion() {
  int current_version = 0;

  auto rs = db_->Execute("SELECT version FROM db_version ORDER BY version DESC LIMIT 1;");
  if (rs && rs->Next()) {
    current_version = rs->GetInt(0);
  }
  else {
    logger_->error("UpdateDatabase: Failed to get current database version: {} (NORMAL FOR DB INIT)", db_->GetLastError());
  }

  return current_version;
}
// -----------------------------------------------------------------------------

bool DBManager::UpdateDatabaseV2() {
  db_->Begin();

  // defer rollback if not committed
  ScopeCleanup rollback_scope([this]() {
    db_->Rollback();
  });

  // TODO: add a new column to an existing table

  // update version
  if (!db_->Execute(R"(
      UPDATE db_version SET version = 2;
  )")) {
    logger_->error("UpdateDatabaseV2: Failed to update db_version: {}", db_->GetLastError());
    return false;
  }

  rollback_scope.Dismiss();

  db_->Commit();

  return true;
}
// -----------------------------------------------------------------------------

std::string DBManager::StandalizeQueryCreateTable(const std::string& query,
                                                  DBType db_type) const {
  auto replace_func = [](const std::string& input,
                         const std::string& from,
                         const std::string& to) -> std::string {
    std::string output = input;
    size_t start_pos = 0;
    while ((start_pos = output.find(from, start_pos)) != std::string::npos) {
      output.replace(start_pos, from.length(), to);
      start_pos += to.length();
    }
    return output;
  };

  std::string modified_query;

  if (db_type == DBType::PostgreSQL) {
    // Example: convert AUTOINCREMENT to GENERATED ALWAYS AS IDENTITY
    modified_query = replace_func(query, "ID_TYPE_PLACEHOLDER", "INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY");
    return modified_query;
  }

  if (db_type == DBType::SQLite) {
    // Example: convert GENERATED ALWAYS AS IDENTITY to AUTOINCREMENT
    std::string modified_query = query;
    size_t pos = modified_query.find("ID_TYPE_PLACEHOLDER");
    if (pos != std::string::npos) {
      modified_query.replace(pos, strlen("ID_TYPE_PLACEHOLDER"), "INTEGER PRIMARY KEY AUTOINCREMENT");
    }
    return modified_query;

  }

  return std::string();
}
// -----------------------------------------------------------------------------

std::string DBManager::ConvertTime(time_t utc_time) {
  std::tm tm = {};

#if defined(_WIN32)
  gmtime_s(&tm, &utc_time);  // UTC
#else
  gmtime_r(&utc_time, &tm);  // UTC
#endif

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}
// -----------------------------------------------------------------------------

time_t DBManager::ConvertTime(std::string str_time) {
  std::tm tm = {};

  strptime(str_time.c_str(), "%Y-%m-%d %H:%M:%S", &tm);

#if defined(_WIN32)
  return _mkgmtime(&tm);  // UTC
#else
  return timegm(&tm);  // UTC
#endif
}
// -----------------------------------------------------------------------------

std::string DBManager::NormalizeUsername(const std::string& username) {
  std::string output = username;
  std::transform(
      output.begin(), output.end(), output.begin(), [](unsigned char c) {
        return std::tolower(c);
      });
  return output;
}
// -----------------------------------------------------------------------------

bool DBManager::VerifyUsername(const std::string& username,
                               std::string& rules) {
  rules = R"(
      Allowed characters: (a-z), (A-Z), (0-9), (.), (_)
      Length: 8-20 characters
      Must start with a letter
      Must end with a letter or number
      Not contain following words: admin, root, system, support, null, undefined
  )";

  // 1. Length check
  if (username.length() < 8 || username.length() > 20) {
    return false;
  }

  // 2. Must start with a letter
  if (!std::isalpha(static_cast<unsigned char>(username.front()))) {
    return false;
  }

  // 3. Must end with a letter or number
  if (!std::isalnum(static_cast<unsigned char>(username.back()))) {
    return false;
  }

  // 4. Allowed characters check
  for (char ch : username) {
    if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '.' ||
          ch == '_')) {
      return false;
    }
  }

  // 5. Convert username to lowercase for keyword checking
  std::string lower_username = username;
  std::transform(lower_username.begin(),
                 lower_username.end(),
                 lower_username.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // 6. Forbidden keywords
  static const std::vector<std::string> forbidden_words = {
      "admin", "root", "system", "support", "null", "undefined"
  };

  for (const auto& word : forbidden_words) {
    if (lower_username.find(word) != std::string::npos) {
      return false;
    }
  }

  return true;
}
// -----------------------------------------------------------------------------

std::string DBManager::HashPassword(const std::string& password) {
  auto hasher = HashFactory::Create(HashType::SHA256);
  hasher->Update(password.data(), password.size());
  return hasher->Final();
}
// -----------------------------------------------------------------------------

bool DBManager::AddUser(const User& user) {
  std::string normalized_username = NormalizeUsername(user.username);

  std::string query = R"(
      INSERT INTO users(type, username, password, display_name, api_token)
      VALUES(?, ?, ?, ?, ?)
  )";
  auto stmt = db_->Prepare(query);
  if (!stmt) {
    logger_->error("AddUser: Failed to prepare stmt: {}", db_->GetLastError());
    return false;
  }

  int i = 1;
  stmt->BindInt(i++, user.type);
  stmt->BindString(i++, normalized_username);
  stmt->BindString(i++, HashPassword(user.password));
  stmt->BindString(i++, user.display_name);
  stmt->BindString(i++, user.api_token);

  if (!stmt->Execute()) {
    logger_->error("AddUser: Failed to add user({}): {}",
                   normalized_username,
                   stmt->GetLastError());
    return false;
  }

  logger_->info("AddUser: Add user({}) successfully", normalized_username);

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::UpdateUser(const User& user) {
  std::string query = R"(
      UPDATE users
      SET
          type = ?,
          password = ?,
          display_name = ?,
          api_token = ?,
          is_banned = ?,
          ban_reason = ?,
          banned_until = ?,
          is_actived = ?
      WHERE username = ?
  )";
  auto stmt = db_->Prepare(query);
  if (!stmt) {
    logger_->error("AddUser: Failed to prepare stmt: {}", db_->GetLastError());
    return false;
  }

  int i = 1;
  stmt->BindInt(i++, user.type);
  stmt->BindString(i++, HashPassword(user.password));
  stmt->BindString(i++, user.display_name);
  stmt->BindString(i++, user.api_token);
  stmt->BindInt(i++, user.is_banned ? 1 : 0);
  stmt->BindString(i++, user.ban_reason);
  stmt->BindString(i++, ConvertTime(user.banned_until));
  stmt->BindInt(i++, user.is_actived ? 1 : 0);
  stmt->BindString(i++, user.username);

  if (!stmt->Execute()) {
    logger_->error("AddUser: Failed to update user({}): {}",
                   user.username,
                   stmt->GetLastError());
    return false;
  }

  logger_->info("AddUser: Update user({}) successfully", user.username);

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::GetUser(const std::string& username, DbUser& user) {
  int i = 0;
  std::string normalized_username = NormalizeUsername(username);

  std::string query = R"(
      SELECT
          id,
          type,
          display_name,
          api_token,
          is_banned,
          ban_reason,
          banned_until,
          is_actived
      FROM users
      WHERE username = ?
  )";

  auto stmt = db_->Prepare(query);
  if (!stmt) {
    logger_->error("GetUser: Failed to prepare stmt: {}", db_->GetLastError());
    return false;
  }

  i = 1;
  stmt->BindString(i++, normalized_username);

  auto result_set = stmt->Query();

  if (!result_set) {
    logger_->error("GetUser: Failed to execute stmt: {}",
                   stmt->GetLastError());
    return false;
  }

  if (!result_set->Next()) {
    logger_->error("GetUser: No user found matching the provided credentials. Username: {}",
                   username);
    return false;
  }

  i = 0;
  user.id = result_set->GetInt64(i++);
  user.type = static_cast<UserType>(result_set->GetInt(i++));
  user.display_name = result_set->GetString(i++);
  user.api_token = result_set->GetString(i++);
  user.is_banned = !!result_set->GetInt(i++);
  user.ban_reason = result_set->GetString(i++);
  user.banned_until = ConvertTime(result_set->GetString(i++));
  user.is_actived = !!result_set->GetInt(i++);
  user.created_at = 0;

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::GetUsers(std::vector<DbUser> users,
                         std::string filter_name,
                         bool sorted_by_asc,
                         std::uint64_t last_id,
                         std::uint64_t max_count) {
  int i = 0;

  std::string query = R"(
      SELECT
          id,
          type,
          username,
          display_name,
          api_token,
          is_banned,
          ban_reason,
          banned_until,
          is_actived
      FROM users
  )";

  // apply filter
  bool filter_applied = false;
  if (!filter_name.empty()) {
    if (db_type_ == DBType::PostgreSQL) {
      query += R"(
          WHERE (   username     ILIKE '%' || ? || '%'
                 OR display_name ILIKE '%' || ? || '%')
      )";
      filter_applied = true;
    }
    else if (db_type_ == DBType::SQLite) {
      query += R"(
          WHERE (   username     LIKE '%' || ? || '%' COLLATE NOCASE
                 OR display_name LIKE '%' || ? || '%' COLLATE NOCASE)
      )";
      filter_applied = true;
    }
  }

  if (last_id) {
    query += !filter_applied ? R"(WHERE)" : R"(AND)";
    query += R"( id)";
    query += sorted_by_asc ? R"( > )" : R"( < )";
    query += R"(?)";
  }

  // apply sort
  if (sorted_by_asc) {
    query += R"(
        ORDER BY id ASC
    )";
  }
  else {
    query += R"(
        ORDER BY id DESC
    )";
  }

  // apply limit
  if (max_count) {
    query += R"(
        LIMIT ?
    )";
  }

  auto stmt = db_->Prepare(query);
  if (!stmt) {
    logger_->error("GetUser: Failed to prepare stmt: {}", db_->GetLastError());
    return false;
  }

  i = 1;
  if (!filter_name.empty()) {
    stmt->BindString(i++, filter_name);
    stmt->BindString(i++, filter_name);
  }
  if (last_id) {
    stmt->BindInt64(i++, last_id);
  }
  if (max_count) {
    stmt->BindInt64(i++, max_count);
  }

  auto result_set = stmt->Query();
  if (!result_set) {
    logger_->error("GetUser: Failed to execute stmt: {}",
                   stmt->GetLastError());
    return false;
  }

  while (result_set->Next()) {
    DbUser user = {};
    i = 0;
    user.id = result_set->GetInt64(i++);
    user.type = static_cast<UserType>(result_set->GetInt(i++));
    user.username = result_set->GetString(i++);
    user.display_name = result_set->GetString(i++);
    user.api_token = result_set->GetString(i++);
    user.is_banned = !!result_set->GetInt(i++);
    user.ban_reason = result_set->GetString(i++);
    user.banned_until = ConvertTime(result_set->GetString(i++));
    user.is_actived = !!result_set->GetInt(i++);
    user.created_at = 0;

    users.push_back(user);
  }

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::AuthUser(const std::string& username,
                         const std::string& password,
                         DbUser& user) {
  int i = 0;
  std::string normalized_username = NormalizeUsername(username);
  std::string hash_password = HashPassword(password);

  std::string query = R"(
      SELECT
          id,
          type,
          display_name,
          api_token,
          is_banned,
          ban_reason,
          banned_until,
          is_actived
      FROM users
      WHERE username = ?
        AND password = ?
  )";

  auto stmt = db_->Prepare(query);

  if (!stmt) {
    logger_->error("AuthUser: Failed to prepare stmt: {}", db_->GetLastError());
    return false;
  }

  i = 1;
  stmt->BindString(i++, normalized_username);
  stmt->BindString(i++, hash_password);

  auto result_set = stmt->Query();

  if (!result_set) {
    logger_->error("AuthUser: Failed to execute stmt: {}",
                   stmt->GetLastError());
    return false;
  }

  if (!result_set->Next()) {
    logger_->error("AuthUser: No user found matching the provided credentials. Username: {}",
                   username);
    return false;
  }

  i = 0;
  user.id = result_set->GetInt64(i++);
  user.type = static_cast<UserType>(result_set->GetInt(i++));
  user.username = normalized_username;
  user.display_name = result_set->GetString(i++);
  user.api_token = result_set->GetString(i++);
  user.is_banned = !!result_set->GetInt(i++);
  user.ban_reason = result_set->GetString(i++);
  user.banned_until = ConvertTime(result_set->GetString(i++));
  user.is_actived = !!result_set->GetInt(i++);
  user.created_at = 0;

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::GetUserPermissions(const std::string& username,
                                   std::set<std::string>& permissions) {
  int i = 0;
  std::string normalized_username = NormalizeUsername(username);

  std::string query = R"(
      SELECT DISTINCT name
      FROM permissions
      WHERE id IN (
          -- via group roles
          SELECT role_permissions.permission_id
          FROM role_permissions
          JOIN roles ON roles.id = role_permissions.role_id
          JOIN group_roles ON group_roles.role_id = roles.id
          JOIN group_users ON group_users.group_id = group_roles.group_id
          JOIN users ON users.id = group_users.user_id
          WHERE users.username = ?

          UNION

          -- directly from user roles
          SELECT role_permissions.permission_id
          FROM role_permissions
          JOIN roles ON roles.id = role_permissions.role_id
          JOIN user_roles ON user_roles.role_id = roles.id
          JOIN users ON users.id = user_roles.user_id
          WHERE users.username = ?
      )
  )";
  auto stmt = db_->Prepare(query);
  if (!stmt) {
    logger_->error("GetUserPermissions: Failed to prepare stmt: {}",
                   db_->GetLastError());
    return false;
  }

  i = 1;
  stmt->BindString(i++, normalized_username);
  stmt->BindString(i++, normalized_username);

  auto result_set = stmt->Query();
  if (!result_set) {
    logger_->error("GetUserPermissions: Failed to execute stmt: {}",
                   stmt->GetLastError());
    return false;
  }

  while (result_set->Next()) {
    i = 0;
    std::string permission = result_set->GetString(i++);

    permissions.insert(permission);
  }

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::AddUserRole(const std::string& username,
                            const std::string& role_name) {
  std::string query = R"(
      WITH tb_users AS (
          SELECT id AS user_id
          FROM users
          WHERE username = ?
      ),
      tb_roles AS (
          SELECT id AS role_id
          FROM roles
          WHERE name = ?
      )
      INSERT INTO user_roles (user_id, role_id)
      SELECT tb_users.user_id, tb_roles.role_id
      FROM tb_users
      CROSS JOIN tb_roles;
  )";
  auto stmt = db_->Prepare(query);
  if (!stmt) {
    logger_->error("AddUserRole: Failed to prepare insert statement: {}",
                   db_->GetLastError());
    return false;
  }

  int i = 1;
  stmt->BindString(i++, username);
  stmt->BindString(i++, role_name);

  if (!stmt->Execute()) {
    logger_->error("AddUserRole: Failed to add role({}) for user({}): {}",
                   role_name,
                   username,
                   stmt->GetLastError());
    return false;
  }

  logger_->info("AddUserRole: Add role({}) for user({}) successfully",
                role_name,
                username);

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::AddGroup(const Group& group) {
  std::string query = R"(
      INSERT INTO groups (is_system, name, display_name, description)
      VALUES (?, ?, ?, ?)
  )";
  auto stmt = db_->Prepare(query);
  if (!stmt) {
    logger_->error("AddGroup: Failed to prepare insert statement: {}",
                   db_->GetLastError());
    return false;
  }

  int i = 1;
  stmt->BindInt(i++, group.is_system ? 1 : 0);
  stmt->BindString(i++, group.name);
  stmt->BindString(i++, group.display_name);
  stmt->BindString(i++, group.desc);

  if (!stmt->Execute()) {
    logger_->error("AddGroup: Failed to add group({}): {}",
                    group.name,
                    stmt->GetLastError());
    return false;
  }

  logger_->info("AddGroup: Add group({}) successfully", group.name);

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::AddGroupUser(const std::string& group_name,
                             const std::string& user_name) {
  std::string query = R"(
      WITH tb_groups AS (
          SELECT id AS group_id
          FROM groups
          WHERE name = ?
      ),
      tb_users AS (
          SELECT id AS user_id
          FROM users
          WHERE username = ?
      )
      INSERT INTO group_users (group_id, user_id)
      SELECT tb_groups.group_id, tb_users.user_id
      FROM tb_groups
      CROSS JOIN tb_users;
  )";
  auto stmt = db_->Prepare(query);
  if (!stmt) {
    logger_->error("AddGroupUser: Failed to prepare insert statement: {}",
                   db_->GetLastError());
    return false;
  }

  int i = 1;
  stmt->BindString(i++, group_name);
  stmt->BindString(i++, user_name);

  if (!stmt->Execute()) {
    logger_->error("AddGroupUser: Failed to add user({}) to group({}): {}",
                   user_name,
                   group_name,
                   stmt->GetLastError());
    return false;
  }

  logger_->info("AddGroupUser: Add user({}) to group({}) successfully",
                user_name,
                group_name);

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::AddGroupRole(const std::string& group_name,
                             const std::string& role_name) {
  std::string query = R"(
      WITH tb_groups AS (
          SELECT id AS group_id
          FROM groups
          WHERE name = ?
      ),
      tb_roles AS (
          SELECT id AS role_id
          FROM roles
          WHERE name = ?
      )
      INSERT INTO group_roles (group_id, role_id)
      SELECT tb_groups.group_id, tb_roles.role_id
      FROM tb_groups
      CROSS JOIN tb_roles;
  )";
  auto stmt = db_->Prepare(query);
  if (!stmt) {
    logger_->error("AddGroupRole: Failed to prepare insert statement: {}",
                   db_->GetLastError());
    return false;
  }

  int i = 1;
  stmt->BindString(i++, group_name);
  stmt->BindString(i++, role_name);

  if (!stmt->Execute()) {
    logger_->error("AddGroupRole: Failed to add role({}) for group({}): {}",
                   role_name,
                   group_name,
                   stmt->GetLastError());
    return false;
  }

  logger_->info("AddGroupUser: Add role({}) for group({}) successfully",
                role_name,
                group_name);

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::AddRole(const Role& role) {
  std::string query = R"(
      INSERT INTO roles (is_system, name, display_name, description)
      VALUES (?, ?, ?, ?)
  )";
  auto stmt = db_->Prepare(query);
  if (!stmt) {
    logger_->error("AddRole: Failed to prepare insert statement: {}",
                   db_->GetLastError());
    return false;
  }

  int i = 1;
  stmt->BindInt(i++, role.is_system ? 1 : 0);
  stmt->BindString(i++, role.name);
  stmt->BindString(i++, role.display_name);
  stmt->BindString(i++, role.desc);

  if (!stmt->Execute()) {
    logger_->error("AddRole: Failed to add role({}): {}",
                    role.name,
                    stmt->GetLastError());
    return false;
  }

  logger_->info("AddRole: Add role({}) successfully", role.name);

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::AddRolePermissions(
    const std::string& role_name,
    const std::set<std::string_view>& permissions) {

  std::string query = R"(
      WITH tb_roles AS (
          SELECT id AS role_id
          FROM roles
          WHERE name = ?
      ),
      tb_permissions AS (
          SELECT id AS permission_id
          FROM permissions
          WHERE name = ?
      )
      INSERT INTO role_permissions (role_id, permission_id)
      SELECT tb_roles.role_id, tb_permissions.permission_id
      FROM tb_roles
      CROSS JOIN tb_permissions;
  )";
  auto stmt = db_->Prepare(query);
  if (!stmt) {
    logger_->error("AddRolePermissions: Failed to prepare insert statement: {}",
                   db_->GetLastError());
    return false;
  }

  for (auto permission : permissions) {
    stmt->Reset();
    int i = 1;
    stmt->BindString(i++, role_name);
    stmt->BindString(i++, std::string(permission));

    if (!stmt->Execute()) {
      logger_->error(
          "AddRolePermissions: Failed to add permission({}) to role({}): {}",
          permission,
          role_name,
          stmt->GetLastError());
      return false;
    }

    logger_->info(
        "AddRolePermissions: Add permission({}) to role({}) successfully",
        permission,
        role_name);
  }

  return true;
}
// -----------------------------------------------------------------------------

bool DBManager::AddAllPermissions() {
  std::string query = R"(
      INSERT INTO permissions (risk_level, name, description)
      VALUES (?, ?, ?)
  )";
  auto stmt = db_->Prepare(query);
  if (!stmt) {
    logger_->error("AddAllPermissions: Failed to prepare insert statement: {}",
                   db_->GetLastError());
    return false;
  }

  for (auto permission : kPermissions) {
    stmt->Reset();
    int i = 1;
    stmt->BindInt(i++, permission.risk);
    stmt->BindString(i++, std::string(permission.name));
    stmt->BindString(i++, std::string(permission.desc));

    if (!stmt->Execute()) {
      logger_->error("AddAllPermissions: Failed to add permission({}): {}",
                     permission.name,
                     stmt->GetLastError());
      return false;
    }
  }

  logger_->info("AddAllPermissions: Add all permission successfully");

  return true;
}
// -----------------------------------------------------------------------------
