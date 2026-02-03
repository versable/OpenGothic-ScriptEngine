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

    static int luaDamageCalculatorDamageTypeMask(lua_State* L);
    static int luaDamageCalculatorDamageValue(lua_State* L);

    static int luaGameScriptSpellDesc(lua_State* L);

    // Npc Primitives
    static int luaNpcGetAttribute(lua_State* L);
    static int luaNpcSetAttribute(lua_State* L);
    static int luaNpcGetLevel(lua_State* L);
    static int luaNpcGetExperience(lua_State* L);
    static int luaNpcGetLearningPoints(lua_State* L);
    static int luaNpcGetGuild(lua_State* L);
    static int luaNpcGetProtection(lua_State* L);

    static int luaNpcIsDead(lua_State* L);
    static int luaNpcIsUnconscious(lua_State* L);
    static int luaNpcIsDown(lua_State* L);
    static int luaNpcIsPlayer(lua_State* L);
    static int luaNpcIsTalking(lua_State* L);
    static int luaNpcGetBodyState(lua_State* L);
    static int luaNpcHasState(lua_State* L);

    static int luaNpcGetRotation(lua_State* L);
    static int luaNpcGetRotationY(lua_State* L);
    static int luaNpcGetPosition(lua_State* L);
    static int luaNpcSetPosition(lua_State* L);
    static int luaNpcSetDirectionY(lua_State* L);
    static int luaNpcGetWalkMode(lua_State* L);
    static int luaNpcSetWalkMode(lua_State* L);

    static int luaNpcGetTalentSkill(lua_State* L);
    static int luaNpcSetTalentSkill(lua_State* L);
    static int luaNpcGetTalentValue(lua_State* L);
    static int luaNpcGetHitChance(lua_State* L);

    static int luaNpcGetAttitude(lua_State* L);
    static int luaNpcSetAttitude(lua_State* L);
    static int luaNpcGetDisplayName(lua_State* L);
    static int luaNpcGetItem(lua_State* L);
    static int luaNpcGetInstanceId(lua_State* L);
    static int luaNpcGetActiveWeapon(lua_State* L);
    static int luaNpcGetActiveSpell(lua_State* L);
    static int luaNpcSetHealth(lua_State* L);

    // Item Primitives
    static int luaItemGetDisplayName(lua_State* L);
    static int luaItemGetDescription(lua_State* L);
    static int luaItemGetCost(lua_State* L);
    static int luaItemGetSellCost(lua_State* L);
    static int luaItemGetCount(lua_State* L);
    static int luaItemSetCount(lua_State* L);
    static int luaItemGetClsId(lua_State* L);
    static int luaItemIsEquipped(lua_State* L);
    static int luaItemIsMission(lua_State* L);
    static int luaItemIsGold(lua_State* L);
    static int luaItemIsMulti(lua_State* L);
    static int luaItemIs2H(lua_State* L);
    static int luaItemIsCrossbow(lua_State* L);
    static int luaItemIsRing(lua_State* L);
    static int luaItemIsArmor(lua_State* L);
    static int luaItemIsSpellShoot(lua_State* L);
    static int luaItemIsSpellOrRune(lua_State* L);
    static int luaItemIsSpell(lua_State* L);
    static int luaItemIsRune(lua_State* L);
    static int luaItemGetWeight(lua_State* L);
    static int luaItemGetDamage(lua_State* L);
    static int luaItemGetDamageType(lua_State* L);
    static int luaItemGetProtection(lua_State* L);
    static int luaItemGetRange(lua_State* L);
    static int luaItemGetFlags(lua_State* L);

    // World Primitives
    static int luaWorldTime(lua_State* L);
    static int luaWorldSetDayTime(lua_State* L);

    static int luaWorldAddNpc(lua_State* L); // addNpc(size_t npcInstance, std::string_view at)
    static int luaWorldAddNpcAt(lua_State* L); // addNpc(size_t npcInstance, const Tempest::Vec3& at)
    static int luaWorldRemoveNpc(lua_State* L);

    static int luaWorldAddItem(lua_State* L); // addItem(size_t itemInstance, std::string_view at)
    static int luaWorldAddItemAt(lua_State* L); // addItem(size_t itemInstance, const Tempest::Vec3& pos)
    static int luaWorldRemoveItem(lua_State* L);

    static int luaWorldFindNpc(lua_State* L);
    static int luaWorldFindItem(lua_State* L);
    static int luaWorldFindInteractive(lua_State* L);
    static int luaWorldGetPlayer(lua_State* L);
    static int luaWorldPlaySound(lua_State* L);
    static int luaWorldDay(lua_State* L);
    static int luaWorldPlayEffect(lua_State* L);

    // Interactive Primitives
    static int luaInteractiveIsContainer(lua_State* L);
    static int luaInteractiveIsDoor(lua_State* L);
    static int luaInteractiveIsTrueDoor(lua_State* L);
    static int luaInteractiveIsLadder(lua_State* L);
    static int luaInteractiveIsCracked(lua_State* L);
    static int luaInteractiveSetAsCracked(lua_State* L);
    static int luaInteractiveAttach(lua_State* L);
    static int luaInteractiveDetach(lua_State* L);
    static int luaInteractiveGetFocusName(lua_State* L);
    static int luaInteractiveGetSchemeName(lua_State* L);
    static int luaInteractiveGetState(lua_State* L);

    // Symbol Resolution
    static int luaResolveSymbol(lua_State* L);

  private:
    // Tempest::Signal Handlers
    void onStartGameHandler(std::string_view worldName);
    void onLoadGameHandler(std::string_view savegameName);
    void onSaveGameHandler(std::string_view slotName, std::string_view userName);
    void onWorldLoadedHandler();
    void onStartLoadingHandler();
    void onSessionExitHandler();
    void onSettingsChangedHandler();
  };
