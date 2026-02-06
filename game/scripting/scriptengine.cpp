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
#include "game/damagecalculator.h"
#include "game/gamescript.h"
#include "game/serialize.h"
#include "graphics/effect.h"
#include "commandline.h"
#include "utils/fileutil.h"
#include "gothic.h"
#include "resources.h"

#include <Tempest/Dir>
#include <Tempest/TextCodec>

#include <fstream>
#include <sstream>

#include "scripting/bootstrap_lua.h"
#include "scripting/constants_lua.h"

namespace Lua {
  template<typename T>
  T** push(lua_State* L, T* obj) {
    T** ptr = reinterpret_cast<T**>(lua_newuserdata(L, sizeof(T*)));
    *ptr = obj;
    return ptr;
    }

  void setMetatable(lua_State* L, const char* name) {
    luaL_getmetatable(L, name);
    lua_setmetatable(L, -2);
    }

  template<typename T>
  T* to(lua_State* L, int idx) {
    return *reinterpret_cast<T**>(lua_touserdata(L, idx));
    }

  template<typename T>
  T* check(lua_State* L, int idx, const char* name) {
    return *reinterpret_cast<T**>(luaL_checkudata(L, idx, name));
    }

  void registerClass(lua_State* L, const luaL_Reg* methods, const char* name, const luaL_Reg* globalApi) {
    luaL_newmetatable(L, name);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    for (const luaL_Reg* l = methods; l->name != nullptr; l++) {
      lua_pushcfunction(L, l->func, l->name);
      lua_setfield(L, -2, l->name);
      }

    if(globalApi!=nullptr) {
      lua_getglobal(L, "opengothic");
      lua_pushvalue(L, -2);
      lua_setfield(L, -2, name);
      lua_pop(L, 1);
      }

    lua_pop(L,1);
    }
  }

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

  // opengothic.resolve
  lua_pushcfunction(L, luaResolveSymbol, "opengothic.resolve");
  lua_setfield(L, -2, "resolve");

  // opengothic.world
  lua_pushcfunction(L, luaGetWorld, "opengothic.world");
  lua_setfield(L, -2, "world");

  // opengothic.player
  lua_pushcfunction(L, luaGetPlayer, "opengothic.player");
  lua_setfield(L, -2, "player");

  // opengothic.daedalus
  lua_newtable(L);
  lua_pushcfunction(L, luaDaedalusCall, "daedalus.call");
  lua_setfield(L, -2, "call");
  lua_pushcfunction(L, luaDaedalusGet, "daedalus.get");
  lua_setfield(L, -2, "get");
  lua_pushcfunction(L, luaDaedalusSet, "daedalus.set");
  lua_setfield(L, -2, "set");
  lua_setfield(L, -2, "daedalus");

  // opengothic.vm
  lua_newtable(L);
  lua_pushcfunction(L, luaVmCallWithContext, "vm.callWithContext");
  lua_setfield(L, -2, "callWithContext");
  lua_pushcfunction(L, luaVmRegisterExternal, "vm.registerExternal");
  lua_setfield(L, -2, "registerExternal");
  lua_pushcfunction(L, luaVmGetSymbol, "vm.getSymbol");
  lua_setfield(L, -2, "getSymbol");
  lua_pushcfunction(L, luaVmEnumerate, "vm.enumerate");
  lua_setfield(L, -2, "enumerate");
  lua_setfield(L, -2, "vm");

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

  // Call Lua: opengothic._serializeStorage()
  lua_getglobal(L, "opengothic");
  lua_getfield(L, -1, "_serializeStorage");
  if(lua_isfunction(L, -1)) {
    if(lua_pcall(L, 0, 1, 0) == LUA_OK) {
      // Result is table {key = "value", ...}
      if(lua_istable(L, -1)) {
        lua_pushnil(L);
        while(lua_next(L, -2) != 0) {
          if(lua_isstring(L, -2) && lua_isstring(L, -1)) {
            const char* key = lua_tostring(L, -2);
            const char* val = lua_tostring(L, -1);
            data.globalData[key] = val;
            }
          lua_pop(L, 1);
          }
        }
      lua_pop(L, 1); // result
      } else {
      Log::e("[ScriptEngine] Error calling _serializeStorage: ", lua_tostring(L, -1));
      lua_pop(L, 1); // error
      }
    } else {
    lua_pop(L, 1); // non-function
    }
  lua_pop(L, 1); // opengothic

  return data;
  }

void ScriptEngine::deserialize(const ScriptData& data) {
  if(!L)
    return;

  // Build table from data
  lua_newtable(L);
  for(const auto& [key, value] : data.globalData) {
    lua_pushstring(L, value.c_str());
    lua_setfield(L, -2, key.c_str());
    }

  // Call Lua: opengothic._deserializeStorage(table)
  lua_getglobal(L, "opengothic");
  lua_getfield(L, -1, "_deserializeStorage");
  if(lua_isfunction(L, -1)) {
    lua_pushvalue(L, -3); // push the data table
    if(lua_pcall(L, 1, 0, 0) != LUA_OK) {
      Log::e("[ScriptEngine] Error calling _deserializeStorage: ", lua_tostring(L, -1));
      lua_pop(L, 1); // error
      }
    } else {
    lua_pop(L, 1); // non-function
    }
  lua_pop(L, 2); // opengothic, data table
  }

void ScriptEngine::save(Serialize& fout) const {
  ScriptData data = serialize();
  uint32_t count = static_cast<uint32_t>(data.globalData.size());
  fout.write(count);
  for(const auto& [key, value] : data.globalData) {
    fout.write(key);
    fout.write(value);
    }
  }

