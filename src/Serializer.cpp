////////////////////////////////////////////////////////////
//
// DAGON - An Adventure Game Engine
// Copyright (c) 2011-2016 Senscape s.r.l.
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

#include "Serializer.h"
#include "Audio.h"
#include "CameraManager.h"
#include "Control.h"
#include "Node.h"
#include "Room.h"
#include "Spot.h"
#include "TimerManager.h"
#include "Version.h"

#include <cstring>
#include <climits>

namespace dagon {

////////////////////////////////////////////////////////////
// Implementation - Constructor & Destructor
////////////////////////////////////////////////////////////

Serializer::Serializer(lua_State *L, SDL_RWops *rw) : _L(L), _rw(rw) {}

Serializer::~Serializer() {
  SDL_RWclose(_rw);
}

////////////////////////////////////////////////////////////
// Implementation - Private
////////////////////////////////////////////////////////////

bool Serializer::writeField(const std::string &key, const std::string &val) {
  return SDL_WriteBE16(_rw, key.length() + val.length() + 1) && // +1 because of =
         SDL_RWwrite(_rw, key.c_str(), key.length(), 1) &&
         SDL_WriteU8(_rw, '=') &&
         SDL_RWwrite(_rw, val.c_str(), val.length(), 1);
}

int32_t Serializer::writeTable(const std::string parentKey) {
  // Handle cycles
  for (const auto &table : _tblMap) {
    lua_rawgeti(_L, LUA_REGISTRYINDEX, table.first);

    if (lua_equal(_L, -1, -2)) { // Maybe a cycle...
      lua_pop(_L, 1);
      if (!writeField(parentKey, table.second))
        return -1;
      return 1;
    }
    else
      lua_pop(_L, 1);
  }

  { // Store this table
    int ref = luaL_ref(_L, LUA_REGISTRYINDEX);
    _tblMap[ref] = parentKey;
  }

  lua_pushnil(_L);
  int32_t numFields = 0;
  while (lua_next(_L, -2) != 0) {
    if (!lua_isstring(_L, -2)) {
      lua_pop(_L, 1);
      continue;
    }

    std::string val;
    std::string fullKey = parentKey + '[';
    if (lua_type(_L, -2) == LUA_TSTRING)
      fullKey += '"';
    lua_pushvalue(_L, -2);
    fullKey += lua_tostring(_L, -1);
    lua_pop(_L, 1);
    if (lua_type(_L, -2) == LUA_TSTRING)
      fullKey += '"';
    fullKey += ']';

    switch (lua_type(_L, -1)) {
    case LUA_TTABLE: {
      int32_t numNewFields = writeTable(fullKey);
      if (numNewFields < 0)
        return -1;
      numFields += numNewFields;

      lua_pop(_L, 1);
      continue;
    }
    case LUA_TBOOLEAN: {
      val = lua_toboolean(_L, -1) ? "true" : "false";
      break;
    }
    case LUA_TSTRING:
      val += '"';
      // Fallthrough
    case LUA_TNUMBER: {
      val += lua_tostring(_L, -1);
      if (val[0] == '"')
        val += '"';
      break;
    }
    default: {
      lua_pop(_L, 1);
      continue;
    }
    }

    if (!writeField(fullKey, val))
      return -1;

    numFields++;
    lua_pop(_L, 1);
  }

  return numFields;
}

int Serializer::writeFunction(lua_State *L, const void *p, size_t sz, void *ud) {
  SDL_RWops *rw = reinterpret_cast<SDL_RWops*>(ud);
  if (!SDL_RWwrite(rw, p, sz, 1))
    return 1;
  return 0;
}

////////////////////////////////////////////////////////////
// Implementation - Public
////////////////////////////////////////////////////////////

bool Serializer::writeHeader() {
  // Write version information
  {
    const size_t len = UCHAR_MAX > strlen(DAGON_VERSION_STRING) ? strlen(DAGON_VERSION_STRING) :
                                                                  UCHAR_MAX;
    if (!SDL_WriteU8(_rw, len))
      return false;
    if (!SDL_RWwrite(_rw, DAGON_VERSION_STRING, len, 1))
      return false;
  }

  // Write preview
  {
    const std::string desc = Control::instance().currentNode()->description();
    const size_t len = UCHAR_MAX > desc.length() ? desc.length() : UCHAR_MAX;

    if (!SDL_WriteU8(_rw, len))
      return false;
    if (!SDL_RWwrite(_rw, desc.c_str(), len, 1))
      return false;
  }

  // Write current room name
  const std::string name = Control::instance().currentRoom()->name();
  if (!SDL_WriteU8(_rw, name.length()))
    return false;
  if (!SDL_RWwrite(_rw, name.c_str(), name.length(), 1))
    return false;

  return true;
}

bool Serializer::writeScriptData() {
  int64_t fieldsPtr = SDL_RWtell(_rw);
  if (fieldsPtr < 0)
    return false;
  if (!SDL_WriteBE32(_rw, 0)) // 4 placeholder bytes for the actual number of fields
    return false;

  lua_getglobal(_L, "dgPersistence");
  int32_t numFields = writeTable("dgPersistence");
  if (numFields < 0)
    return false;

  if (SDL_RWseek(_rw, fieldsPtr, RW_SEEK_SET) < 0) // Restore file pointer to 4 placeholder bytes
    return false;
  if (!SDL_WriteBE32(_rw, numFields))
    return false;
  if (SDL_RWseek(_rw, 0, RW_SEEK_END) < 0) // Restore file pointer to end
    return false;

  return true;
}

bool Serializer::writeRoomData() {
  Room *room = Control::instance().currentRoom();

  // Write the enable status for the spots of all nodes
  uint16_t numNodes = 0;
  if (room->hasNodes()) {
    int64_t nodesPtr = SDL_RWtell(_rw);
    if (nodesPtr < 0)
      return false;
    if (!SDL_WriteBE16(_rw, 0)) // 2 placeholder bytes for the actual number of nodes
      return false;

    room->beginIteratingNodes();
    do {
      Node *node = room->iterator();

      uint16_t numSpots = 0;
      if (node->hasSpots()) {
        int64_t spotsPtr = SDL_RWtell(_rw);
        if (spotsPtr < 0)
          return false;
        if (!SDL_WriteBE16(_rw, 0)) // 2 placeholder bytes for the actual number of spots
          return false;

        node->beginIteratingSpots();
        do {
          Spot *spot = node->currentSpot();
          if (!SDL_WriteU8(_rw, spot->isEnabled()))
            return false;
          numSpots++;
        } while (node->iterateSpots());

        if (SDL_RWseek(_rw, spotsPtr, RW_SEEK_SET) < 0)
          return false;
      }

      if (!SDL_WriteBE16(_rw, numSpots))
        return false;
      if (SDL_RWseek(_rw, 0, RW_SEEK_END) < 0) // Restore file pointer to end
        return false;

      numNodes++;
    } while (room->iterateNodes());

    if (SDL_RWseek(_rw, nodesPtr, RW_SEEK_SET) < 0)
      return false;
  }

  if (!SDL_WriteBE16(_rw, numNodes))
    return false;
  if (SDL_RWseek(_rw, 0, RW_SEEK_END) < 0) // Restore file pointer to end
    return false;

  // Write node number
  uint16_t nodeIdx = 0;
  if (room->hasNodes()) {
    room->beginIteratingNodes();
    do {
      if (room->iterator() == room->currentNode())
        break;

      nodeIdx++;
    } while (room->iterateNodes());
  }

  if (!SDL_WriteBE16(_rw, nodeIdx))
    return false;

  // Write camera angles
  if (!SDL_WriteBE16(_rw, CameraManager::instance().angleHorizontal()))
    return false;
  if (!SDL_WriteBE16(_rw, CameraManager::instance().angleVertical()))
    return false;

  // Write audio states
  if (!SDL_WriteBE16(_rw, room->arrayOfAudios().size()))
    return false;

  for (Audio *audio : room->arrayOfAudios()) {
    if (!SDL_WriteU8(_rw, audio->state()))
      return false;
  }

  // Write timers
  int64_t timersPtr = SDL_RWtell(_rw);
  if (timersPtr < 0)
    return false;
  if (!SDL_WriteBE16(_rw, 0)) // 2 placeholder bytes for the actual number of timers
    return false;

  uint16_t numTimers = 0;
  for (const auto &timer : TimerManager::instance().timers()) {
    if (timer.type != DGTimerNormal || !timer.isEnabled ||
        (!timer.isLoopable && timer.hasTriggered))
      continue;

    double timeLeft = TimerManager::instance().timeLeft(timer);
    if (timeLeft > 0) {
      const std::string triggerStr = std::to_string(timer.trigger);
      if (!SDL_WriteU8(_rw, timer.isLoopable))
        return false;
      if (!SDL_WriteU8(_rw, triggerStr.length()))
        return false;
      if (!SDL_RWwrite(_rw, triggerStr.c_str(), triggerStr.length(), 1))
        return false;

      const std::string timerTime = std::to_string(timeLeft);
      if (!SDL_WriteU8(_rw, timerTime.length()))
        return false;
      if (!SDL_RWwrite(_rw, timerTime.c_str(), timerTime.length(), 1))
        return false;

      lua_rawgeti(_L, LUA_REGISTRYINDEX, timer.luaHandler); // Push timer function to top of stack

      int64_t funcSizePtr = SDL_RWtell(_rw);
      if (funcSizePtr < 0)
        return false;
      if (!SDL_WriteBE16(_rw, 0)) // 2 placeholder bytes for the actual function size
        return false;

      int64_t beforeFuncPtr = SDL_RWtell(_rw);
      if (beforeFuncPtr < 0)
        return false;
      int errCode = lua_dump(_L, writeFunction, _rw);
      if (errCode != 0)
        return false;
      int64_t afterFuncPtr = SDL_RWtell(_rw);
      if (afterFuncPtr < 0)
        return false;

      uint16_t numBytesWritten = afterFuncPtr - beforeFuncPtr;
      if (SDL_RWseek(_rw, funcSizePtr, RW_SEEK_SET) < 0)
        return false;
      if (!SDL_WriteBE16(_rw, numBytesWritten))
        return false;

      if (SDL_RWseek(_rw, 0, RW_SEEK_END) < 0) // Restore file pointer to end
        return false;

      numTimers++;
    }
  }

  if (SDL_RWseek(_rw, timersPtr, RW_SEEK_SET) < 0)
    return false;
  if (!SDL_WriteBE16(_rw, numTimers))
    return false;
  if (SDL_RWseek(_rw, 0, RW_SEEK_END) < 0) // Restore file pointer to end
    return false;

  // Write control mode
  if (!SDL_WriteU8(_rw, Config::instance().controlMode))
    return false;

  return true;
}

}