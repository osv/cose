// Copyright (C) 2010-2011 Olexandr Sydorchuk <olexandr_syd [at] users.sourceforge.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _GAMECORE_H_
#define _GAMECORE_H_

class GameCore
{
public:
    enum GameMode {
    GAME = 1,
    VIEWER = 2  
    };
    GameCore();
    ~GameCore();

    void setGameMode(GameMode gameMode);
    GameMode getGameMode() const {
    return fGameMode; }
   
private:
    GameMode fGameMode;
};

#endif // _GAMECORE_H_