void ScriptEngine::load(Serialize& fin) {
  ScriptData data;
  uint32_t count = 0;
  fin.read(count);
  for(uint32_t i = 0; i < count; ++i) {
    std::string key, value;
    fin.read(key);
    fin.read(value);
    data.globalData[key] = value;
    }
  deserialize(data);
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

  // Always load constants.lua first
  if(!executeBootstrapCode(CONSTANTS_LUA, "constants")) {
    Log::e("[ScriptEngine] Failed to load constants code");
    }

  auto scriptsDir = CommandLine::inst().nestedPath({u"Data", u"opengothic", u"scripts"}, Dir::FT_Dir);
  if(scriptsDir.empty()) {
    Log::i("[ScriptEngine] No scripts directory found at Data/opengothic/scripts/");
    return;
    }

  std::vector<std::u16string> scripts;

  // Recursive directory scanner
  std::function<void(const std::u16string&)> scanDir = [&scripts, &scanDir](const std::u16string& dir) {
    Dir::scan(dir, [&scripts, &scanDir, &dir](const std::u16string& name, Dir::FileType type) {
      if(type == Dir::FT_File) {
        if(name.size() > 4 && name.substr(name.size() - 4) == u".lua") {
          scripts.push_back(dir + name);
          }
        } else if(type == Dir::FT_Dir && name != u"." && name != u"..") {
        scanDir(dir + name + u"/");
        }
      return false;
      });
    };

  scanDir(scriptsDir);

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

int ScriptEngine::luaPrintScreen(lua_State* L) {
  const char* msg = luaL_checkstring(L, 1);
  int x = static_cast<int>(luaL_checkinteger(L, 2));
  int y = static_cast<int>(luaL_checkinteger(L, 3));
  int timesec = static_cast<int>(luaL_optinteger(L, 4, 5));
  const char* font = luaL_optstring(L, 5, "font_old_10_white.tga");

  auto& fnt = Resources::font(font, Resources::FontType::Normal, 1);
  Gothic::inst().onPrintScreen(msg, x, y, timesec, fnt);
  return 0;
  }

int ScriptEngine::luaInventoryGetItems(lua_State* L) {
  auto* inv = Lua::check<Inventory>(L, 1, "Inventory");
  if(!inv) {
    lua_newtable(L);
    return 1;
    }

  lua_newtable(L);
  int idx = 1;
  for(auto it = inv->iterator(Inventory::T_Ransack); it.isValid(); ++it) {
    Item* item = const_cast<Item*>(&(*it));
    Lua::push(L, item);
    Lua::setMetatable(L, "Item");
    lua_rawseti(L, -2, idx++);
    }
  return 1;
  }

int ScriptEngine::luaInventoryTransfer(lua_State* L) {
  auto* dstInv = Lua::check<Inventory>(L, 1, "Inventory");
  auto* srcInv = Lua::check<Inventory>(L, 2, "Inventory");
  int   itemId = luaL_checkinteger(L, 3);
  int   count  = luaL_checkinteger(L, 4);
  auto* world  = Lua::check<World>(L, 5, "World");

  if(!dstInv || !srcInv || !world || itemId < 0 || count <= 0) {
    lua_pushboolean(L, false);
    return 1;
    }

  Inventory::transfer(*dstInv, *srcInv, nullptr, size_t(itemId), size_t(count), *world);
  lua_pushboolean(L, true);
  return 1;
  }

int ScriptEngine::luaInventoryItemCount(lua_State* L) {
  auto* inv = Lua::check<Inventory>(L, 1, "Inventory");
  int itemId = luaL_checkinteger(L, 2);

  if(!inv || itemId < 0) {
    lua_pushinteger(L, 0);
    return 1;
    }

  lua_pushinteger(L, int(inv->itemCount(size_t(itemId))));
  return 1;
  }

int ScriptEngine::luaInventoryAddItem(lua_State* L) {
  auto* inv  = Lua::check<Inventory>(L, 1, "Inventory");
  int itemId = luaL_checkinteger(L, 2);
  int count  = luaL_checkinteger(L, 3);

  if (!inv || itemId < 0 || count <= 0) {
    lua_pushnil(L);
    return 1;
  }

  World* world = Gothic::inst().world();
  if (!world) {
    luaL_error(L, "Inventory:addItem: no world loaded");
    return 0;
  }

  Item* addedItem = inv->addItem(static_cast<size_t>(itemId), static_cast<size_t>(count), *world);

  if (addedItem) {
    Lua::push(L, addedItem);
    Lua::setMetatable(L, "Item");
  } else {
    lua_pushnil(L);
  }
  return 1;
  }

static const luaL_Reg inventory_meta[] = {
  {"items",     &ScriptEngine::luaInventoryGetItems},
  {"transfer",  &ScriptEngine::luaInventoryTransfer},
  {"itemCount", &ScriptEngine::luaInventoryItemCount},
  {"addItem",   &ScriptEngine::luaInventoryAddItem},
  {nullptr,     nullptr}
  };

  int ScriptEngine::luaNpcGetAttribute(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    int attributeId = luaL_checkinteger(L, 2);
    if(!npc || attributeId < 0 || attributeId >= Attribute::ATR_MAX) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, npc->attribute(static_cast<Attribute>(attributeId)));
    return 1;
    }

  int ScriptEngine::luaNpcSetAttribute(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    int attributeId = luaL_checkinteger(L, 2);
    int value = luaL_checkinteger(L, 3);
    bool allowUnconscious = lua_toboolean(L, 4);
    if(!npc || attributeId < 0 || attributeId >= Attribute::ATR_MAX) {
      return 0;
      }
    npc->changeAttribute(static_cast<Attribute>(attributeId), value, allowUnconscious);
    return 0;
    }

  int ScriptEngine::luaNpcInventory(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(npc) {
      Lua::push(L, &npc->inventory());
      Lua::setMetatable(L, "Inventory");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaNpcWorld(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(npc) {
      Lua::push(L, &npc->world());
      Lua::setMetatable(L, "World");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaNpcGetLevel(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, npc->level());
    return 1;
    }

  int ScriptEngine::luaNpcGetExperience(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, npc->experience());
    return 1;
    }

  int ScriptEngine::luaNpcGetLearningPoints(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, npc->learningPoints());
    return 1;
    }

  int ScriptEngine::luaNpcGetGuild(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, npc->guild());
    return 1;
    }

  int ScriptEngine::luaNpcGetProtection(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    int protectionId = luaL_checkinteger(L, 2);
    if(!npc || protectionId < 0 || protectionId >= Protection::PROT_MAX) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, npc->protection(static_cast<Protection>(protectionId)));
    return 1;
    }

  int ScriptEngine::luaNpcGetInstanceId(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, static_cast<int>(npc->instanceSymbol()));
    return 1;
    }

  int ScriptEngine::luaNpcGetActiveWeapon(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushnil(L);
      return 1;
      }
    Item* weapon = npc->activeWeapon();
    if(weapon) {
      Lua::push(L, weapon);
      Lua::setMetatable(L, "Item");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaNpcGetActiveSpell(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushinteger(L, -1);
      return 1;
      }
    lua_pushinteger(L, npc->activeSpellLevel());
    return 1;
    }

  int ScriptEngine::luaNpcSetHealth(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    int value = luaL_checkinteger(L, 2);
    if(!npc) {
      return 0;
      }
    int currentHp = npc->attribute(Attribute::ATR_HITPOINTS);
    int delta = value - currentHp;
    npc->changeAttribute(Attribute::ATR_HITPOINTS, delta, true);
    return 0;
    }

  int ScriptEngine::luaNpcDistanceTo(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    auto* other = Lua::check<Npc>(L, 2, "Npc");
    if(npc == nullptr || other == nullptr) {
      lua_pushnumber(L, -1);
      return 1;
      }
    float dist = std::sqrt(npc->qDistTo(*other));
    lua_pushnumber(L, dist);
    return 1;
    }

  int ScriptEngine::luaNpcFlee(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(npc == nullptr)
      return 0;
    npc->aiPush(AiQueue::aiFlee());
    return 0;
    }

  int ScriptEngine::luaNpcSetTarget(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    auto* target = Lua::to<Npc>(L, 2);  // Can be nil
    if(npc == nullptr)
      return 0;
    npc->setTarget(target);
    return 0;
    }

  int ScriptEngine::luaNpcAttack(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(npc == nullptr)
      return 0;
    npc->aiPush(AiQueue::aiAttack());
    return 0;
    }

  int ScriptEngine::luaNpcClearAI(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(npc == nullptr)
      return 0;
    npc->clearAiQueue();
    return 0;
    }

  static const luaL_Reg npc_meta[] = {
    {"inventory",      &ScriptEngine::luaNpcInventory},
    {"world",          &ScriptEngine::luaNpcWorld},
    {"attribute",      &ScriptEngine::luaNpcGetAttribute},
    {"changeAttribute",&ScriptEngine::luaNpcSetAttribute},
    {"level",          &ScriptEngine::luaNpcGetLevel},
    {"experience",     &ScriptEngine::luaNpcGetExperience},
    {"learningPoints", &ScriptEngine::luaNpcGetLearningPoints},
    {"guild",          &ScriptEngine::luaNpcGetGuild},
    {"protection",     &ScriptEngine::luaNpcGetProtection},
    {"isDead",         &ScriptEngine::luaNpcIsDead},
    {"isUnconscious",  &ScriptEngine::luaNpcIsUnconscious},
    {"isDown",         &ScriptEngine::luaNpcIsDown},
    {"isPlayer",       &ScriptEngine::luaNpcIsPlayer},
    {"isTalking",      &ScriptEngine::luaNpcIsTalking},
    {"bodyState",      &ScriptEngine::luaNpcGetBodyState},
    {"hasState",       &ScriptEngine::luaNpcHasState},
    {"rotation",       &ScriptEngine::luaNpcGetRotation},
    {"rotationY",      &ScriptEngine::luaNpcGetRotationY},
    {"position",       &ScriptEngine::luaNpcGetPosition},
    {"setPosition",    &ScriptEngine::luaNpcSetPosition},
    {"setDirectionY",  &ScriptEngine::luaNpcSetDirectionY},
    {"walkMode",       &ScriptEngine::luaNpcGetWalkMode},
    {"setWalkMode",    &ScriptEngine::luaNpcSetWalkMode},
    {"talentSkill",    &ScriptEngine::luaNpcGetTalentSkill},
    {"setTalentSkill", &ScriptEngine::luaNpcSetTalentSkill},
    {"talentValue",    &ScriptEngine::luaNpcGetTalentValue},
    {"hitChance",      &ScriptEngine::luaNpcGetHitChance},
    {"attitude",       &ScriptEngine::luaNpcGetAttitude},
    {"setAttitude",    &ScriptEngine::luaNpcSetAttitude},
    {"displayName",    &ScriptEngine::luaNpcGetDisplayName},
    {"item",           &ScriptEngine::luaNpcGetItem},
    {"instanceId",     &ScriptEngine::luaNpcGetInstanceId},
    {"activeWeapon",   &ScriptEngine::luaNpcGetActiveWeapon},
    {"activeSpell",    &ScriptEngine::luaNpcGetActiveSpell},
    {"setHealth",      &ScriptEngine::luaNpcSetHealth},
    {"distanceTo",     &ScriptEngine::luaNpcDistanceTo},
    {"flee",           &ScriptEngine::luaNpcFlee},
    {"setTarget",      &ScriptEngine::luaNpcSetTarget},
    {"attack",         &ScriptEngine::luaNpcAttack},
    {"clearAI",        &ScriptEngine::luaNpcClearAI},
    {nullptr,          nullptr}
    };

  int ScriptEngine::luaNpcHasState(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    int stateId = luaL_checkinteger(L, 2);
    if(!npc) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, npc->hasState(static_cast<BodyState>(stateId)));
    return 1;
    }

  int ScriptEngine::luaNpcGetBodyState(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, static_cast<int>(npc->bodyState()));
    return 1;
    }

  int ScriptEngine::luaNpcIsDown(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, npc->isDown());
    return 1;
    }

  int ScriptEngine::luaNpcIsPlayer(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, npc->isPlayer());
    return 1;
    }

  int ScriptEngine::luaNpcIsTalking(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, npc->isTalk());
    return 1;
    }

  int ScriptEngine::luaNpcIsUnconscious(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, npc->isUnconscious());
    return 1;
    }

  int ScriptEngine::luaNpcIsDead(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, npc->isDead());
    return 1;
    }

  int ScriptEngine::luaNpcSetWalkMode(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    int mode = luaL_checkinteger(L, 2);
    if(npc) {
      npc->setWalkMode(static_cast<WalkBit>(mode));
      }
    return 0;
    }

  int ScriptEngine::luaNpcGetWalkMode(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, static_cast<int>(npc->walkMode()));
    return 1;
    }

  int ScriptEngine::luaNpcSetDirectionY(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    float rotation = static_cast<float>(luaL_checknumber(L, 2));
    if(npc) {
      npc->setDirectionY(rotation);
      }
    return 0;
    }

  int ScriptEngine::luaNpcSetPosition(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    float z = static_cast<float>(luaL_checknumber(L, 4));
    if(npc) {
      npc->setPosition(x, y, z);
      }
    return 0;
    }

  int ScriptEngine::luaNpcGetPosition(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushnumber(L, 0.0f);
      lua_pushnumber(L, 0.0f);
      lua_pushnumber(L, 0.0f);
      return 3;
      }
    Tempest::Vec3 pos = npc->position();
    lua_pushnumber(L, pos.x);
    lua_pushnumber(L, pos.y);
    lua_pushnumber(L, pos.z);
    return 3;
    }

  int ScriptEngine::luaNpcGetRotationY(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushnumber(L, 0.0f);
      return 1;
      }
    lua_pushnumber(L, npc->rotationY());
    return 1;
    }

  int ScriptEngine::luaNpcGetRotation(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushnumber(L, 0.0f);
      return 1;
      }
    lua_pushnumber(L, npc->rotation());
    return 1;
    }

  int ScriptEngine::luaNpcSetAttitude(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    int attitudeId = luaL_checkinteger(L, 2);
    if(npc) {
      npc->setAttitude(static_cast<Attitude>(attitudeId));
      }
    return 0;
    }

  int ScriptEngine::luaNpcGetAttitude(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushinteger(L, Attitude::ATT_NULL);
      return 1;
      }
    lua_pushinteger(L, npc->attitude());
    return 1;
    }

  int ScriptEngine::luaNpcGetHitChance(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    int talentId = luaL_checkinteger(L, 2);
    if(!npc || talentId < 0 || talentId >= Talent::TALENT_MAX_G2) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, npc->hitChance(static_cast<Talent>(talentId)));
    return 1;
    }

  int ScriptEngine::luaNpcGetTalentValue(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    int talentId = luaL_checkinteger(L, 2);
    if(!npc || talentId < 0 || talentId >= Talent::TALENT_MAX_G2) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, npc->talentValue(static_cast<Talent>(talentId)));
    return 1;
    }

  int ScriptEngine::luaNpcSetTalentSkill(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    int talentId = luaL_checkinteger(L, 2);
    int level = luaL_checkinteger(L, 3);
    if(!npc || talentId < 0 || talentId >= Talent::TALENT_MAX_G2) {
      return 0;
      }
    npc->setTalentSkill(static_cast<Talent>(talentId), level);
    return 0;
    }

  int ScriptEngine::luaNpcGetTalentSkill(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    int talentId = luaL_checkinteger(L, 2);
    if(!npc || talentId < 0 || talentId >= Talent::TALENT_MAX_G2) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, npc->talentSkill(static_cast<Talent>(talentId)));
    return 1;
    }

  int ScriptEngine::luaNpcGetItem(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    size_t itemId = static_cast<size_t>(luaL_checkinteger(L, 2));
    if(!npc) {
      lua_pushnil(L);
      return 1;
      }
    Item* item = npc->getItem(itemId);
    if(item) {
      Lua::push(L, item);
      Lua::setMetatable(L, "Item");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaNpcGetDisplayName(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushstring(L, "");
      return 1;
      }
    lua_pushstring(L, std::string(npc->displayName()).c_str());
    return 1;
    }

  int ScriptEngine::luaWorldSetDayTime(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    int hour = luaL_checkinteger(L, 2);
    int minute = luaL_checkinteger(L, 3);
    if(world) {
      world->setDayTime(hour, minute);
    }
    return 0;
    }

  int ScriptEngine::luaWorldTime(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    if(!world) {
      lua_pushinteger(L, 0);
      lua_pushinteger(L, 0);
      return 2;
      }
    gtime time = world->time();
    lua_pushinteger(L, static_cast<int>(time.hour()));
    lua_pushinteger(L, static_cast<int>(time.minute()));
    return 2;
    }

  int ScriptEngine::luaWorldRemoveItem(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    auto* item = Lua::check<Item>(L, 2, "Item");
    if(world && item) {
      world->removeItem(*item);
      }
    return 0;
    }

  int ScriptEngine::luaWorldAddItemAt(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    size_t itemInstance = static_cast<size_t>(luaL_checkinteger(L, 2));
    float x = static_cast<float>(luaL_checknumber(L, 3));
    float y = static_cast<float>(luaL_checknumber(L, 4));
    float z = static_cast<float>(luaL_checknumber(L, 5));
    if(!world) {
      lua_pushnil(L);
      return 1;
      }
    Item* item = world->addItem(itemInstance, Tempest::Vec3(x, y, z));
    if(item) {
      Lua::push(L, item);
      Lua::setMetatable(L, "Item");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaWorldAddItem(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    size_t itemInstance = static_cast<size_t>(luaL_checkinteger(L, 2));
    std::string_view waypoint = luaL_checkstring(L, 3);
    if(!world) {
      lua_pushnil(L);
      return 1;
      }
    Item* item = world->addItem(itemInstance, waypoint);
    if(item) {
      Lua::push(L, item);
      Lua::setMetatable(L, "Item");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaWorldRemoveNpc(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    auto* npc = Lua::check<Npc>(L, 2, "Npc");
    if(world && npc) {
      world->removeNpc(*npc);
      }
    return 0;
    }

  int ScriptEngine::luaWorldAddNpcAt(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    size_t npcInstance = static_cast<size_t>(luaL_checkinteger(L, 2));
    float x = static_cast<float>(luaL_checknumber(L, 3));
    float y = static_cast<float>(luaL_checknumber(L, 4));
    float z = static_cast<float>(luaL_checknumber(L, 5));
    if(!world) {
      lua_pushnil(L);
      return 1;
      }
    Npc* npc = world->addNpc(npcInstance, Tempest::Vec3(x, y, z));
    if(npc) {
      Lua::push(L, npc);
      Lua::setMetatable(L, "Npc");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaWorldAddNpc(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    size_t npcInstance = static_cast<size_t>(luaL_checkinteger(L, 2));
    std::string_view waypoint = luaL_checkstring(L, 3);
    if(!world) {
      lua_pushnil(L);
      return 1;
      }
    Npc* npc = world->addNpc(npcInstance, waypoint);
    if(npc) {
      Lua::push(L, npc);
      Lua::setMetatable(L, "Npc");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaWorldFindItem(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    size_t itemInstance = static_cast<size_t>(luaL_checkinteger(L, 2));
    size_t n = static_cast<size_t>(luaL_optinteger(L, 3, 0));
    if(!world) {
      lua_pushnil(L);
      return 1;
      }
    Item* item = world->findItemByInstance(itemInstance, n);
    if(item) {
      Lua::push(L, item);
      Lua::setMetatable(L, "Item");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaWorldFindNpc(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    size_t npcInstance = static_cast<size_t>(luaL_checkinteger(L, 2));
    size_t n = static_cast<size_t>(luaL_optinteger(L, 3, 0));
    if(!world) {
      lua_pushnil(L);
      return 1;
      }
    Npc* npc = world->findNpcByInstance(npcInstance, n);
    if(npc) {
      Lua::push(L, npc);
      Lua::setMetatable(L, "Npc");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaWorldFindInteractive(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    size_t instanceId = static_cast<size_t>(luaL_checkinteger(L, 2));
    if(!world) {
      lua_pushnil(L);
      return 1;
      }
    Interactive* interactive = world->mobsiById(static_cast<uint32_t>(instanceId));
    if(interactive) {
      Lua::push(L, interactive);
      Lua::setMetatable(L, "Interactive");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaWorldGetPlayer(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    if(!world) {
      lua_pushnil(L);
      return 1;
      }
    Npc* player = world->player();
    if(player) {
      Lua::push(L, player);
      Lua::setMetatable(L, "Npc");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaWorldPlaySound(lua_State* L) {
    Lua::check<World>(L, 1, "World");
    const char* soundName = luaL_checkstring(L, 2);
    Gothic::inst().emitGlobalSound(soundName);
    return 0;
    }

  int ScriptEngine::luaWorldDay(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    if(!world) {
      lua_pushinteger(L, 0);
      return 1;
      }
    gtime t = world->time();
    lua_pushinteger(L, static_cast<int>(t.day()));
    return 1;
    }

  int ScriptEngine::luaWorldPlayEffect(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    const char* effectName = luaL_checkstring(L, 2);
    float x = static_cast<float>(luaL_checknumber(L, 3));
    float y = static_cast<float>(luaL_checknumber(L, 4));
    float z = static_cast<float>(luaL_checknumber(L, 5));
    if(!world) {
      return 0;
      }
    const VisualFx* vfx = Gothic::inst().loadVisualFx(effectName);
    if(vfx) {
      Effect e(*vfx, *world, Tempest::Vec3(x, y, z));
      world->runEffect(std::move(e));
      }
    return 0;
    }

  int ScriptEngine::luaWorldFindNpcsInRange(lua_State* L) {
    auto* world = Lua::check<World>(L, 1, "World");
    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    float z = static_cast<float>(luaL_checknumber(L, 4));
    float range = static_cast<float>(luaL_checknumber(L, 5));
    if(!world) {
      lua_newtable(L);
      return 1;
      }
    lua_newtable(L);
    int idx = 1;
    world->detectNpc(Tempest::Vec3(x, y, z), range, [L, &idx](Npc& npc) {
      Lua::push(L, &npc);
      Lua::setMetatable(L, "Npc");
      lua_rawseti(L, -2, idx++);
      });
    return 1;
    }

  int ScriptEngine::luaInteractiveDetach(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    auto* npc = Lua::check<Npc>(L, 2, "Npc");
    bool quick = lua_toboolean(L, 3);
    if(inter && npc) {
      lua_pushboolean(L, inter->detach(*npc, quick));
      return 1;
      }
    lua_pushboolean(L, false);
    return 1;
    }

  int ScriptEngine::luaInteractiveAttach(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    auto* npc = Lua::check<Npc>(L, 2, "Npc");
    if(inter && npc) {
      lua_pushboolean(L, inter->attach(*npc));
      return 1;
      }
    lua_pushboolean(L, false);
    return 1;
    }

  int ScriptEngine::luaInteractiveSetAsCracked(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    bool cracked = lua_toboolean(L, 2);
    if(inter) {
      inter->setAsCracked(cracked);
      }
    return 0;
    }

  int ScriptEngine::luaInteractiveIsCracked(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    if(!inter) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, inter->isCracked());
    return 1;
    }

  int ScriptEngine::luaInteractiveIsLadder(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    if(!inter) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, inter->isLadder());
    return 1;
    }

  int ScriptEngine::luaInteractiveIsTrueDoor(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    auto* npc = Lua::check<Npc>(L, 2, "Npc");
    if(!inter || !npc) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, inter->isTrueDoor(*npc));
    return 1;
    }

  int ScriptEngine::luaInteractiveIsDoor(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    if(!inter) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, inter->isDoor());
    return 1;
    }

  int ScriptEngine::luaInteractiveIsContainer(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    if(!inter) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, inter->isContainer());
    return 1;
    }

  int ScriptEngine::luaInteractiveInventory(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    if(inter) {
      Lua::push(L, &inter->inventory());
      Lua::setMetatable(L, "Inventory");
      } else {
      lua_pushnil(L);
      }
    return 1;
    }

  int ScriptEngine::luaInteractiveNeedToLockpick(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    auto* player = Lua::check<Npc>(L, 2, "Npc");
    if(inter && player) {
      lua_pushboolean(L, inter->needToLockpick(*player));
      } else {
      lua_pushboolean(L, false);
      }
    return 1;
    }

  int ScriptEngine::luaInteractiveGetFocusName(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    if(!inter) {
      lua_pushstring(L, "");
      return 1;
      }
    lua_pushstring(L, std::string(inter->focusName()).c_str());
    return 1;
    }

  int ScriptEngine::luaInteractiveGetSchemeName(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    if(!inter) {
      lua_pushstring(L, "");
      return 1;
      }
    lua_pushstring(L, std::string(inter->schemeName()).c_str());
    return 1;
    }

  int ScriptEngine::luaInteractiveGetState(lua_State* L) {
    auto* inter = Lua::check<Interactive>(L, 1, "Interactive");
    if(!inter) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, inter->stateId());
    return 1;
    }

  // Symbol Resolution
  int ScriptEngine::luaResolveSymbol(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    World* world = Gothic::inst().world();
    if(!world) {
      lua_pushnil(L);
      return 1;
      }
    size_t id = world->script().findSymbolIndex(name);
    if(id == size_t(-1)) {
      lua_pushnil(L);
      } else {
      lua_pushinteger(L, static_cast<lua_Integer>(id));
      }
    return 1;
    }

  // Global Accessors
  int ScriptEngine::luaGetWorld(lua_State* L) {
    World* world = Gothic::inst().world();
    if(!world) {
      lua_pushnil(L);
      return 1;
      }
    Lua::push(L, world);
    Lua::setMetatable(L, "World");
    return 1;
    }

  int ScriptEngine::luaGetPlayer(lua_State* L) {
    World* world = Gothic::inst().world();
    if(!world) {
      lua_pushnil(L);
      return 1;
      }
    Npc* player = world->player();
    if(!player) {
      lua_pushnil(L);
      return 1;
      }
    Lua::push(L, player);
    Lua::setMetatable(L, "Npc");
    return 1;
    }

  // --- Daedalus Bridge (opengothic.daedalus) ---

  // Helper: Test if userdata has specific metatable (Luau compatible)
  static bool testUserdata(lua_State* L, int idx, const char* name) {
    if(!lua_isuserdata(L, idx))
      return false;
    lua_getmetatable(L, idx);
    if(!lua_istable(L, -1)) {
      lua_pop(L, 1);
      return false;
      }
    luaL_getmetatable(L, name);
    bool result = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);
    return result;
    }

  // Helper: Push a Daedalus symbol value onto Lua stack
  static void pushDaedalusValue(lua_State* L, zenkit::DaedalusSymbol* sym, uint16_t index = 0) {
    if(!sym) {
      lua_pushnil(L);
      return;
      }

    switch(sym->type()) {
      case zenkit::DaedalusDataType::INT:
        lua_pushinteger(L, sym->get_int(index));
        break;
      case zenkit::DaedalusDataType::FLOAT:
        lua_pushnumber(L, static_cast<double>(sym->get_float(index)));
        break;
      case zenkit::DaedalusDataType::STRING:
        lua_pushstring(L, sym->get_string(index).c_str());
        break;
      case zenkit::DaedalusDataType::INSTANCE: {
        auto inst = sym->get_instance();
        if(!inst) {
          lua_pushnil(L);
          }
        else if(auto npc = std::dynamic_pointer_cast<zenkit::INpc>(inst)) {
          World* world = Gothic::inst().world();
          if(world) {
            Npc* npcObj = world->findNpcByInstance(npc->symbol_index());
            if(npcObj) {
              Lua::push(L, npcObj);
              Lua::setMetatable(L, "Npc");
              }
            else {
              lua_pushnil(L);
              }
            }
          else {
            lua_pushnil(L);
            }
          }
        else if(auto item = std::dynamic_pointer_cast<zenkit::IItem>(inst)) {
          World* world = Gothic::inst().world();
          if(world) {
            Item* itemObj = world->findItemByInstance(item->symbol_index());
            if(itemObj) {
              Lua::push(L, itemObj);
              Lua::setMetatable(L, "Item");
              }
            else {
              lua_pushnil(L);
              }
            }
          else {
            lua_pushnil(L);
            }
          }
        else {
          // Generic instance - return symbol index
          lua_pushinteger(L, static_cast<lua_Integer>(inst->symbol_index()));
          }
        break;
        }
      case zenkit::DaedalusDataType::FUNCTION:
        lua_pushinteger(L, static_cast<lua_Integer>(sym->get_int(index)));
        break;
      default:
        lua_pushnil(L);
        break;
      }
    }

  // opengothic.daedalus.call(funcName, ...) - Call a Daedalus function
  int ScriptEngine::luaDaedalusCall(lua_State* L) {
    const char* funcName = luaL_checkstring(L, 1);

    World* world = Gothic::inst().world();
    if(!world) {
      luaL_error(L, "daedalus.call: no world loaded");
      return 0;
      }

    auto& vm = world->script().getVm();
    auto* sym = vm.find_symbol_by_name(funcName);
    if(!sym) {
      luaL_error(L, "daedalus.call: function '%s' not found", funcName);
      return 0;
      }

    if(!sym->is_const() && sym->type() != zenkit::DaedalusDataType::FUNCTION) {
      luaL_error(L, "daedalus.call: '%s' is not a function", funcName);
      return 0;
      }

    // Gather arguments from Lua stack (start at index 2)
    int nargs = lua_gettop(L) - 1;

    // Push arguments onto Daedalus VM stack (in reverse order for Daedalus)
    for(int i = nargs + 1; i >= 2; --i) {
      if(lua_isnumber(L, i)) {
        double num = lua_tonumber(L, i);
        // Check if it's an integer (no fractional part)
        if(num == static_cast<double>(static_cast<int32_t>(num))) {
          vm.push_int(static_cast<int32_t>(lua_tointeger(L, i)));
          }
        else {
          vm.push_float(static_cast<float>(num));
          }
        }
      else if(lua_isstring(L, i)) {
        vm.push_string(lua_tostring(L, i));
        }
      else if(lua_isuserdata(L, i)) {
        // Check if it's an Npc or Item
        if(testUserdata(L, i, "Npc")) {
          auto* npc = Lua::check<Npc>(L, i, "Npc");
          if(npc) {
            vm.push_instance(npc->handlePtr());
            }
          else {
            vm.push_int(0);
            }
          }
        else if(testUserdata(L, i, "Item")) {
          auto* item = Lua::check<Item>(L, i, "Item");
          if(item) {
            vm.push_instance(item->handlePtr());
            }
          else {
            vm.push_int(0);
            }
          }
        else {
          vm.push_int(0);
          }
        }
      else if(lua_isnil(L, i)) {
        vm.push_int(0);
        }
      else {
        luaL_error(L, "daedalus.call: unsupported argument type at position %d", i - 1);
        return 0;
        }
      }

    // Call the function
    try {
      if(sym->rtype() == zenkit::DaedalusDataType::INT) {
        int32_t result = vm.call_function<int32_t>(sym);
        lua_pushinteger(L, result);
        return 1;
        }
      else if(sym->rtype() == zenkit::DaedalusDataType::FLOAT) {
        float result = vm.call_function<float>(sym);
        lua_pushnumber(L, static_cast<double>(result));
        return 1;
        }
      else if(sym->rtype() == zenkit::DaedalusDataType::STRING) {
        std::string result = vm.call_function<std::string>(sym);
        lua_pushstring(L, result.c_str());
        return 1;
        }
      else {
        vm.call_function<void>(sym);
        return 0;
        }
      }
    catch(const std::exception& e) {
      luaL_error(L, "daedalus.call: error calling '%s': %s", funcName, e.what());
      return 0;
      }
    }

  // opengothic.daedalus.get(varName, [index]) - Get a Daedalus global variable
  int ScriptEngine::luaDaedalusGet(lua_State* L) {
    const char* varName = luaL_checkstring(L, 1);
    uint16_t index = static_cast<uint16_t>(luaL_optinteger(L, 2, 0));

    World* world = Gothic::inst().world();
    if(!world) {
      lua_pushnil(L);
      return 1;
      }

    auto& vm = world->script().getVm();
    auto* sym = vm.find_symbol_by_name(varName);
    if(!sym) {
      lua_pushnil(L);
      return 1;
      }

    if(index >= sym->count()) {
      lua_pushnil(L);
      return 1;
      }

    pushDaedalusValue(L, sym, index);
    return 1;
    }

  // opengothic.daedalus.set(varName, value, [index]) - Set a Daedalus global variable
  int ScriptEngine::luaDaedalusSet(lua_State* L) {
    const char* varName = luaL_checkstring(L, 1);
    uint16_t index = static_cast<uint16_t>(luaL_optinteger(L, 3, 0));

    World* world = Gothic::inst().world();
    if(!world) {
      luaL_error(L, "daedalus.set: no world loaded");
      return 0;
      }

    auto& vm = world->script().getVm();
    auto* sym = vm.find_symbol_by_name(varName);
    if(!sym) {
      luaL_error(L, "daedalus.set: symbol '%s' not found", varName);
      return 0;
      }

    if(sym->is_const()) {
      luaL_error(L, "daedalus.set: cannot modify const symbol '%s'", varName);
      return 0;
      }

    if(index >= sym->count()) {
      luaL_error(L, "daedalus.set: index %d out of bounds for '%s'", static_cast<int>(index), varName);
      return 0;
      }

    switch(sym->type()) {
      case zenkit::DaedalusDataType::INT:
        if(!lua_isnumber(L, 2)) {
          luaL_error(L, "daedalus.set: expected integer for '%s'", varName);
          return 0;
          }
        sym->set_int(static_cast<int32_t>(lua_tointeger(L, 2)), index);
        break;
      case zenkit::DaedalusDataType::FLOAT:
        if(!lua_isnumber(L, 2)) {
          luaL_error(L, "daedalus.set: expected number for '%s'", varName);
          return 0;
          }
        sym->set_float(static_cast<float>(lua_tonumber(L, 2)), index);
        break;
      case zenkit::DaedalusDataType::STRING:
        if(!lua_isstring(L, 2)) {
          luaL_error(L, "daedalus.set: expected string for '%s'", varName);
          return 0;
          }
        sym->set_string(lua_tostring(L, 2), index);
        break;
      default:
        luaL_error(L, "daedalus.set: cannot set symbol '%s' of this type", varName);
        return 0;
      }

    return 0;
    }

  // --- VM Bridge (opengothic.vm) ---

  // Helper: Set context variables from a Lua table
  static bool setContextFromTable(lua_State* L, int tableIdx, zenkit::DaedalusVm& vm, World* /*world*/,
                                  std::shared_ptr<zenkit::DaedalusInstance>& prevSelf,
                                  std::shared_ptr<zenkit::DaedalusInstance>& prevOther,
                                  std::shared_ptr<zenkit::DaedalusInstance>& prevVictim,
                                  std::shared_ptr<zenkit::DaedalusInstance>& prevItem) {
    if(!lua_istable(L, tableIdx))
      return false;

    // self
    lua_getfield(L, tableIdx, "self");
    if(!lua_isnil(L, -1) && testUserdata(L, -1, "Npc")) {
      auto* npc = Lua::to<Npc>(L, -1);
      if(npc) {
        prevSelf = vm.global_self()->get_instance();
        vm.global_self()->set_instance(npc->handlePtr());
        }
      }
    lua_pop(L, 1);

    // other
    lua_getfield(L, tableIdx, "other");
    if(!lua_isnil(L, -1) && testUserdata(L, -1, "Npc")) {
      auto* npc = Lua::to<Npc>(L, -1);
      if(npc) {
        prevOther = vm.global_other()->get_instance();
        vm.global_other()->set_instance(npc->handlePtr());
        }
      }
    lua_pop(L, 1);

    // victim
    lua_getfield(L, tableIdx, "victim");
    if(!lua_isnil(L, -1) && testUserdata(L, -1, "Npc")) {
      auto* npc = Lua::to<Npc>(L, -1);
      if(npc) {
        prevVictim = vm.global_victim()->get_instance();
        vm.global_victim()->set_instance(npc->handlePtr());
        }
      }
    lua_pop(L, 1);

    // item
    lua_getfield(L, tableIdx, "item");
    if(!lua_isnil(L, -1) && testUserdata(L, -1, "Item")) {
      auto* item = Lua::to<Item>(L, -1);
      if(item) {
        prevItem = vm.global_item()->get_instance();
        vm.global_item()->set_instance(item->handlePtr());
        }
      }
    lua_pop(L, 1);

    return true;
    }

  // opengothic.vm.callWithContext(funcName, contextTable, ...) - Call with explicit context
  int ScriptEngine::luaVmCallWithContext(lua_State* L) {
    const char* funcName = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    World* world = Gothic::inst().world();
    if(!world) {
      luaL_error(L, "vm.callWithContext: no world loaded");
      return 0;
      }

    auto& vm = world->script().getVm();
    auto* sym = vm.find_symbol_by_name(funcName);
    if(!sym) {
      luaL_error(L, "vm.callWithContext: function '%s' not found", funcName);
      return 0;
      }

    // Store previous context. We always restore these values after the call,
    // including explicit/null context values.
    std::shared_ptr<zenkit::DaedalusInstance> prevSelf   = vm.global_self()->get_instance();
    std::shared_ptr<zenkit::DaedalusInstance> prevOther  = vm.global_other()->get_instance();
    std::shared_ptr<zenkit::DaedalusInstance> prevVictim = vm.global_victim()->get_instance();
    std::shared_ptr<zenkit::DaedalusInstance> prevItem   = vm.global_item()->get_instance();
    setContextFromTable(L, 2, vm, world, prevSelf, prevOther, prevVictim, prevItem);

    // Gather arguments from Lua stack (start at index 3)
    int nargs = lua_gettop(L) - 2;
    for(int i = nargs + 2; i >= 3; --i) {
      if(lua_isnumber(L, i)) {
        double num = lua_tonumber(L, i);
        if(num == static_cast<double>(static_cast<int32_t>(num))) {
          vm.push_int(static_cast<int32_t>(lua_tointeger(L, i)));
          }
        else {
          vm.push_float(static_cast<float>(num));
          }
        }
      else if(lua_isstring(L, i)) {
        vm.push_string(lua_tostring(L, i));
        }
      else if(lua_isuserdata(L, i)) {
        if(testUserdata(L, i, "Npc")) {
          auto* npc = Lua::check<Npc>(L, i, "Npc");
          if(npc) vm.push_instance(npc->handlePtr());
          else vm.push_int(0);
          }
        else if(testUserdata(L, i, "Item")) {
          auto* item = Lua::check<Item>(L, i, "Item");
          if(item) vm.push_instance(item->handlePtr());
          else vm.push_int(0);
          }
        else {
          vm.push_int(0);
          }
        }
      else {
        vm.push_int(0);
        }
      }

    int result = 0;
    try {
      if(sym->rtype() == zenkit::DaedalusDataType::INT) {
        int32_t ret = vm.call_function<int32_t>(sym);
        lua_pushinteger(L, ret);
        result = 1;
        }
      else if(sym->rtype() == zenkit::DaedalusDataType::FLOAT) {
        float ret = vm.call_function<float>(sym);
        lua_pushnumber(L, static_cast<double>(ret));
        result = 1;
        }
      else if(sym->rtype() == zenkit::DaedalusDataType::STRING) {
        std::string ret = vm.call_function<std::string>(sym);
        lua_pushstring(L, ret.c_str());
        result = 1;
        }
      else {
        vm.call_function<void>(sym);
        result = 0;
        }
      }
    catch(const std::exception& e) {
      // Restore context before error
      vm.global_self()->set_instance(prevSelf);
      vm.global_other()->set_instance(prevOther);
      vm.global_victim()->set_instance(prevVictim);
      vm.global_item()->set_instance(prevItem);
      luaL_error(L, "vm.callWithContext: error calling '%s': %s", funcName, e.what());
      return 0;
      }

    // Restore context
    vm.global_self()->set_instance(prevSelf);
    vm.global_other()->set_instance(prevOther);
    vm.global_victim()->set_instance(prevVictim);
    vm.global_item()->set_instance(prevItem);

    return result;
    }

  // opengothic.vm.registerExternal(name, luaFunc) - Register Lua function as Daedalus external
  int ScriptEngine::luaVmRegisterExternal(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // Get ScriptEngine pointer from registry
    lua_getfield(L, LUA_REGISTRYINDEX, "ScriptEngine");
    auto* engine = static_cast<ScriptEngine*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if(!engine) {
      luaL_error(L, "vm.registerExternal: engine not available");
      return 0;
      }

    World* world = Gothic::inst().world();
    if(!world) {
      luaL_error(L, "vm.registerExternal: no world loaded");
      return 0;
      }

    // Store Lua function reference
    lua_pushvalue(L, 2);
    int ref = lua_ref(L, LUA_REGISTRYINDEX);
    std::string nameStr = name;
    engine->luaExternals[nameStr] = ref;

    // Register external with VM
    auto& vm = world->script().getVm();

    vm.register_external(nameStr, [engine, nameStr](/* variadic args handled via VM stack */) -> int {
      // Get the Lua function
      auto it = engine->luaExternals.find(nameStr);
      if(it == engine->luaExternals.end()) {
        Log::e("[ScriptEngine] Lua external '", nameStr, "' not found");
        return 0;
        }

      lua_rawgeti(engine->L, LUA_REGISTRYINDEX, it->second);
      if(!lua_isfunction(engine->L, -1)) {
        lua_pop(engine->L, 1);
        Log::e("[ScriptEngine] Lua external '", nameStr, "' is not a function");
        return 0;
        }

      // Call the Lua function (no args for now - complex arg passing would need VM stack inspection)
      if(lua_pcall(engine->L, 0, 1, 0) != 0) {
        Log::e("[ScriptEngine] Lua external '", nameStr, "' error: ", lua_tostring(engine->L, -1));
        lua_pop(engine->L, 1);
        return 0;
        }

      // Get return value
      int result = 0;
      if(lua_isnumber(engine->L, -1)) {
        result = static_cast<int>(lua_tonumber(engine->L, -1));
        }
      lua_pop(engine->L, 1);

      return result;
      });

    Log::i("[ScriptEngine] Registered Lua external: ", name);
    return 0;
    }

  // opengothic.vm.getSymbol(name) - Get symbol information
  int ScriptEngine::luaVmGetSymbol(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    World* world = Gothic::inst().world();
    if(!world) {
      lua_pushnil(L);
      return 1;
      }

    auto& vm = world->script().getVm();
    auto* sym = vm.find_symbol_by_name(name);
    if(!sym) {
      lua_pushnil(L);
      return 1;
      }

    lua_newtable(L);

    lua_pushstring(L, sym->name().c_str());
    lua_setfield(L, -2, "name");

    lua_pushinteger(L, static_cast<lua_Integer>(sym->index()));
    lua_setfield(L, -2, "index");

    lua_pushinteger(L, static_cast<lua_Integer>(sym->count()));
    lua_setfield(L, -2, "count");

    lua_pushboolean(L, sym->is_const());
    lua_setfield(L, -2, "isConst");

    const char* typeName = "unknown";
    switch(sym->type()) {
      case zenkit::DaedalusDataType::VOID:     typeName = "void"; break;
      case zenkit::DaedalusDataType::FLOAT:    typeName = "float"; break;
      case zenkit::DaedalusDataType::INT:      typeName = "int"; break;
      case zenkit::DaedalusDataType::STRING:   typeName = "string"; break;
      case zenkit::DaedalusDataType::CLASS:    typeName = "class"; break;
      case zenkit::DaedalusDataType::FUNCTION: typeName = "function"; break;
      case zenkit::DaedalusDataType::PROTOTYPE:typeName = "prototype"; break;
      case zenkit::DaedalusDataType::INSTANCE: typeName = "instance"; break;
      }
    lua_pushstring(L, typeName);
    lua_setfield(L, -2, "type");

    // Include value for simple types
    if(sym->type() == zenkit::DaedalusDataType::INT && sym->count() == 1) {
      lua_pushinteger(L, sym->get_int());
      lua_setfield(L, -2, "value");
      }
    else if(sym->type() == zenkit::DaedalusDataType::FLOAT && sym->count() == 1) {
      lua_pushnumber(L, static_cast<double>(sym->get_float()));
      lua_setfield(L, -2, "value");
      }
    else if(sym->type() == zenkit::DaedalusDataType::STRING && sym->count() == 1) {
      lua_pushstring(L, sym->get_string().c_str());
      lua_setfield(L, -2, "value");
      }

    return 1;
    }

  // opengothic.vm.enumerate(className, callback) - Enumerate symbols of a class
  int ScriptEngine::luaVmEnumerate(lua_State* L) {
    const char* className = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    World* world = Gothic::inst().world();
    if(!world) {
      return 0;
      }

    auto& script = world->script();
    auto& vm = script.getVm();
    size_t count = script.symbolsCount();

    for(size_t i = 0; i < count; ++i) {
      auto* sym = vm.find_symbol_by_index(static_cast<uint32_t>(i));
      if(!sym)
        continue;

      // Filter by class if specified
      if(className[0] != '\0') {
        // Check if symbol's parent class matches
        uint32_t parentIdx = sym->parent();
        if(parentIdx == uint32_t(-1))
          continue;
        auto* parentSym = vm.find_symbol_by_index(parentIdx);
        if(!parentSym || parentSym->name() != className)
          continue;
        }

      // Call callback with symbol info table
      lua_pushvalue(L, 2); // callback

      lua_newtable(L);
      lua_pushstring(L, sym->name().c_str());
      lua_setfield(L, -2, "name");
      lua_pushinteger(L, static_cast<lua_Integer>(sym->index()));
      lua_setfield(L, -2, "index");

      if(lua_pcall(L, 1, 1, 0) != 0) {
        Log::e("[ScriptEngine] vm.enumerate callback error: ", lua_tostring(L, -1));
        lua_pop(L, 1);
        break;
        }

      // Check if callback returned false to stop iteration
      if(lua_isboolean(L, -1) && !lua_toboolean(L, -1)) {
        lua_pop(L, 1);
        break;
        }
      lua_pop(L, 1);
      }

    return 0;
    }

  // Tempest::Signal Handlers implementations
  void ScriptEngine::onStartGameHandler(std::string_view worldName) {
    (void)dispatchEvent("onStartGame", std::string(worldName).c_str());
    }

  void ScriptEngine::onLoadGameHandler(std::string_view savegameName) {
    (void)dispatchEvent("onLoadGame", std::string(savegameName).c_str());
    }

  void ScriptEngine::onSaveGameHandler(std::string_view slotName, std::string_view userName) {
    (void)dispatchEvent("onSaveGame", std::string(slotName).c_str(), std::string(userName).c_str());
    }

  void ScriptEngine::onWorldLoadedHandler() {
    reregisterLuaExternals();
    (void)dispatchEvent("onWorldLoaded");
    }

  void ScriptEngine::reregisterLuaExternals() {
    if(luaExternals.empty())
      return;

    World* world = Gothic::inst().world();
    if(!world)
      return;

    auto& vm = world->script().getVm();

    for(const auto& [nameStr, ref] : luaExternals) {
      vm.register_external(nameStr, [this, nameStr](/* variadic args handled via VM stack */) -> int {
        auto it = luaExternals.find(nameStr);
        if(it == luaExternals.end()) {
          Log::e("[ScriptEngine] Lua external '", nameStr, "' not found");
          return 0;
          }

        lua_rawgeti(L, LUA_REGISTRYINDEX, it->second);
        if(!lua_isfunction(L, -1)) {
          lua_pop(L, 1);
          Log::e("[ScriptEngine] Lua external '", nameStr, "' is not a function");
          return 0;
          }

        if(lua_pcall(L, 0, 1, 0) != 0) {
          Log::e("[ScriptEngine] Lua external '", nameStr, "' error: ", lua_tostring(L, -1));
          lua_pop(L, 1);
          return 0;
          }

        int result = 0;
        if(lua_isnumber(L, -1)) {
          result = static_cast<int>(lua_tonumber(L, -1));
          }
        lua_pop(L, 1);

        return result;
        });
      Log::i("[ScriptEngine] Re-registered Lua external: ", nameStr);
      }
    }

  void ScriptEngine::onStartLoadingHandler() {
    (void)dispatchEvent("onStartLoading");
    }

  void ScriptEngine::onSessionExitHandler() {
    (void)dispatchEvent("onSessionExit");
    }

  void ScriptEngine::onSettingsChangedHandler() {
    (void)dispatchEvent("onSettingsChanged");
    }

  static const luaL_Reg interactive_meta[] = {
    {"inventory",      &ScriptEngine::luaInteractiveInventory},
    {"needToLockpick", &ScriptEngine::luaInteractiveNeedToLockpick},
    {"isContainer",    &ScriptEngine::luaInteractiveIsContainer},
    {"isDoor",         &ScriptEngine::luaInteractiveIsDoor},
    {"isTrueDoor",     &ScriptEngine::luaInteractiveIsTrueDoor},
    {"isLadder",       &ScriptEngine::luaInteractiveIsLadder},
    {"isCracked",      &ScriptEngine::luaInteractiveIsCracked},
    {"setAsCracked",   &ScriptEngine::luaInteractiveSetAsCracked},
    {"attach",         &ScriptEngine::luaInteractiveAttach},
    {"detach",         &ScriptEngine::luaInteractiveDetach},
    {"focusName",      &ScriptEngine::luaInteractiveGetFocusName},
    {"schemeName",     &ScriptEngine::luaInteractiveGetSchemeName},
    {"state",          &ScriptEngine::luaInteractiveGetState},
    {nullptr,          nullptr}
    };

  int ScriptEngine::luaDamageCalculatorDamageTypeMask(lua_State* L) {
    auto* npc = Lua::check<Npc>(L, 1, "Npc");
    if(!npc) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, DamageCalculator::damageTypeMask(*npc));
    return 1;
    }

  int ScriptEngine::luaDamageCalculatorDamageValue(lua_State* L) {
    auto* attacker = Lua::check<Npc>(L, 1, "Npc");
    auto* victim   = Lua::check<Npc>(L, 2, "Npc");
    bool  isSpell  = lua_toboolean(L, 3);

    if(!attacker || !victim) {
      lua_pushinteger(L, 0);
      lua_pushboolean(L, false);
      return 2;
      }

    DamageCalculator::Damage dmg = {};

    // Read damage array from table at arg 4
    if(lua_istable(L, 4)) {
      for(size_t i = 0; i < zenkit::DamageType::NUM; ++i) {
        lua_rawgeti(L, 4, int(i));
        if(lua_isnumber(L, -1))
          dmg[i] = int32_t(lua_tointeger(L, -1));
        lua_pop(L, 1);
        }
      }

    auto result = DamageCalculator::damageValue(*attacker, *victim, nullptr, isSpell, dmg, COLL_DOEVERYTHING);

    lua_pushinteger(L, result.value);
    lua_pushboolean(L, result.hasHit);
    return 2;
    }

  int ScriptEngine::luaGameScriptSpellDesc(lua_State* L) {
    auto* world   = Lua::check<World>(L, 1, "World");
    int   spellId = luaL_checkinteger(L, 2);

    if(!world || spellId <= 0) {
      lua_pushnil(L);
      return 1;
      }

    const auto& spell = world->script().spellDesc(spellId);

    lua_newtable(L);
    lua_pushinteger(L, spell.damage_per_level);
    lua_setfield(L, -2, "damagePerLevel");
    lua_pushinteger(L, spell.damage_type);
    lua_setfield(L, -2, "damageType");
    lua_pushinteger(L, spell.spell_type);
    lua_setfield(L, -2, "spellType");
    lua_pushnumber(L, double(spell.time_per_mana));
    lua_setfield(L, -2, "timePerMana");

    return 1;
    }

  static const luaL_Reg damagecalculator_funcs[] = {
    {"damageTypeMask", &ScriptEngine::luaDamageCalculatorDamageTypeMask},
    {"damageValue",    &ScriptEngine::luaDamageCalculatorDamageValue},
    {nullptr,          nullptr}
    };

  static const luaL_Reg gamescript_funcs[] = {
    {"spellDesc", &ScriptEngine::luaGameScriptSpellDesc},
    {nullptr,     nullptr}
    };

  int ScriptEngine::luaItemIsRune(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, item->isRune());
    return 1;
    }

  int ScriptEngine::luaItemIsSpell(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, item->isSpell());
    return 1;
    }

  int ScriptEngine::luaItemIsSpellOrRune(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, item->isSpellOrRune());
    return 1;
    }

  int ScriptEngine::luaItemIsSpellShoot(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, item->isSpellShoot());
    return 1;
    }

  int ScriptEngine::luaItemIsArmor(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, item->isArmor());
    return 1;
    }

  int ScriptEngine::luaItemIsRing(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, item->isRing());
    return 1;
    }

  int ScriptEngine::luaItemIsCrossbow(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, item->isCrossbow());
    return 1;
    }

  int ScriptEngine::luaItemIs2H(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, item->is2H());
    return 1;
    }

  int ScriptEngine::luaItemIsMulti(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, item->isMulti());
    return 1;
    }

  int ScriptEngine::luaItemIsGold(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, item->isGold());
    return 1;
    }

  int ScriptEngine::luaItemIsMission(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, item->isMission());
    return 1;
    }

  int ScriptEngine::luaItemIsEquipped(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushboolean(L, false);
      return 1;
      }
    lua_pushboolean(L, item->isEquipped());
    return 1;
    }

  int ScriptEngine::luaItemGetClsId(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, static_cast<int>(item->clsId()));
    return 1;
    }

  int ScriptEngine::luaItemSetCount(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    size_t count = static_cast<size_t>(luaL_checkinteger(L, 2));
    if(item) {
      item->setCount(count);
      }
    return 0;
    }

  int ScriptEngine::luaItemGetCount(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, static_cast<int>(item->count()));
    return 1;
    }

  int ScriptEngine::luaItemGetSellCost(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, item->sellCost());
    return 1;
    }

  int ScriptEngine::luaItemGetCost(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, item->cost());
    return 1;
    }

  int ScriptEngine::luaItemGetDescription(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushstring(L, "");
      return 1;
      }
    lua_pushstring(L, std::string(item->description()).c_str());
    return 1;
    }

  int ScriptEngine::luaItemGetDisplayName(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushstring(L, "");
      return 1;
      }
    lua_pushstring(L, std::string(item->displayName()).c_str());
    return 1;
    }

  int ScriptEngine::luaItemGetWeight(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, item->handle().weight);
    return 1;
    }

  int ScriptEngine::luaItemGetDamage(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, item->handle().damage_total);
    return 1;
    }

  int ScriptEngine::luaItemGetDamageType(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, item->handle().damage_type);
    return 1;
    }

  int ScriptEngine::luaItemGetProtection(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    int protectionId = luaL_checkinteger(L, 2);
    if(!item || protectionId < 0 || protectionId >= Protection::PROT_MAX) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, item->handle().protection[protectionId]);
    return 1;
    }

  int ScriptEngine::luaItemGetRange(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, item->handle().range);
    return 1;
    }

  int ScriptEngine::luaItemGetFlags(lua_State* L) {
    auto* item = Lua::check<Item>(L, 1, "Item");
    if(!item) {
      lua_pushinteger(L, 0);
      return 1;
      }
    lua_pushinteger(L, item->itemFlag());
    return 1;
    }

