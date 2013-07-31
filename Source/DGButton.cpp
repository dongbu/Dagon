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

#include "DGButton.h"
#include "Config.h"
#include "DGFontManager.h"
#include "DGTexture.h"

using namespace std;

////////////////////////////////////////////////////////////
// Implementation - Constructor
////////////////////////////////////////////////////////////

DGButton::DGButton() :
    config(Config::instance()),
    fontManager(DGFontManager::instance())
{
    _textColor = kColorWhite;
    
    _hasAction = false;
    _hasOnHoverTexture = false;
    _hasText = false;
    
    _font = fontManager.loadDefault();
    
    this->setFadeSpeed(DGFadeNormal);
    this->setType(DGObjectButton);
}

////////////////////////////////////////////////////////////
// Implementation - Destructor
////////////////////////////////////////////////////////////

DGButton::~DGButton() {
    if (_hasAction)
        delete _actionData;
}

////////////////////////////////////////////////////////////
// Implementation - Checks
////////////////////////////////////////////////////////////

bool DGButton::hasAction() {
    return _hasAction;
}

bool DGButton::hasOnHoverTexture() {
    return _hasOnHoverTexture;
}

bool DGButton::hasFont() {
    return _hasFont;
}

bool DGButton::hasText() {
    return _hasText;
}

////////////////////////////////////////////////////////////
// Implementation - Gets
////////////////////////////////////////////////////////////

Action* DGButton::action() {
    return _actionData;
}

DGFont* DGButton::font() {
    return _font;
}

DGTexture* DGButton::onHoverTexture() {
    return _attachedOnHoverTexture;
}

const char* DGButton::text() {
    return _text.c_str();
}

int DGButton::textColor() {
    return _textColor;
}

////////////////////////////////////////////////////////////
// Implementation - Sets
////////////////////////////////////////////////////////////

void DGButton::setAction(Action* anAction) {
    _actionData = new Action;
    memcpy(_actionData, anAction, sizeof(Action));
    _hasAction = true;   
}

void DGButton::setFont(const char* fromFileName, unsigned int heightOfFont) {
    // FIXME: Wrong, this can load many repeated fonts!
    _font = fontManager.load(fromFileName, heightOfFont);
}

void DGButton::setOnHoverTexture(const char* fromFileName) {
    // TODO: Important! We should determine first if the texture already exists,
    // to avoid repeating resources. Eventually, this would be done via the
    // resource manager.
    DGTexture* texture;
    
    texture = new DGTexture;
    texture->setResource(config.path(kPathResources, fromFileName, DGObjectImage).c_str());
    texture->load();
    
    _attachedOnHoverTexture = texture;
    _hasOnHoverTexture = true;
}

void DGButton::setText(const char* text){
    _text = text;
    _hasText = true;
}

void DGButton::setTextColor(int aColor) {
    // Note this expects one of our pre-generated colors
    _textColor = aColor;
}