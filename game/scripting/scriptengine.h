#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <tuple>

#include <lua.h> // Required for pushDispatchArg overloads

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
    template<typename... Args>
    bool dispatchEvent(const char* eventName, Args... args);

    template<typename T>
    void pushDispatchArg(T* arg);

    void pushDispatchArg(int arg) {
      lua_pushinteger(L, arg);
      }

    void pushDispatchArg(float arg) {
      lua_pushnumber(L, static_cast<lua_Number>(arg));
      }

    void pushDispatchArg(bool arg) {
      lua_pushboolean(L, arg);
      }

    void pushDispatchArg(const char* arg) {
      lua_pushstring(L, arg);
      }

    template<typename... CppArgs, typename... LuaArgs>
    void bind(std::function<bool(CppArgs...)>& hook, const char* eventName, std::function<std::tuple<LuaArgs...>(CppArgs...)> argTransformer) {
      hook = [this, eventName, argTransformer](CppArgs... args) {
        auto luaArgs = argTransformer(args...);
        return std::apply([this, eventName](LuaArgs... largs) {
          return this->dispatchEvent(eventName, largs...);
          }, luaArgs);
        };
      }

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

  public:
    static int luaPrint(lua_State* L);
    static int luaPrintMessage(lua_State* L);

    static int luaInventoryGetItems(lua_State* L);
    static int luaInventoryTransfer(lua_State* L);
    static int luaInventoryItemCount(lua_State* L);

    static int luaNpcInventory(lua_State* L);
    static int luaNpcWorld(lua_State* L);

    static int luaInteractiveInventory(lua_State* L);
    static int luaInteractiveNeedToLockpick(lua_State* L);
  };
