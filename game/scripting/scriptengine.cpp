#include "scriptengine.h"

#include <Tempest/Log>

#include <lua.h>
#include <lualib.h>
#include <luacodegen.h>
#include <Luau/Compiler.h>
#include <Luau/CodeGen.h>

#include "world/objects/npc.h"
#include "world/objects/item.h"
#include "world/objects/interactive.h"
#include "world/world.h"
#include "game/inventory.h"
#include "commandline.h"
#include "utils/fileutil.h"
#include "gothic.h"

#include <Tempest/Dir>
#include <Tempest/TextCodec>

#include <fstream>
#include <sstream>

using namespace Tempest;

ScriptEngine::ScriptEngine() {
  }

ScriptEngine::~ScriptEngine() {
  shutdown();
  }

void ScriptEngine::initialize() {
  L = luaL_newstate();
  if(!L) {
    Log::e("[ScriptEngine] Failed to create Lua state");
    return;
    }

  luaL_openlibs(L);
  setupSandbox();
  registerCoreFunctions();
  enableJIT();
  bindHooks();

  Log::i("[ScriptEngine] Initialized");
  }

void ScriptEngine::shutdown() {
  unbindHooks();
  if(L) {
    lua_close(L);
    L = nullptr;
    }
  loadedScripts.clear();
  Log::i("[ScriptEngine] Shutdown");
  }

void ScriptEngine::setupSandbox() {
  if(!L)
    return;

  lua_pushnil(L);
  lua_setglobal(L, "dofile");
  lua_pushnil(L);
  lua_setglobal(L, "loadfile");

  lua_getglobal(L, "os");
  if(lua_istable(L, -1)) {
    lua_pushnil(L);
    lua_setfield(L, -2, "execute");
    lua_pushnil(L);
    lua_setfield(L, -2, "exit");
    lua_pushnil(L);
    lua_setfield(L, -2, "remove");
    lua_pushnil(L);
    lua_setfield(L, -2, "rename");
    }
  lua_pop(L, 1);

  lua_pushnil(L);
  lua_setglobal(L, "io");
  }

int ScriptEngine::luaPrint(lua_State* L) {
  // Get ScriptEngine pointer from registry
  lua_getfield(L, LUA_REGISTRYINDEX, "ScriptEngine");
  auto* engine = static_cast<ScriptEngine*>(lua_touserdata(L, -1));
  lua_pop(L, 1);

  std::stringstream ss;
  int n = lua_gettop(L);
  for(int i = 1; i <= n; i++) {
    if(i > 1)
      ss << "\t";
    if(lua_isstring(L, i))
      ss << lua_tostring(L, i);
    else if(lua_isnumber(L, i))
      ss << lua_tonumber(L, i);
    else if(lua_isboolean(L, i))
      ss << (lua_toboolean(L, i) ? "true" : "false");
    else if(lua_isnil(L, i))
      ss << "nil";
    else
      ss << lua_typename(L, lua_type(L, i));
    }

  std::string output = ss.str();
  if(engine && engine->consoleOutput) {
    if(!engine->consoleOutput->empty())
      engine->consoleOutput->append("\n");
    engine->consoleOutput->append(output);
    }
  else {
    Log::i("[Lua] ", output);
    }

  return 0;
  }

void ScriptEngine::registerCoreFunctions() {
  if(!L)
    return;

  // Store 'this' pointer in registry for callbacks
  lua_pushlightuserdata(L, this);
  lua_setfield(L, LUA_REGISTRYINDEX, "ScriptEngine");

  // Override print function
  lua_pushcfunction(L, luaPrint, "print");
  lua_setglobal(L, "print");

  // Create opengothic table
  lua_newtable(L);

  // opengothic.core
  lua_newtable(L);
  lua_pushstring(L, "0.1.0");
  lua_setfield(L, -2, "VERSION");
  lua_setfield(L, -2, "core");

  lua_setglobal(L, "opengothic");

  // Register internal API and load bootstrap
  registerInternalAPI();
  loadBootstrap();
  }

void ScriptEngine::enableJIT() {
#if defined(__x86_64__) || defined(__aarch64__) || defined(_M_X64)
  if(L && luau_codegen_supported()) {
    Luau::CodeGen::create(L);
    jitEnabled = true;
    Log::i("[ScriptEngine] JIT enabled");
    } else {
    Log::i("[ScriptEngine] JIT not supported on this platform");
    }
#else
  Log::i("[ScriptEngine] JIT not available (architecture not supported)");
#endif
  }

