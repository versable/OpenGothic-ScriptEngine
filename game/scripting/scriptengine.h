#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

struct lua_State;

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

    using EventCallback = std::function<void(lua_State*)>;
    void fireEvent(const std::string& eventName, const std::vector<void*>& args);

    std::vector<std::string> getLoadedScripts() const;
    void reloadAllScripts();

  private:
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
    void enableJIT();
    bool compileScript(const std::string& source, std::string& outBytecode);

    static int luaPrint(lua_State* L);
  };
