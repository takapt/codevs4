#ifndef INCLUDE_GAME_COMMON
#define INCLUDE_GAME_COMMON

#include "Common.hpp"

const int BOARD_SIZE = 100;


enum Dir
{
    RIGHT,
    UP,
    LEFT,
    DOWN,
};

const string STR_DIR[] = {
    "RIGHT",
    "UP",
    "LEFT",
    "DOWN",
};

namespace std
{
    ostream& operator<<(ostream& os, Dir dir);
}

#endif
