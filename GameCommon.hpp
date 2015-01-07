#ifndef INCLUDE_GAME_COMMON
#define INCLUDE_GAME_COMMON

#include "Common.hpp"

const int BOARD_SIZE = 100;
const int MAX_RESOURCE_GAIN = 5;


enum Dir
{
    RIGHT,
    UP,
    LEFT,
    DOWN,

    INVALID,
};
int rev_dir(int dir);
Dir rev_dir(Dir dir);

const char RIGHT_ORDER = 'R';
const char UP_ORDER = 'U';
const char LEFT_ORDER = 'L';
const char DOWN_ORDER = 'D';

char to_order(Dir dir);

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