bool ScriptEngine::compileScript(const std::string& source, std::string& outBytecode) {
  Luau::CompileOptions options;
  options.optimizationLevel = 2;
  options.debugLevel = 1;

  outBytecode = Luau::compile(source, options);
  return !outBytecode.empty();
  }

bool ScriptEngine::loadGlobalScript(const std::string& filepath) {
  if(!L) {
    Log::e("[ScriptEngine] Not initialized");
    return false;
    }

  std::ifstream file(filepath);
  if(!file.is_open()) {
    Log::e("[ScriptEngine] Failed to open: ", filepath);
    return false;
    }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  std::string bytecode;
  if(!compileScript(source, bytecode)) {
    Log::e("[ScriptEngine] Failed to compile: ", filepath);
    return false;
    }

  if(luau_load(L, filepath.c_str(), bytecode.data(), bytecode.size(), 0) != 0) {
    Log::e("[ScriptEngine] Load error: ", lua_tostring(L, -1));
    lua_pop(L, 1);
    return false;
    }

  if(lua_pcall(L, 0, 1, 0) != 0) {
    Log::e("[ScriptEngine] Runtime error: ", lua_tostring(L, -1));
    lua_pop(L, 1);
    return false;
    }

  ScriptInfo info;
  info.filepath = filepath;
  info.source   = source;
  info.bytecode = bytecode;
  loadedScripts.push_back(info);

  if(lua_istable(L, -1)) {
    lua_getfield(L, -1, "engineHandlers");
    if(lua_istable(L, -1)) {
      lua_getfield(L, -1, "onInit");
      if(lua_isfunction(L, -1)) {
        if(lua_pcall(L, 0, 0, 0) != 0) {
          Log::e("[ScriptEngine] onInit error: ", lua_tostring(L, -1));
          lua_pop(L, 1);
          }
        } else {
        lua_pop(L, 1);
        }
      }
    lua_pop(L, 1);
    }
  lua_pop(L, 1);

  Log::i("[ScriptEngine] Loaded: ", filepath);
  return true;
  }

bool ScriptEngine::loadScriptsFromManifest(const std::string& manifestPath) {
  std::ifstream file(manifestPath);
  if(!file.is_open()) {
    Log::e("[ScriptEngine] Failed to open manifest: ", manifestPath);
    return false;
    }

  std::string line;
  int loadedCount = 0;

  while(std::getline(file, line)) {
    if(line.empty() || line[0] == '#')
      continue;

    size_t colonPos = line.find(':');
    if(colonPos == std::string::npos)
      continue;

    std::string scriptType = line.substr(0, colonPos);
    std::string scriptPath = line.substr(colonPos + 1);

    scriptPath.erase(0, scriptPath.find_first_not_of(" \t"));
    scriptPath.erase(scriptPath.find_last_not_of(" \t\r\n") + 1);

    if(loadGlobalScript(scriptPath))
      loadedCount++;
    }

  Log::i("[ScriptEngine] Loaded ", loadedCount, " scripts from manifest");
  return loadedCount > 0;
  }

void ScriptEngine::update(float dt) {
  if(!L)
    return;
  // TODO: call onUpdate handlers for loaded scripts
  (void)dt;
  }

std::string ScriptEngine::executeString(const std::string& code) {
  if(!L)
    return "Error: ScriptEngine not initialized";

  std::string bytecode;
  if(!compileScript(code, bytecode))
    return "Error: Compilation failed";

  if(luau_load(L, "console", bytecode.data(), bytecode.size(), 0) != 0) {
    std::string error = lua_tostring(L, -1);
    lua_pop(L, 1);
    return "Error: " + error;
    }

  // Capture print output during execution
  std::string printOutput;
  consoleOutput = &printOutput;

  int status = lua_pcall(L, 0, LUA_MULTRET, 0);

  consoleOutput = nullptr;

  if(status != 0) {
    std::string error = lua_tostring(L, -1);
    lua_pop(L, 1);
    return "Error: " + error;
    }

  std::stringstream result;
  int nresults = lua_gettop(L);

  for(int i = 1; i <= nresults; i++) {
    if(lua_isstring(L, i))
      result << lua_tostring(L, i);
    else if(lua_isnumber(L, i))
      result << lua_tonumber(L, i);
    else if(lua_isboolean(L, i))
      result << (lua_toboolean(L, i) ? "true" : "false");
    else if(lua_isnil(L, i))
      result << "nil";
    else
      result << lua_typename(L, lua_type(L, i));

    if(i < nresults)
      result << ", ";
    }

  lua_settop(L, 0);

  // Combine print output and return values
  std::string returnValue = result.str();
  if(!printOutput.empty() && !returnValue.empty())
    return printOutput + "\n" + returnValue;
  if(!printOutput.empty())
    return printOutput;
  return returnValue;
  }

