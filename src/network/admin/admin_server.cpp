/********************************************************************
  Copyright 2014, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/11/14 21:40
  filename:  ElefantBlaster/ElefantBlasterServer/network/net_server.cpp

  purpose:   Implementation file for the admin server
*********************************************************************/


// -----------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <sstream>

#include <aries_base/process/thread_pool/thread_pool.hpp>
#include <aries_base/utils/file_io.hpp>

#include "common/events.hpp"
#include "network/admin/admin_server.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
using namespace aries_base;
using namespace aries_base::process;
// -----------------------------------------------------------------------------
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
// -----------------------------------------------------------------------------
typedef asio::ssl::context context;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

AdminServer::AdminServer(DBManager* db_manager)
    : accept_client_(false),
      next_conn_id_(1),
      settings_(SettingsManager::Instance()),
      db_manager_(db_manager) {
  logger_ = settings_->GetLogger("admin_server", "AdminServer", true);
}
// -----------------------------------------------------------------------------

AdminServer::~AdminServer() {}
// -----------------------------------------------------------------------------

bool AdminServer::LoadConfig() {
  std::string path;

  settings_->SetCurrentConfig(SettingsManager::kSettingServer);

  host_ = settings_->GetNested<std::string>("/admin_network/host");
  port_ = settings_->GetNested<int>("/admin_network/port", 9003);

  cert_file_password_ = settings_->GetNested<std::string>("/admin_network/certificate_password");
  cert_file_path_ = settings_->GetNested<std::string>("/admin_network/certificate_chain_file");
  key_file_path_ = settings_->GetNested<std::string>("/admin_network/private_key_file");
  dh_params_file_path_ = settings_->GetNested<std::string>("/admin_network/dh_params_file");

  return true;
}
// -----------------------------------------------------------------------------

bool AdminServer::Start() {
  logger_->info("AdminServer: Starting server...");

  if (!LoadConfig()) {
      logger_->error("AdminServer: Failed to load configs, cannot start server");
      return false;
  }

  if (!LoadTlsData()) {
      logger_->error("AdminServer: Failed to load TLS data, cannot start server");
      return false;
  }

  server_ = std::make_unique<websocket_server>();

  // Initialize Asio
  server_->init_asio();

  // Register our message handler
  server_->set_tls_init_handler(bind(&AdminServer::OnTlsInit, this, _1));
  server_->set_validate_handler(bind(&AdminServer::OnValidate, this, _1));
  server_->set_open_handler(bind(&AdminServer::OnConnected, this, _1));
  server_->set_close_handler(bind(&AdminServer::OnDisconnected, this, _1));
  server_->set_fail_handler(bind(&AdminServer::OnError, this, _1));
  server_->set_message_handler(bind(&AdminServer::OnMessage, this, _1, _2));

  // Set perpetual mode to keep the server running
  server_->start_perpetual();

  // Listen on the specified host and port
  server_->listen(host_, std::to_string(port_));

  // Start the server accept loop
  server_->start_accept();

  ThreadPool::Instance()->PostTask(bind(&AdminServer::Worker, this),
                                   &worker_end_event_);

  logger_->info("AdminServer: Server listening on {}:{}", host_, port_);

  Active();

  logger_->info("AdminServer: Server started successfully");

  return true;
}
// -----------------------------------------------------------------------------

void AdminServer::Stop() {
  logger_->info("AdminServer: Stopping server...");
  Deactive();
  // Unset perpetual mode so server stop when no connection actived
  server_->stop_perpetual();
  server_->stop_listening();
  DisconnectAllClients("Server shutting down");
  server_->stop();
  worker_end_event_.Wait();
  server_.reset();
  logger_->info("AdminServer: Server stopped");
}
// -----------------------------------------------------------------------------

void AdminServer::Active() {
  logger_->info("AdminServer: Start accepting clients");
  accept_client_ = true;
}
// -----------------------------------------------------------------------------

void AdminServer::Deactive() {
  logger_->info("AdminServer: Stop accepting new clients");
  accept_client_ = false;
}
// -----------------------------------------------------------------------------

void AdminServer::DisconnectAllClients(const std::string &reason) {
  logger_->info("AdminServer: Disconnecting all clients...");
  std::unique_lock<std::mutex> lock(connections_mutex_);
  for (auto& pair : map_hdl_connections_) {
    websocketpp::lib::error_code ec;
    server_->close(pair.first, websocketpp::close::status::going_away, reason, ec);
    if (ec) {
      logger_->error("AdminServer: Close connection failed: {}", ec.message());
    }
  }
  logger_->info("AdminServer: All clients disconnected.");
}
// -----------------------------------------------------------------------------

