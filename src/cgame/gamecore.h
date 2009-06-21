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
