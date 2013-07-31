////////////////////////////////////////////////////////////
//
// DAGON - An Adventure Game Engine
// Copyright (c) 2011-2013 Senscape s.r.l.
// All rights reserved.
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.
//
////////////////////////////////////////////////////////////

#ifndef DG_SCRIPT_H
#define DG_SCRIPT_H

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include "Action.h"
#include "DGEvent.h"
#include "Platform.h"

#include "Lua.hpp"

////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////

enum DGAttachTypes {
    AUDIO = DGObjectAudio,
    FUNCTION = kActionFunction,
    IMAGE = DGObjectTexture,
    FEED = kActionFeed,
    SWITCH = kActionSwitch,
    VIDEO = DGObjectVideo
};

// Helper to push values to Lua
#define DGLuaEnum(L, name, val) \
lua_pushnumber(L, val); \
lua_setfield(L, -2, #name);

class Config;
class Log;

////////////////////////////////////////////////////////////
// Interface - Singleton class
////////////////////////////////////////////////////////////

class DGScript {
    Config& config;
    Log& log;
    
    std::vector<std::string> _arrayOfModuleNames;
    bool _isInitialized;
    bool _isSuspended;    
    lua_State* _L;
    lua_State* _thread;
    
    int _ref;
    void _error(int result);
    static int _globalCurrentNode(lua_State *L);
    static int _globalCurrentRoom(lua_State *L);
    static int _globalCutscene(lua_State *L);
    static int _globalFeed(lua_State *L);
    static int _globalHotkey(lua_State *L);
    static int _globalLookAt(lua_State *L);    
    static int _globalPlay(lua_State *L);
    static int _globalPrint(lua_State *L);
    static int _globalQueue(lua_State *L);
    static int _globalRegister(lua_State *L);   
    static int _globalRoom(lua_State *L);
    static int _globalSetFont(lua_State *L);    
    static int _globalSleep(lua_State *L);
    static int _globalSnap(lua_State *L);
    static int _globalSwitch(lua_State *L);
    static int _globalStartTimer(lua_State *L); 
    static int _globalStopTimer(lua_State *L);
    static int _globalVersion(lua_State *L);
    void _registerEnums();
    void _registerGlobals();
    
    DGScript();
    DGScript(DGScript const&);
    DGScript& operator=(DGScript const&);
    ~DGScript();
    
public:
    static DGScript& instance() {
        static DGScript script;
        return script;
    }
    
    void init();
    const char* module();
    bool isExecutingModule();    
    void processCallback(int handler, int object);    
    void processCommand(const char* command);
    void resume();
    void run();
    void setModule(const char* module);
    int suspend();
    void unsetModule();
};

#endif // DG_SCRIPT_H