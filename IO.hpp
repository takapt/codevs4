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

    vector<Unit> get_my(const vector<UnitType>& unit_types) const;
    vector<Unit> get_enemy(const vector<UnitType>& unit_types) const;

    bool is_2p() const;
    void change_dir();
};

InputResult input();
void output(const map<int, char>& order, bool change_dir);

#endif
