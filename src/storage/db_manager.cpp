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

#include "storage/db_manager.hpp"
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

  logger_->error("LoadConfig: Unsupported database type '{}'", str_type);
  return false;
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

  // create permissions table
  query = R"(
      CREATE TABLE IF NOT EXISTS permissions (
          id            INTEGER PRIMARY KEY,
          name          TEXT UNIQUE NOT NULL,
          description   TEXT
      );
  )";
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create permissions table: {}", db_->GetLastError());
    return false;
  }

  // add all permissions

  // create roles table
  query = R"(
      CREATE TABLE IF NOT EXISTS roles (
          id            INTEGER PRIMARY KEY,
          name          TEXT UNIQUE NOT NULL,
          description   TEXT
      );
  )";
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create roles table: {}", db_->GetLastError());
    return false;
  }

  // add all roles

  // create role_permissions table
  query = R"(
      CREATE TABLE IF NOT EXISTS role_permissions (
          role_id       INTEGER REFERENCES roles(id),
          permission_id INTEGER REFERENCES permissions(id),
          PRIMARY KEY (role_id, permission_id)
      );
  )";
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create role_permissions table: {}", db_->GetLastError());
    return false;
  }

  // create users table
  query = R"(
      CREATE TABLE IF NOT EXISTS users (
          id            ID_TYPE_PLACEHOLDER,
          type          INTEGER NOT NULL DEFAULT 2,        -- 1=admin, 2=player
          username      TEXT UNIQUE NOT NULL,
          password_hash TEXT NOT NULL,
          display_name  TEXT NOT NULL,
          api_token     TEXT NOT NULL,
          is_banned     INTEGER DEFAULT 0,                 -- BOOLEAN emulation
          ban_reason    TEXT,
          banned_until  TEXT,                              -- ISO 8601 or NULL
          created_at    TEXT DEFAULT (datetime('now'))     -- ISO 8601
      );
  )";
  query = StandalizeQueryCreateTable(query, db_type_);
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create users table: {}", db_->GetLastError());
    return false;
  }

  // add admin user
  {
    std::string username = "admin";
    std::string password = "quyen194";
    query = R"(
        INSERT INTO users(type, username, password_hash, display_name, api_token)
        VALUES(1, ?, ?, 'Administrator', 'N/A')
    )";
    auto stmt = db_->Prepare(query);
    if (!stmt) {
      logger_->error("CreateDatabase: Failed to create admin user: {}", db_->GetLastError());
      return false;
    }

    stmt->BindString(1, username);
    stmt->BindString(2, HashPassword(password));

    if (!stmt->Execute()) {
      logger_->error("CreateDatabase: Failed to create admin user: {}", stmt->GetLastError());
      return false;
    }

    logger_->info(
        "CreateDatabase: Create Administrator user with username: {} / password: {}",
        username.c_str(),
        password.c_str());
  }

  // create user_roles table
  query = R"(
      CREATE TABLE IF NOT EXISTS user_roles (
          user_id       INTEGER REFERENCES users(id),
          role_id       INTEGER REFERENCES roles(id),
          PRIMARY KEY (user_id, role_id)
      );
  )";
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create user_roles table: {}", db_->GetLastError());
    return false;
  }

  // create maps table
  query = R"(
      CREATE TABLE IF NOT EXISTS maps (
          id            ID_TYPE_PLACEHOLDER,
          name          TEXT NOT NULL,
          data          TEXT NOT NULL,                     -- JSON string
          width         INTEGER NOT NULL,
          height        INTEGER NOT NULL,
          owner_id      INTEGER REFERENCES users(id),      -- NULL = official
          status        INTEGER NOT NULL DEFAULT 0,        -- 0=official, 10=private, 11=pending, 12=public
          uploaded_at   TEXT DEFAULT (datetime('now')),
          approved_by   INTEGER REFERENCES users(id),
          approved_at   TEXT
      );
  )";
  query = StandalizeQueryCreateTable(query, db_type_);
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create maps table: {}", db_->GetLastError());
    return false;
  }

  // create db_version table
  query = R"(
      CREATE TABLE IF NOT EXISTS db_version (
          version       INTEGER PRIMARY KEY,
          uploaded_at   TEXT DEFAULT (datetime('now'))
      );
  )";
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to create db_version table: {}", db_->GetLastError());
    return false;
  }

  // initial version
  query = R"(
      INSERT INTO db_version (version) VALUES (1);
  )";
  if (!db_->Execute(query)) {
    logger_->error("CreateDatabase: Failed to insert initial db_version: {}", db_->GetLastError());
    return false;
  }

  logger_->info("CreateDatabase: Database created successfully");

  Disconnect();

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
    logger_->error("UpdateDatabase: Failed to get current database version: {}", db_->GetLastError());
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

bool DBManager::AuthUser(const std::string& username,
                         const std::string& password,
                         User& user) {
  int i = 0;
  std::string normalized_username = NormalizeUsername(username);
  std::string hash_password = HashPassword(password);

  std::string query = R"(
      SELECT id, type, display_name, api_token, is_banned, ban_reason, banned_until
      FROM users
      WHERE username = ?
        AND password_hash = ?
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
  user.display_name = result_set->GetString(i++);
  user.api_token = result_set->GetString(i++);
  user.is_banned = !!result_set->GetInt(i++);
  user.ban_reason = result_set->GetString(i++);
  user.banned_until = ConvertTime(result_set->GetString(i++));
  user.created_at = 0;

  return true;
}
// -----------------------------------------------------------------------------
