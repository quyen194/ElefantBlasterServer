# Common Utilities

## SettingsManager

Server configuration management with auto-save and default config fallback.

### Directory Structure

```
ElefantBlasterServer/
└── assets/
    └── configs/
        ├── default/          # Default configs (committed to git)
        │   ├── app.json
        │   └── logging.json
        │   └── server.json
        └── app.json          # User config (gitignored, auto-generated)
```

### Features

- ✅ **Auto-load from default**: Automatically copies from `assets/configs/default/` if config doesn't exist
- ✅ **Auto-save**: Automatically saves after a configurable interval (default 60s)
- ✅ **Thread-safe**: Uses mutex for safe multi-threaded access
- ✅ **JSON support**: Supports nested keys with JSON pointer syntax
- ✅ **Logging**: Logs to `logs/all.json.log`

### Basic Usage

```cpp
#include "common/settings_manager.hpp"

using namespace common;

void Example() {
  // Get instance (singleton)
  auto& settings = SettingsManager::Instance();

  // Load config (auto-copies from default if doesn't exist)
  if (!settings.Load("app.json")) {
    // Handle error
  }

  // Enable auto-save every 30 seconds
  settings.SetAutoSaveInterval(30);
  settings.StartAutoSave();

  // Read values
  int port = settings.Get<int>("game_port", 9002);
  std::string host = settings.Get<std::string>("host", "0.0.0.0");

  // Write values (automatically marks dirty for auto-save)
  settings.Set("game_port", 9003);

  // Save immediately
  settings.Save();

  // Stop auto-save when not needed
  settings.StopAutoSave();
}
```

### Nested Keys with JSON Pointer

```cpp
// Config file: app.json
{
  "network": {
    "host": "0.0.0.0",
    "ports": {
      "game": 9002,
      "admin": 9003
    }
  }
}

// Code
auto& settings = SettingsManager::Instance();
settings.Load("app.json");

// Read nested value
std::string host = settings.GetNested<std::string>("/network/host", "localhost");
int game_port = settings.GetNested<int>("/network/ports/game", 8080);

// Write nested value
settings.SetNested("/network/ports/admin", 9004);
```

### Auto-save Behavior

- Auto-save only writes file when changes exist (dirty flag)
- Interval is configurable (unit: seconds)
- Automatically saves on destructor
- Thread-safe for concurrent Get/Set operations

### Best Practices

1. **Load config early**: Call `Load()` during application initialization
2. **Use default values**: Always provide default values for `Get()`
3. **Gitignore user configs**: Only commit `assets/configs/default/`, ignore `assets/configs/*.json`
4. **Auto-save interval**: Choose appropriate interval (30-60s for production)
5. **Manual save**: Call `Save()` before critical shutdown

### Example: ProcessControl

```cpp
void ProcessControl::Create() {
  auto& settings = SettingsManager::Instance();

  // Load and enable auto-save
  if (!settings.Load("app.json")) {
    logger_->warn("Using default settings");
  }
  settings.SetAutoSaveInterval(30);
  settings.StartAutoSave();

  // Use config values
  int game_port = settings.Get<int>("game_port", 9002);
  int admin_port = settings.Get<int>("admin_port", 9003);

  // ... use settings
}
```

### Logging

All operations are logged to `logs/Common.json.log`:

```json
{"ts":"2025-12-01T08:04:12.345Z","class":"SettingsManager","lvl":"info","tid":12345,"src":"settings_manager.cpp:85","func":"Load","msg":"Settings loaded from: assets/configs/app.json"}
{"ts":"2025-12-01T08:04:12.367Z","class":"SettingsManager","lvl":"debug","tid":12345,"src":"settings_manager.cpp:87","func":"Load","msg":"{\"event\":\"settings_load\",\"path\":\"assets/configs/app.json\",\"keys\":4}"}
```
