#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

struct lua_State;

class Npc;
class Interactive;
class Inventory;
class World;

class ScriptEngine final {
  public:
    ScriptEngine();
    ~ScriptEngine();

    void initialize();
    void shutdown();

    bool loadGlobalScript(const std::string& filepath);
    bool loadScriptsFromManifest(const std::string& manifestPath);

    void update(float dt);

    std::string executeString(const std::string& code);

    struct ScriptData {
      std::unordered_map<std::string, std::string> globalData;
      };
    ScriptData serialize() const;
    void deserialize(const ScriptData& data);

    std::vector<std::string> getLoadedScripts() const;
    void reloadAllScripts();
    void loadModScripts();
    void bindHooks();
    void unbindHooks();

  private:
    // Fire event via Lua dispatcher - returns true if handled
    bool dispatchEvent(const char* eventName, std::initializer_list<void*> handles);
    struct ScriptInfo {
      std::string filepath;
      std::string source;
      std::string bytecode;
      };

    lua_State*              L = nullptr;
    std::vector<ScriptInfo> loadedScripts;
    bool                    jitEnabled = false;
    std::string*            consoleOutput = nullptr;

    void setupSandbox();
    void registerCoreFunctions();
    void registerInternalAPI();
    void loadBootstrap();
    void enableJIT();
    bool compileScript(const std::string& source, std::string& outBytecode);
    bool executeBootstrapCode(const char* code, const char* name);

    static int luaPrint(lua_State* L);
    static int luaInventoryGetItems(lua_State* L);
    static int luaInventoryTransferAll(lua_State* L);
  };