static const luaL_Reg item_meta[] = {
    {"displayName",     &ScriptEngine::luaItemGetDisplayName},
    {"description",     &ScriptEngine::luaItemGetDescription},
    {"cost",            &ScriptEngine::luaItemGetCost},
    {"sellCost",        &ScriptEngine::luaItemGetSellCost},
    {"count",           &ScriptEngine::luaItemGetCount},
    {"setCount",        &ScriptEngine::luaItemSetCount},
    {"clsId",           &ScriptEngine::luaItemGetClsId},
    {"isEquipped",      &ScriptEngine::luaItemIsEquipped},
    {"isMission",       &ScriptEngine::luaItemIsMission},
    {"isGold",          &ScriptEngine::luaItemIsGold},
    {"isMulti",         &ScriptEngine::luaItemIsMulti},
    {"is2H",            &ScriptEngine::luaItemIs2H},
    {"isCrossbow",      &ScriptEngine::luaItemIsCrossbow},
    {"isRing",          &ScriptEngine::luaItemIsRing},
    {"isArmor",         &ScriptEngine::luaItemIsArmor},
    {"isSpellShoot",    &ScriptEngine::luaItemIsSpellShoot},
    {"isSpellOrRune",   &ScriptEngine::luaItemIsSpellOrRune},
    {"isSpell",         &ScriptEngine::luaItemIsSpell},
    {"isRune",          &ScriptEngine::luaItemIsRune},
    {"weight",          &ScriptEngine::luaItemGetWeight},
    {"damage",          &ScriptEngine::luaItemGetDamage},
    {"damageType",      &ScriptEngine::luaItemGetDamageType},
    {"protection",      &ScriptEngine::luaItemGetProtection},
    {"range",           &ScriptEngine::luaItemGetRange},
    {"flags",           &ScriptEngine::luaItemGetFlags},
    {nullptr,           nullptr}
};

