/********************************************************************
  Copyright 2014, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/11/14 21:40
  filename:  ElefantBlaster/ElefantBlasterServer/network/net_server.cpp

  purpose:
*********************************************************************/


// -----------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <sstream>

#include "network/net_server.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
// -----------------------------------------------------------------------------
typedef asio::ssl::context context;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

NetServer::NetServer() {
  settings_ = SettingsManager::Instance();
  logger_ = settings_->GetLogger("app", "NetServer", true);

  settings_->SetCurrentConfig(SettingsManager::kSettingServer);
  port_ = settings_->GetNested<int>("/network/port", 9002);

  // Initialize Asio
  server_.init_asio();

  // Register our message handler
  server_.set_tls_init_handler(bind(&NetServer::OnTlsInit, this, _1));
  server_.set_open_handler(bind(&NetServer::OnConnected, this, _1));
  server_.set_close_handler(bind(&NetServer::OnDisconnected, this, _1));
  server_.set_fail_handler(bind(&NetServer::OnError, this, _1));
  server_.set_message_handler(bind(&NetServer::OnDataRecv, this, _1, _2));
}
// -----------------------------------------------------------------------------

NetServer::~NetServer() {}
// -----------------------------------------------------------------------------

bool NetServer::Start() {
  if (!LoadTlsData()) {
      logger_->error("NetServer: Failed to load TLS data, cannot start server");
      return false;
  }

  // Listen on the specified port
  server_.listen(port_);

  // Start the server accept loop
  server_.start_accept();

  // Start the ASIO io_service run loop
  logger_->info("NetServer: Starting server on port {}", port_);
  try {
      server_.run();
  } catch (const std::exception& e) {
      logger_->error("NetServer: Exception in server run loop: {}", e.what());
      return false;
  }

  return true;
}
// -----------------------------------------------------------------------------

void NetServer::Stop() {
  logger_->info("NetServer: Stopping server...");
  server_.stop_listening();
  server_.stop();
}
// -----------------------------------------------------------------------------

context_ptr NetServer::OnTlsInit(connection_hdl hdl) {
  namespace asio = websocketpp::lib::asio;

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
      logger_->error("NetServer: Error setting cipher list");
  }

  return ctx;
}
// -----------------------------------------------------------------------------

bool NetServer::LoadTlsData() {
  cert_file_data_ = ReadFileToString("server.crt");
  key_file_data_ = ReadFileToString("server.key");
  dh_params_file_data_ = ReadFileToString("dh2048.pem");
  cert_file_password_ = "";
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

std::string NetServer::ReadFileToString(const std::string& file_path) {
  std::ifstream file(file_path, std::ios::in);
  if (!file.is_open()) {
      return "";
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  return buffer.str();
}
// -----------------------------------------------------------------------------

void NetServer::OnConnected(connection_hdl hdl) {
  auto connection = server_.get_con_from_hdl(hdl);
}
// -----------------------------------------------------------------------------

void NetServer::OnDisconnected(connection_hdl hdl) {

}
// -----------------------------------------------------------------------------

void NetServer::OnError(connection_hdl hdl) {

}
// -----------------------------------------------------------------------------

void NetServer::OnDataRecv(connection_hdl hdl, message_ptr msg) {

}
// -----------------------------------------------------------------------------
