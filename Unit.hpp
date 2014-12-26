#ifndef INCLUDED_UNIT
#define INCLUDED_UNIT


#include "GameCommon.hpp"
#include "Pos.hpp"

enum UnitType
{
    WORKER,
    KNIGHT,
    FIGHTER,
    ASSASSIN,
    CASTLE,
    VILLAGE,
    BASE,
};

const int SIGHT_RANGE[] = {
    4,
    4,
    4,
    4,
    10,
    10,
    4,
};
const int ATTACK_RANGE[] = {
    2,
    2,
    2,
    2,
    10,
    2,
    2,
};

struct Unit
{
    UnitType type;
    int hp;
    int id;
    Pos pos;

    int sight_range() const { return SIGHT_RANGE[type]; }
    int attack_range() const { return ATTACK_RANGE[type]; }
};

#endif