void ScriptEngine::registerInternalAPI() {
    if(!L)
      return;

  static const luaL_Reg world_meta[] = {
    {"spellDesc",       &ScriptEngine::luaGameScriptSpellDesc},
    {"time",            &ScriptEngine::luaWorldTime},
    {"setDayTime",      &ScriptEngine::luaWorldSetDayTime},
    {"addNpc",          &ScriptEngine::luaWorldAddNpc},
    {"addNpcAt",        &ScriptEngine::luaWorldAddNpcAt},
    {"removeNpc",       &ScriptEngine::luaWorldRemoveNpc},
    {"addItem",         &ScriptEngine::luaWorldAddItem},
    {"addItemAt",       &ScriptEngine::luaWorldAddItemAt},
    {"removeItem",      &ScriptEngine::luaWorldRemoveItem},
    {"findNpc",         &ScriptEngine::luaWorldFindNpc},
    {"findItem",        &ScriptEngine::luaWorldFindItem},
    {"findInteractive", &ScriptEngine::luaWorldFindInteractive},
    {"player",          &ScriptEngine::luaWorldGetPlayer},
    {"playSound",       &ScriptEngine::luaWorldPlaySound},
    {"playEffect",       &ScriptEngine::luaWorldPlayEffect},
    {"day",              &ScriptEngine::luaWorldDay},
    {"findNpcsInRange",  &ScriptEngine::luaWorldFindNpcsInRange},
    {nullptr,            nullptr}
    };
  static const luaL_Reg empty[] = {{nullptr, nullptr}};
  Lua::registerClass(L, inventory_meta,   "Inventory",   empty);
  Lua::registerClass(L, item_meta,        "Item",        empty);
  Lua::registerClass(L, world_meta,       "World",       empty);
  Lua::registerClass(L, npc_meta,         "Npc",         empty);
  Lua::registerClass(L, interactive_meta, "Interactive", empty);

  lua_getglobal(L, "opengothic");

  lua_pushcfunction(L, luaPrintMessage, "_printMessage");
  lua_setfield(L, -2, "_printMessage");

  lua_pushcfunction(L, luaPrintScreen, "_printScreen");
  lua_setfield(L, -2, "_printScreen");

  // Register DamageCalculator as opengothic.DamageCalculator
  lua_newtable(L);
  for(const luaL_Reg* l = damagecalculator_funcs; l->name != nullptr; l++) {
    lua_pushcfunction(L, l->func, l->name);
    lua_setfield(L, -2, l->name);
    }
  lua_setfield(L, -2, "DamageCalculator");

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
  if(!executeBootstrapCode(BOOTSTRAP_LUA, "bootstrap"))
    Log::e("[ScriptEngine] Failed to load bootstrap code");
  }

