#include "Header.hpp"
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

InputResult input_result();
