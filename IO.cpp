#include "IO.hpp"


Unit input_unit()
{
    Unit unit;
    int type;
    cin >> unit.id >> unit.pos.y >> unit.pos.x >> unit.hp >> type;
    unit.type = (UnitType)type;

    return unit;
}
InputResult input_result()
{
    InputResult result;

    cin >> result.remain_ms;
    cin >> result.current_stage_no;
    cin >> result.current_turn;
    cin >> result.resources;

    int num_my_units;
    cin >> num_my_units;
    rep(i, num_my_units)
        result.my_units.push_back(input_unit());

    int num_enemy_units;
    cin >> num_enemy_units;
    rep(i, num_enemy_units)
        result.enemy_units.push_back(input_unit());

    int num_resource;
    cin >> num_resource;
    rep(i, num_resource)
    {
        Pos pos;
        cin >> pos.y >> pos.x;
        result.resource_pos_in_sight.push_back(pos);
    }

    string end;
    cin >> end;
    assert(end == "END");

    return result;
}
