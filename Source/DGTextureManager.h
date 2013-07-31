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

#ifndef DG_TEXTUREMANAGER_H
#define DG_TEXTUREMANAGER_H

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include "Platform.h"
#include "DGTexture.h"

////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////

// TODO: This class should be a singleton

// This is temporary until we actually test how much memory is available.
// NOT accurate! It's only a reference value since textures are flushed
// before the next switch.
#define kMaxActiveTextures 18

class Config;
class Log;
class DGNode;
class DGRoom;

////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////

class DGTextureManager {
    Config& config;
    Log& log;
    
    std::thread _preloaderThread;
    
    std::vector<DGTexture*> _arrayOfActiveTextures;
    std::vector<DGTexture*> _arrayOfTextures;

    DGRoom* _roomToPreload;
    
    DGTextureManager();
    DGTextureManager(DGTextureManager const&);
    DGTextureManager& operator=(DGTextureManager const&);
    ~DGTextureManager();
    
public:
    static DGTextureManager& instance() {
        static DGTextureManager textureManager;
        return textureManager;
    }
    
    void appendTextureToBundle(const char* nameOfBundle, DGTexture* textureToAppend);
    void createBundle(const char* nameOfBundle);
    int itemsInBundle(const char* nameOfBundle);
    void flush();
    void init();
    void registerTexture(DGTexture* target);
    void requestBundle(DGNode* forNode);
    void requestTexture(DGTexture* target);
    void setRoomToPreload(DGRoom* theRoom);
    bool updatePreloader();
};

#endif // DG_TEXTUREMANAGER_H