ScriptEngine::ScriptData ScriptEngine::serialize() const {
  ScriptData data;
  // TODO: implement proper save state serialization
  return data;
  }

void ScriptEngine::deserialize(const ScriptData& data) {
  // TODO: implement proper save state restoration
  (void)data;
  }

std::vector<std::string> ScriptEngine::getLoadedScripts() const {
  std::vector<std::string> scripts;
  for(const auto& info : loadedScripts)
    scripts.push_back(info.filepath);
  return scripts;
  }

void ScriptEngine::reloadAllScripts() {
  Log::i("[ScriptEngine] Reloading all scripts...");

  std::vector<std::string> paths;
  for(const auto& info : loadedScripts)
    paths.push_back(info.filepath);

  loadedScripts.clear();

  for(const auto& path : paths)
    loadGlobalScript(path);
  }

void ScriptEngine::loadModScripts() {
  using namespace Tempest;

  auto scriptsDir = CommandLine::inst().nestedPath({u"Data", u"opengothic", u"scripts"}, Dir::FT_Dir);
  if(scriptsDir.empty()) {
    Log::i("[ScriptEngine] No scripts directory found at Data/opengothic/scripts/");
    return;
    }

  std::vector<std::u16string> scripts;
  Dir::scan(scriptsDir, [&scripts, &scriptsDir](const std::u16string& name, Dir::FileType type) {
    if(type == Dir::FT_File) {
      if(name.size() > 4 && name.substr(name.size() - 4) == u".lua") {
        scripts.push_back(scriptsDir + name);
        }
      }
    return false;
    });

  if(scripts.empty()) {
    Log::i("[ScriptEngine] No .lua scripts found in Data/opengothic/scripts/");
    return;
    }

  Log::i("[ScriptEngine] Found ", scripts.size(), " script(s) to load");

  for(const auto& script : scripts) {
    auto path = TextCodec::toUtf8(script);
    loadGlobalScript(path);
    }
  }

// --- Internal API (low-level, _ prefixed) ---

int ScriptEngine::luaPrintMessage(lua_State* L) {
  const char* msg = luaL_checkstring(L, 1);
  Gothic::inst().onPrint(msg);
  return 0;
  }

int ScriptEngine::luaInventoryGetItems(lua_State* L) {
  auto* inv = static_cast<Inventory*>(lua_touserdata(L, 1));
  if(!inv) {
    lua_newtable(L);
    return 1;
    }

  lua_newtable(L);
  int idx = 1;
  for(auto it = inv->iterator(Inventory::T_Ransack); it.isValid(); ++it) {
    lua_newtable(L);

    lua_pushinteger(L, int(it->clsId()));
    lua_setfield(L, -2, "id");

    auto name = it->displayName();
    lua_pushlstring(L, name.data(), name.size());
    lua_setfield(L, -2, "name");

    lua_pushinteger(L, int(it.count()));
    lua_setfield(L, -2, "count");

    lua_pushboolean(L, it.isEquipped());
    lua_setfield(L, -2, "equipped");

    lua_rawseti(L, -2, idx++);
    }
  return 1;
  }

int ScriptEngine::luaInventoryTransferAll(lua_State* L) {
  auto* srcInv = static_cast<Inventory*>(lua_touserdata(L, 1));
  auto* dstInv = static_cast<Inventory*>(lua_touserdata(L, 2));
  auto* world  = static_cast<World*>(lua_touserdata(L, 3));

  if(!srcInv || !dstInv || !world) {
    lua_newtable(L);
    return 1;
    }

  // Collect non-equipped items with names to avoid iterator invalidation
  struct ItemInfo {
    size_t      id;
    size_t      count;
    std::string name;
    };
  std::vector<ItemInfo> items;
  for(auto it = srcInv->iterator(Inventory::T_Ransack); it.isValid(); ++it) {
    if(!it.isEquipped())
      items.push_back({it->clsId(), it.count(), std::string(it->displayName())});
    }

  for(auto& item : items)
    Inventory::transfer(*dstInv, *srcInv, nullptr, item.id, item.count, *world);

  // Return table of transferred items
  lua_newtable(L);
  int idx = 1;
  for(auto& item : items) {
    lua_newtable(L);
    lua_pushstring(L, item.name.c_str());
    lua_setfield(L, -2, "name");
    lua_pushinteger(L, int(item.count));
    lua_setfield(L, -2, "count");
    lua_rawseti(L, -2, idx++);
    }
  return 1;
  }

