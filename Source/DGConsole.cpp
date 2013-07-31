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

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include "DGCameraManager.h"
#include "Config.h"
#include "DGConsole.h"
#include "DGCursorManager.h"
#include "Log.h"
#include "DGFontManager.h"
#include "DGRenderManager.h"

using namespace std;

////////////////////////////////////////////////////////////
// Implementation - Constructor
////////////////////////////////////////////////////////////

DGConsole::DGConsole() :
    cameraManager(DGCameraManager::instance()),
    config(Config::instance()),
    cursorManager(DGCursorManager::instance()),
    fontManager(DGFontManager::instance()),
    log(Log::instance()),
    renderManager(DGRenderManager::instance())
{
    _command = "";
    
    _isEnabled = false;
    _isInitialized = false;
    _isReadyToProcess = false;
    _size = DGConsoleRows * (kDefFontSize + DGConsoleSpacing);
    _offset = _size;
    
    _state = DGConsoleHidden;
}

////////////////////////////////////////////////////////////
// Implementation - Destructor
////////////////////////////////////////////////////////////

DGConsole::~DGConsole() {
    // Nothing to do here
}

////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////

void DGConsole::init() {
    log.trace(kModNone, "%s", kString12003);
    _font = fontManager.loadDefault();
    _isInitialized = true;
}

void DGConsole::deleteChar() {
    if (_command.size () > 0) 
        _command.resize(_command.size () - 1);
}

void DGConsole::disable() {
    _isEnabled = false;
}

void DGConsole::enable() {
    if (_isInitialized)
        _isEnabled = true;
}

void DGConsole::execute() {
    log.command(kModScript, _command.c_str());    
    _isReadyToProcess = true;
}

void DGConsole::getCommand(char* pointerToBuffer) {
    strncpy(pointerToBuffer, _command.c_str(), kMaxLogLength);
    _command.clear();
    _isReadyToProcess = false;
}

void DGConsole::inputChar(char aKey) {
    if (_command.size() < kMaxLogLength)
        _command.insert(_command.end(), aKey);
}

bool DGConsole::isEnabled() {
    return _isEnabled;
}

bool DGConsole::isHidden() {
    if (_state == DGConsoleHidden)
        return true;
    else
        return false;
} 

bool DGConsole::isReadyToProcess() {
    return _isReadyToProcess;
}

void DGConsole::toggle() {
    switch (_state) {
        case DGConsoleHidden:
        case DGConsoleHiding:
            _state = DGConsoleShowing;
            break;            
        case DGConsoleShowing:
        case DGConsoleVisible:
            _state = DGConsoleHiding;
            break;            
    }
}

void DGConsole::update() {
    if (_isEnabled) {
        Point position = cursorManager.position();
        
        switch (_state) {
            case DGConsoleHidden:
                // Set the color used for information
                renderManager.setColor(kColorBrightCyan);
                _font->print(DGInfoMargin, DGInfoMargin, 
                             "Viewport size: %d x %d", config.displayWidth, config.displayHeight);                
                _font->print(DGInfoMargin, (DGInfoMargin * 2) + kDefFontSize, 
                             "Coordinates: (%d, %d)", (int)position.x, (int)position.y);
                _font->print(DGInfoMargin, (DGInfoMargin * 3) + (kDefFontSize * 2), 
                             "Viewing angle: %2.0f", cameraManager.fieldOfView());
                _font->print(DGInfoMargin, (DGInfoMargin * 4) + (kDefFontSize * 3), 
                             "FPS: %2.0f", config.framesPerSecond()); 
                
                break;            
            case DGConsoleHiding:
                if (_offset < _size)
                    _offset += DGConsoleSpeed;
                else 
                    _state = DGConsoleHidden;
                break;
            case DGConsoleShowing:
                if (_offset > 0 && _offset > -_size)
                    _offset -= DGConsoleSpeed;
                else
                    _state = DGConsoleVisible;
                break;
        }
        
        if (_state != DGConsoleHidden) {
            LogData logData;
            int row = (DGConsoleRows - 2); // Extra row saved for prompt
            
            int coords[] = { 0, -_offset,
                config.displayWidth, -_offset,
                config.displayWidth, _size - _offset,
                0, _size - _offset };
            
            // Draw the slide
            renderManager.disableTextures();
            renderManager.setColor(0x25AA0000); // TODO: Add a mask color to achieve this (ie: Red + Shade)
            renderManager.drawSlide(coords);
            renderManager.enableTextures();
            
            if (log.beginIteratingHistory()) {
                do {
                    log.getCurrentLine(&logData);
                    
                    // Draw the current line
                    renderManager.setColor(logData.color);
                    _font->print(DGConsoleMargin, ((DGConsoleSpacing + kDefFontSize) * row) - _offset, logData.line.c_str());
                    
                    row--;
                } while (log.iterateHistory());
            }
            
            renderManager.setColor(kColorBrightGreen);
            _font->print(DGConsoleMargin, (_size - (DGConsoleSpacing + kDefFontSize)) - _offset, ">%s_", _command.c_str());
        }
    }
}