void AdminServer::Worker() {
  // Start the ASIO io_service run loop
  logger_->info("AdminServer: Server run loop started");
  try {
    server_->run();
  } catch (const std::exception& e) {
    logger_->error("AdminServer: Exception in server run loop: {}", e.what());
  }
}
// -----------------------------------------------------------------------------

context_ptr AdminServer::OnTlsInit(connection_hdl hdl) {
  context_ptr ctx = websocketpp::lib::make_shared<context>(context::sslv23);
  ctx->set_options(context::default_workarounds |
                   context::no_sslv2 |
                   context::no_sslv3 |
                   context::single_dh_use);
  ctx->set_password_callback([this](std::size_t max_length,
                                    context::password_purpose purpose) -> std::string {
      return cert_file_password_;
  });
  ctx->use_certificate_chain({cert_file_data_.data(), cert_file_data_.size()});
  ctx->use_private_key({key_file_data_.data(), key_file_data_.size()}, context::pem);
  // Example method of generating this file:
  // `openssl dhparam -out dh.pem 2048`
  // Mozilla Intermediate suggests 1024 as the minimum size to use
  // Mozilla Modern suggests 2048 as the minimum size to use.
  ctx->use_tmp_dh({dh_params_file_data_.data(), dh_params_file_data_.size()});

  // Apply cipher list
  if (SSL_CTX_set_cipher_list(ctx->native_handle() , ciphers_.c_str()) != 1) {
      logger_->error("AdminServer: Error setting cipher list");
  }

  return ctx;
}
// -----------------------------------------------------------------------------

bool AdminServer::LoadTlsData() {
  cert_file_data_ = utils::StringFromFile(cert_file_path_);
  key_file_data_ = utils::StringFromFile(key_file_path_);
  dh_params_file_data_ = utils::StringFromFile(dh_params_file_path_);

  // Recommended cipher suite list for wide compatibility (2025)
  // Based on Mozilla Intermediate v5.7+
  // Supports: TLS 1.2 & TLS 1.3, modern browsers, Android 7+, iOS 11+, Windows 10+
  ciphers_ =
      // === TLS 1.3 - Automatic & strongest (recommended first) ===
      "TLS_AES_128_GCM_SHA256:"           // TLS 1.3 - AES-128-GCM (fast, secure)
      "TLS_AES_256_GCM_SHA384:"           // TLS 1.3 - AES-256-GCM (stronger)
      "TLS_CHACHA20_POLY1305_SHA256:"     // TLS 1.3 - ChaCha20 (mobile-friendly, no AES-NI needed)
      // === TLS 1.2 - Forward secrecy (ECDHE) + GCM (authenticated encryption) ===
      "ECDHE-ECDSA-AES128-GCM-SHA256:"    // Preferred: ECDSA + AES-128-GCM
      "ECDHE-RSA-AES128-GCM-SHA256:"      // RSA fallback
      "ECDHE-ECDSA-AES256-GCM-SHA384:"    // Stronger key
      "ECDHE-RSA-AES256-GCM-SHA384:"      // RSA + AES-256
      "ECDHE-ECDSA-CHACHA20-POLY1305:"    // ChaCha20 for ECDSA (Android/iOS optimized)
      "ECDHE-RSA-CHACHA20-POLY1305:"      // RSA version
      // === TLS 1.2 - DHE (ephemeral DH) - less preferred but secure ===
      "DHE-RSA-AES128-GCM-SHA256:"        // DHE + AES-128-GCM
      "DHE-RSA-AES256-GCM-SHA384:"        // DHE + AES-256-GCM
      // === TLS 1.2 - CBC fallback (only if needed for old clients) ===
      // Warning: CBC is vulnerable to padding oracle if misconfigured
      "ECDHE-ECDSA-AES128-SHA256:"        // CBC-SHA256 (acceptable)
      "ECDHE-RSA-AES128-SHA256:"          // RSA version
      "ECDHE-ECDSA-AES256-SHA384:"        // Stronger
      "ECDHE-RSA-AES256-SHA384:"          // RSA + SHA384
      // === Exclude weak, broken, or anonymous ciphers ===
      "!aNULL:"                           // Disable anonymous (no auth)
      "!eNULL:"                           // Disable null encryption
      "!EXPORT:"                          // Disable export ciphers (weak keys)
      "!DES:"                             // Disable DES (56-bit)
      "!3DES:"                            // Disable 3DES (sweet32 attack)
      "!RC4:"                             // Disable RC4 (broken)
      "!MD5:"                             // Disable MD5 (collision)
      "!PSK:"                             // Disable PSK (not needed)
      "!DSS:"                             // Disable DSA (deprecated)
      "!DHE-DSS-AES128-GCM-SHA256:"       // Explicitly disable DSS variants
      "!DHE-DSS-AES256-GCM-SHA384:"       // (rarely used, avoid confusion)
      "!EDH-RSA-DES-CBC3-SHA:"            // Legacy 3DES
      "!EDH-DSS-DES-CBC3-SHA";            // Legacy 3DES
  return true;
}
// -----------------------------------------------------------------------------