namespace {
  template<class T>
  const char* getMetatableName() {
    if constexpr(std::is_same_v<T, Inventory>)
      return "Inventory";
    if constexpr(std::is_same_v<T, World>)
      return "World";
    if constexpr(std::is_same_v<T, Interactive>)
      return "Interactive";
    if constexpr(std::is_same_v<T, Item>)
      return "Item";
    if constexpr(std::is_same_v<T, Npc>)
      return "Npc";
    return nullptr;
    }
  }

template<class T>
void ScriptEngine::pushDispatchArg(T* arg) {
  if(arg == nullptr) {
    lua_pushnil(L);
    return;
    }

  const char* metatableName = getMetatableName<std::remove_const_t<T>>();
  if(metatableName) {
    Lua::push(L, const_cast<std::remove_const_t<T>*>(arg));
    Lua::setMetatable(L, metatableName);
    } else {
    lua_pushlightuserdata(L, const_cast<std::remove_const_t<T>*>(arg));
    }
  }

template<typename... Args>
bool ScriptEngine::dispatchEvent(const char* eventName, Args... args) {
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
  (pushDispatchArg(args), ...);

  int nargs = 1 + sizeof...(args);
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
  bind(Gothic::inst().onOpen, "onOpen", std::function([](Npc& p, Interactive& c) {
    return std::make_tuple(&p, &c);
    }));

  bind(Gothic::inst().onRansack, "onRansack", std::function([](Npc& p, Npc& t) {
    return std::make_tuple(&p, &t);
    }));

  bind(Gothic::inst().onNpcTakeDamage, "onNpcTakeDamage", std::function([](Npc& victim, Npc& attacker, bool isSpell, int spellId) {
    return std::make_tuple(&victim, &attacker, isSpell, spellId);
    }));

  bind(Gothic::inst().onNpcDeath, "onNpcDeath", std::function([](Npc& victim, Npc* killer, bool isDeath) {
    return std::make_tuple(&victim, killer, isDeath);
    }));

  bind(Gothic::inst().onItemPickup, "onItemPickup", std::function([](Npc& npc, Item& item) {
    return std::make_tuple(&npc, &item);
    }));

  bind(Gothic::inst().onDialogStart, "onDialogStart", std::function([](Npc& npc, Npc& player) {
    return std::make_tuple(&npc, &player);
    }));

  bind(Gothic::inst().onDialogOption, "onDialogOption", std::function([](Npc& npc, Npc& player, std::string_view infoName) {
    return std::make_tuple(&npc, &player, std::string(infoName));
    }));

  bind(Gothic::inst().onSpellCast, "onSpellCast", std::function([](Npc& caster, Npc* target, int spellId) {
    return std::make_tuple(&caster, target, spellId);
    }));

  bind(Gothic::inst().onUseItem, "onUseItem", std::function([](Npc& npc, Item& item) {
    return std::make_tuple(&npc, &item);
    }));

  bind(Gothic::inst().onEquip, "onEquip", std::function([](Npc& npc, Item& item) {
    return std::make_tuple(&npc, &item);
    }));

  bind(Gothic::inst().onUnequip, "onUnequip", std::function([](Npc& npc, Item& item) {
    return std::make_tuple(&npc, &item);
    }));

  bind(Gothic::inst().onDropItem, "onDropItem", std::function([](Npc& npc, size_t itemId, size_t count) {
    return std::make_tuple(&npc, static_cast<int>(itemId), static_cast<int>(count));
    }));

  bind(Gothic::inst().onDrawWeapon, "onDrawWeapon", std::function([](Npc& npc, int weaponType) {
    return std::make_tuple(&npc, weaponType);
    }));

  bind(Gothic::inst().onCloseWeapon, "onCloseWeapon", std::function([](Npc& npc) {
    return std::make_tuple(&npc);
    }));

  bind(Gothic::inst().onNpcPerception, "onNpcPerception", std::function([](Npc& npc, Npc& other, int percType) {
    return std::make_tuple(&npc, &other, percType);
    }));

  bind(Gothic::inst().onTrade, "onTrade", std::function([](Npc& buyer, Npc& seller, size_t itemId, size_t count, bool isBuying) {
    return std::make_tuple(&buyer, &seller, static_cast<int>(itemId), static_cast<int>(count), isBuying);
    }));

  bind(Gothic::inst().onMobInteract, "onMobInteract", std::function([](Npc& npc, Interactive& mob) {
    return std::make_tuple(&npc, &mob);
    }));

  bind(Gothic::inst().onJump, "onJump", std::function([](Npc& npc) {
    return std::make_tuple(&npc);
    }));

  // Notification-only hooks (return value ignored by C++ caller)
  Gothic::inst().onNpcSpawn = [this](Npc& npc) {
    dispatchEvent("onNpcSpawn", &npc);
    };

  Gothic::inst().onNpcRemove = [this](Npc& npc) {
    dispatchEvent("onNpcRemove", &npc);
    };

  Gothic::inst().onSwimStart = [this](Npc& npc) {
    dispatchEvent("onSwimStart", &npc);
    };

  Gothic::inst().onSwimEnd = [this](Npc& npc) {
    dispatchEvent("onSwimEnd", &npc);
    };

  Gothic::inst().onDiveStart = [this](Npc& npc) {
    dispatchEvent("onDiveStart", &npc);
    };

  Gothic::inst().onDiveEnd = [this](Npc& npc) {
    dispatchEvent("onDiveEnd", &npc);
    };

  // New lifecycle and settings hooks (using Tempest::Signal::bind)
  Gothic::inst().onStartGame.bind(this, &ScriptEngine::onStartGameHandler);
  Gothic::inst().onLoadGame.bind(this, &ScriptEngine::onLoadGameHandler);
  Gothic::inst().onSaveGame.bind(this, &ScriptEngine::onSaveGameHandler);
  Gothic::inst().onWorldLoaded.bind(this, &ScriptEngine::onWorldLoadedHandler);
  Gothic::inst().onStartLoading.bind(this, &ScriptEngine::onStartLoadingHandler);
  Gothic::inst().onSessionExit.bind(this, &ScriptEngine::onSessionExitHandler);
  Gothic::inst().onSettingsChanged.bind(this, &ScriptEngine::onSettingsChangedHandler);
  }

