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
#include "commandline.h"
#include "utils/fileutil.h"
#include "gothic.h"

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
  auto* inv = Lua::check<Inventory>(L, 1, "Inventory");
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

static const luaL_Reg inventory_meta[] = {
  {"items",     &ScriptEngine::luaInventoryGetItems},
  {"transfer",  &ScriptEngine::luaInventoryTransfer},
  {"itemCount", &ScriptEngine::luaInventoryItemCount},
  {nullptr,     nullptr}
  };
  
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
  
  static const luaL_Reg npc_meta[] = {
    {"inventory", &ScriptEngine::luaNpcInventory},
    {"world",     &ScriptEngine::luaNpcWorld},
    {nullptr,     nullptr}
    };
  
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

  static const luaL_Reg interactive_meta[] = {
    {"inventory",      &ScriptEngine::luaInteractiveInventory},
    {"needToLockpick", &ScriptEngine::luaInteractiveNeedToLockpick},
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

  void ScriptEngine::registerInternalAPI() {
    if(!L)
      return;

  static const luaL_Reg world_meta[] = {
    {"spellDesc", &ScriptEngine::luaGameScriptSpellDesc},
    {nullptr,     nullptr}
    };
  static const luaL_Reg empty[] = {{nullptr, nullptr}};
  Lua::registerClass(L, inventory_meta,   "Inventory",   empty);
  Lua::registerClass(L, world_meta,       "World",       empty);
  Lua::registerClass(L, npc_meta,         "Npc",         empty);
  Lua::registerClass(L, interactive_meta, "Interactive", empty);

  lua_getglobal(L, "opengothic");

  lua_pushcfunction(L, luaPrintMessage, "_printMessage");
  lua_setfield(L, -2, "_printMessage");

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
    if constexpr(std::is_same_v<T, Npc>)
      return "Npc";
    return nullptr;
    }
  }

template<class T>
void ScriptEngine::pushDispatchArg(T* arg) {
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
  }

void ScriptEngine::unbindHooks() {
  Gothic::inst().onOpen          = nullptr;
  Gothic::inst().onRansack       = nullptr;
  Gothic::inst().onNpcTakeDamage = nullptr;
  }
