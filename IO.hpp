#ifndef INCLUDED_IO
#define INCLUDED_IO


#include "Common.hpp"
#include "Unit.hpp"


struct InputResult
{
    int remain_ms;
    int current_stage_no;
    int current_turn;
    int resources;
    vector<Unit> my_units;
    vector<Unit> enemy_units;
    vector<Pos> resource_pos_in_sight;
};

InputResult input();
void output(const map<int, char>& order);

#endif