std::shared_ptr<AdminClientInfo> AdminServer::AddConnection(
    connection_hdl hdl) {
  std::unique_lock<std::mutex> lock(connections_mutex_);

  auto obj = std::make_shared<AdminClientInfo>();
  obj->index = next_conn_id_++;
  obj->hdl = hdl;
  map_id_connections_[obj->index] = obj;
  map_hdl_connections_[obj->hdl] = obj;

  return obj;
}
// -----------------------------------------------------------------------------

void AdminServer::RemoveConnection(connection_hdl hdl) {
  std::unique_lock<std::mutex> lock(connections_mutex_);

  auto it = map_hdl_connections_.find(hdl);

  if (it != map_hdl_connections_.end()) {
    auto obj = it->second;
    map_id_connections_.erase(obj->index);
    map_hdl_connections_.erase(hdl);
  }
}
// -----------------------------------------------------------------------------

std::shared_ptr<AdminClientInfo> AdminServer::GetConnectionInfo(
    connection_hdl hdl) {
  std::unique_lock<std::mutex> lock(connections_mutex_);

  auto it = map_hdl_connections_.find(hdl);

  if (it != map_hdl_connections_.end()) {
    return it->second;
  }

  return nullptr;
}
// -----------------------------------------------------------------------------

void AdminServer::OnConnected(connection_hdl hdl) {
  std::shared_ptr<AdminClientInfo> obj = AddConnection(hdl);
  auto conn = server_->get_con_from_hdl(hdl);

  logger_->info("AdminServer: Client({}) connected from {}:{}",
                obj->index,
                conn->get_remote_endpoint(),
                conn->get_port());
}
// -----------------------------------------------------------------------------

bool AdminServer::OnValidate(connection_hdl hdl) {
  auto conn = server_->get_con_from_hdl(hdl);
  conn->set_status(websocketpp::http::status_code::service_unavailable);
  return accept_client_;
}
// -----------------------------------------------------------------------------

void AdminServer::OnDisconnected(connection_hdl hdl) {
  auto conn = server_->get_con_from_hdl(hdl);
  auto it = map_hdl_connections_.find(hdl);

  if (it != map_hdl_connections_.end()) {
    auto obj = it->second;

    logger_->info("AdminServer: Client({}) disconnected {}:{}",
                  obj->index,
                  conn->get_remote_endpoint(),
                  conn->get_port());

    RemoveConnection(hdl);
  }
  else {
    logger_->info("AdminServer: Client(-) disconnected {}:{}",
                  conn->get_remote_endpoint(),
                  conn->get_port());
  }
}
// -----------------------------------------------------------------------------

void AdminServer::OnError(connection_hdl hdl) {
  auto conn = server_->get_con_from_hdl(hdl);
  logger_->error("AdminServer: Client(-) error ", conn->get_ec().message());
}
// -----------------------------------------------------------------------------

void AdminServer::OnMessage(connection_hdl hdl, message_ptr message) {
  if (message->get_opcode() != websocketpp::frame::opcode::binary)
    return;

  protocol::ClientMessage msg;
  if (!msg.ParseFromArray(message->get_payload().data(),
                          message->get_payload().size())) {
    // corrupted or incompatible
    return;
  }

  switch (msg.body_case()) {
    case protocol::ClientMessage::kLoginRequest: {
      OnLoginRequest(hdl, msg.login_request());
    } break;
    case protocol::ClientMessage::kShutdownServerRequest: {
      OnShutDownServer(hdl);
    } break;
    case protocol::ClientMessage::kRestartServerRequest: {
      OnRestartServer(hdl);
    } break;
    case protocol::ClientMessage::kActiveGameServerRequest: {
      OnActiveGameServer(hdl);
    } break;
    case protocol::ClientMessage::kDeactiveGameServerRequest: {
      OnDeactiveGameServer(hdl);
    } break;
    case protocol::ClientMessage::kDisconnectAllGameClientsRequest: {
      OnDisconnectAllGameClients(hdl);
    } break;
  }
}
// -----------------------------------------------------------------------------

