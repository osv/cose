#include <iostream>

#include "gamecore.h"

using namespace std;

GameCore::GameCore()
{

}

GameCore::~GameCore()
{

}

void GameCore::setGameMode(GameMode gameMode) 
{
    fGameMode = gameMode;
    switch(gameMode)
    {
    case VIEWER:
    cout << " >>> Viewer mode <<<\n";
    break;
    case GAME:
    cout << " >>> Game mode <<<\n";
    break;
    }
}


// // pool of all body selections refered by UI
// std::vector <Selection> selectionPool; 

// // add selection in pool for alive monitoring
// bool addSelToPool(Selection sel)
// {
//     selectionPool.push_back(sel);
//     cout << "added: " << sel.getName() << "\tTotal" << selectionPool.size()<<"\n";
// }

// // remove all sel from pool 
// void removeSelFromPool(Selection &sel)
// {
//     for (vector<Selection>::iterator i = selectionPool.begin();
//   i != selectionPool.end(); i++)
//     {
//  if (i->getType() != Selection::Type_Nil &&
//      *i==sel)
//  {
//      cout << "removed: " << sel.getName() << "\tTotal" << selectionPool.size()<<"\n";
//      return;
//  }
//     }
// }

// // clear all selection in pool given by sel arg
// void clearSelsInPool(Selection &sel)
// {
//     for (vector<Selection>::iterator i = selectionPool.begin();
//   i != selectionPool.end(); i++)
//     {
//  if (i->getType() != Selection::Type_Nil &&
//      *i==sel)
//  {
//      *i=Selection();
//      selectionPool.erase(i);
//      cout << "remove one: " << sel.getName() << "\tTotal" << selectionPool.size()<<"\n";
//      --i;
//  }
//     }
// }