void ScriptEngine::unbindHooks() {
  Gothic::inst().onOpen          = nullptr;
  Gothic::inst().onRansack       = nullptr;
  Gothic::inst().onNpcTakeDamage = nullptr;
  Gothic::inst().onNpcDeath      = nullptr;
  Gothic::inst().onItemPickup    = nullptr;
  Gothic::inst().onDialogStart   = nullptr;
  Gothic::inst().onDialogOption  = nullptr;
  Gothic::inst().onSpellCast     = nullptr;
  Gothic::inst().onUseItem       = nullptr;
  Gothic::inst().onEquip         = nullptr;
  Gothic::inst().onUnequip       = nullptr;
  Gothic::inst().onDropItem      = nullptr;
  Gothic::inst().onDrawWeapon    = nullptr;
  Gothic::inst().onCloseWeapon   = nullptr;
  Gothic::inst().onNpcPerception = nullptr;
  Gothic::inst().onTrade         = nullptr;
  Gothic::inst().onNpcSpawn      = nullptr;
  Gothic::inst().onNpcRemove     = nullptr;
  Gothic::inst().onMobInteract   = nullptr;
  Gothic::inst().onJump          = nullptr;
  Gothic::inst().onSwimStart     = nullptr;
  Gothic::inst().onSwimEnd       = nullptr;
  Gothic::inst().onDiveStart     = nullptr;
  Gothic::inst().onDiveEnd       = nullptr;

  // Unbind new lifecycle and settings hooks
  Gothic::inst().onStartGame.ubind(this, &ScriptEngine::onStartGameHandler);
  Gothic::inst().onLoadGame.ubind(this, &ScriptEngine::onLoadGameHandler);
  Gothic::inst().onSaveGame.ubind(this, &ScriptEngine::onSaveGameHandler);
  Gothic::inst().onWorldLoaded.ubind(this, &ScriptEngine::onWorldLoadedHandler);
  Gothic::inst().onStartLoading.ubind(this, &ScriptEngine::onStartLoadingHandler);
  Gothic::inst().onSessionExit.ubind(this, &ScriptEngine::onSessionExitHandler);
  Gothic::inst().onSettingsChanged.ubind(this, &ScriptEngine::onSettingsChangedHandler);
  }