void ScriptEngine::registerInternalAPI() {
  if(!L)
    return;

  lua_getglobal(L, "opengothic");

  lua_pushcfunction(L, luaPrintMessage, "_printMessage");
  lua_setfield(L, -2, "_printMessage");

  lua_pushcfunction(L, luaInventoryGetItems, "_inventoryGetItems");
  lua_setfield(L, -2, "_inventoryGetItems");

  lua_pushcfunction(L, luaInventoryTransferAll, "_inventoryTransferAll");
  lua_setfield(L, -2, "_inventoryTransferAll");

  lua_pop(L, 1);
  }

bool ScriptEngine::executeBootstrapCode(const char* code, const char* name) {
  std::string bytecode;
  if(!compileScript(code, bytecode)) {
    Log::e("[ScriptEngine] Failed to compile bootstrap: ", name);
    return false;
    }

  if(luau_load(L, name, bytecode.data(), bytecode.size(), 0) != 0) {
    Log::e("[ScriptEngine] Bootstrap load error: ", lua_tostring(L, -1));
    lua_pop(L, 1);
    return false;
    }

  if(lua_pcall(L, 0, 0, 0) != 0) {
    Log::e("[ScriptEngine] Bootstrap runtime error: ", lua_tostring(L, -1));
    lua_pop(L, 1);
    return false;
    }

  return true;
  }

void ScriptEngine::loadBootstrap() {
  const char* bootstrap = R"lua(
-- Inventory class (wraps C++ Inventory handle)
local Inventory = {}
Inventory.__index = Inventory

function Inventory.new(handle, worldHandle)
    return setmetatable({
        _handle = handle,
        _world = worldHandle
    }, Inventory)
end

function Inventory:items()
    return opengothic._inventoryGetItems(self._handle)
end

function Inventory:transferAllTo(target)
    return opengothic._inventoryTransferAll(self._handle, target._handle, self._world)
end

function Inventory:hasItems()
    local items = self:items()
    for _, item in ipairs(items) do
        if not item.equipped then
            return true
        end
    end
    return false
end

-- Event system
opengothic.events = {
    _handlers = {}
}

function opengothic.events.register(eventName, callback)
    if not opengothic.events._handlers[eventName] then
        opengothic.events._handlers[eventName] = {}
    end
    table.insert(opengothic.events._handlers[eventName], callback)
end

-- Called from C++ to dispatch events
function opengothic._dispatchEvent(eventName, playerInvHandle, targetInvHandle, worldHandle)
    local handlers = opengothic.events._handlers[eventName]
    if not handlers then
        return false
    end

    local playerInv = Inventory.new(playerInvHandle, worldHandle)
    local targetInv = Inventory.new(targetInvHandle, worldHandle)

    for _, handler in ipairs(handlers) do
        local handled = handler(playerInv, targetInv)
        if handled then
            return true
        end
    end
    return false
end

-- Print message to game screen
function opengothic.printMessage(msg)
    opengothic._printMessage(msg)
end

-- Export Inventory class
opengothic.Inventory = Inventory
)lua";

  if(!executeBootstrapCode(bootstrap, "bootstrap"))
    Log::e("[ScriptEngine] Failed to load bootstrap code");
  }

bool ScriptEngine::dispatchEvent(const char* eventName, std::initializer_list<void*> handles) {
  if(!L)
    return false;

  lua_getglobal(L, "opengothic");
  if(!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return false;
    }

  lua_getfield(L, -1, "_dispatchEvent");
  if(!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return false;
    }

  lua_pushstring(L, eventName);

  for(void* handle : handles)
    lua_pushlightuserdata(L, handle);

  int nargs = 1 + int(handles.size());
  if(lua_pcall(L, nargs, 1, 0) != 0) {
    Log::e("[ScriptEngine] Event dispatch error: ", lua_tostring(L, -1));
    lua_pop(L, 2);
    return false;
    }

  bool handled = lua_toboolean(L, -1);
  lua_pop(L, 2);
  return handled;
  }

void ScriptEngine::bindHooks() {
  Gothic::inst().onOpen = [this](Npc& player, Interactive& container) {
    return dispatchEvent("onOpen",
        {&player.inventory(), &container.inventory(), &player.world()});
    };

  Gothic::inst().onRansack = [this](Npc& player, Npc& target) {
    return dispatchEvent("onRansack",
        {&player.inventory(), &target.inventory(), &player.world()});
    };
  }

void ScriptEngine::unbindHooks() {
  Gothic::inst().onOpen    = nullptr;
  Gothic::inst().onRansack = nullptr;
  }
