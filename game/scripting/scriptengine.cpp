#include "scriptengine.h"

#include <Tempest/Log>

#include <lua.h>
#include <lualib.h>
#include <luacodegen.h>
#include <Luau/Compiler.h>
#include <Luau/CodeGen.h>

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

  Log::i("[ScriptEngine] Initialized");
  }

void ScriptEngine::shutdown() {
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

  lua_newtable(L);

  lua_newtable(L);
  lua_pushstring(L, "0.1.0");
  lua_setfield(L, -2, "VERSION");
  lua_setfield(L, -2, "core");

  lua_setglobal(L, "opengothic");
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

void ScriptEngine::fireEvent(const std::string& eventName, const std::vector<void*>& args) {
  // TODO: implement event system
  (void)eventName;
  (void)args;
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