bool AdminServer::Send(connection_hdl hdl,
                       const protocol::ServerMessage& message) {
  utils::bytes buffer(message.ByteSizeLong());
  message.SerializeToArray(buffer.data(), buffer.size());

  return Send(hdl, buffer);
}
// -----------------------------------------------------------------------------

bool AdminServer::Send(connection_hdl hdl, const utils::bytes &data) {
  auto obj = GetConnectionInfo(hdl);

  websocketpp::lib::error_code ec;
  server_->send(hdl,  //
                data.data(),
                data.size(),
                websocketpp::frame::opcode::binary,
                ec);

  if (ec) {
    logger_->error("AdminServer: Client({}) Send message failed: {}", obj->index, ec.message());
    return false;
  }

  return true;
}
// -----------------------------------------------------------------------------

void AdminServer::OnLoginRequest(connection_hdl hdl,
                                 const admin_auth::LoginRequest& req) {
  auto obj = GetConnectionInfo(hdl);

  if (!obj) {
    logger_->error("AdminServer::OnLoginReq: Client Info not found");
    return;
  }

  bool authed = db_manager_->AuthUser(req.username(), req.password(), obj->user);

  protocol::ServerMessage msg;
  auto res = msg.mutable_login_response();
  if (authed && obj->user.type == UserType::kAdmin) {
    res->set_result(1);
  }
  else {
    res->set_result(0);
    if (!authed) {
      res->set_reason("Invalid Credentials");
    }
    else if (obj->user.type != UserType::kAdmin) {
      res->set_reason("Insufficient privileges");
    }
  }

  Send(hdl, msg);
}
// -----------------------------------------------------------------------------

void AdminServer::OnShutDownServer(connection_hdl hdl) {
  auto obj = GetConnectionInfo(hdl);

  logger_->info("AdminServer: Client({}) Shutdown Server", obj->index);

  IpcClient::Enqueue(EventId::kAdminServer_ShutdownServer);

  protocol::ServerMessage msg;
  auto res = msg.mutable_shutdown_server_response();
  res->set_result(1);
  Send(hdl, msg);
}
// -----------------------------------------------------------------------------

void AdminServer::OnRestartServer(connection_hdl hdl) {
  auto obj = GetConnectionInfo(hdl);

  logger_->info("AdminServer: Client({}) Restart Server", obj->index);

  IpcClient::Enqueue(EventId::kAdminServer_RestartServer);

  protocol::ServerMessage msg;
  auto res = msg.mutable_restart_server_response();
  res->set_result(1);
  Send(hdl, msg);
}
// -----------------------------------------------------------------------------

void AdminServer::OnActiveGameServer(connection_hdl hdl) {
  auto obj = GetConnectionInfo(hdl);

  logger_->info("AdminServer: Client({}) Active GameServer", obj->index);

  IpcClient::Enqueue(EventId::kAdminServer_ActiveGameServer);

  protocol::ServerMessage msg;
  auto res = msg.mutable_active_game_server_response();
  res->set_result(1);
  Send(hdl, msg);
}
// -----------------------------------------------------------------------------

void AdminServer::OnDeactiveGameServer(connection_hdl hdl) {
  auto obj = GetConnectionInfo(hdl);

  logger_->info("AdminServer: Client({}) Deactive GameServer", obj->index);

  IpcClient::Enqueue(EventId::kAdminServer_DeactiveGameServer);

  protocol::ServerMessage msg;
  auto res = msg.mutable_deactive_game_server_response();
  res->set_result(1);
  Send(hdl, msg);
}
// -----------------------------------------------------------------------------

void AdminServer::OnDisconnectAllGameClients(connection_hdl hdl) {
  auto obj = GetConnectionInfo(hdl);
  logger_->info("AdminServer: Client({}) Disconnect all game clients", obj->index);
  IpcClient::Enqueue(EventId::kAdminServer_DisconnectAllGameClients);

  protocol::ServerMessage msg;
  auto res = msg.mutable_disconnect_all_game_clients_response();
  res->set_result(1);
  Send(hdl, msg);
}
// -----------------------------------------------------------------------------
