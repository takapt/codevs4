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

const int CREATE_COST[] = {
    40,
    20,
    40,
    60,
    1919810,
    100,
    500,
};

const char CREATE_ORDER[] = {
    '0',
    '1',
    '2',
    '3',
    '-',
    '5',
    '6',
};

// (attacker, defencer)
const int DAMAGE_TABLE[7][7] = {
    {100, 100, 100, 100, 100, 100, 100},
    {100, 500, 200, 200, 200, 200, 200},
    {500, 1600, 500, 200, 200, 200, 200},
    {1000, 500, 1000, 500, 200, 200, 200},
    {100, 100, 100, 100, 100, 100, 100},
    {100, 100, 100, 100, 100, 100, 100},
    {100, 100, 100, 100, 100, 100, 100},
};

struct Unit
{
    UnitType type;
    int hp;
    int id;
    Pos pos;

    int sight_range() const { return SIGHT_RANGE[type]; }
    int attack_range() const { return ATTACK_RANGE[type]; }

    bool operator<(const Unit& other) const
    {
        return id < other.id;
    }
    bool operator==(const Unit& other) const
    {
        return id == other.id;
    }

    bool is_warrior() const { return type == KNIGHT || type == FIGHTER || type == ASSASSIN; }
};

map<int, int> simulate_damage(const vector<Unit>& a, const vector<Unit>& b);


#endif
