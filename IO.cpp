#include "IO.hpp"


Pos reverse(const Pos& pos)
{
    return Pos(99 - pos.x, 99 - pos.y);
}

bool InputResult::is_2p() const
{
    return get_my({CASTLE})[0].pos.dist(Pos(0, 0)) >= 50;
}
void InputResult::change_dir()
{
    for (auto& u : my_units)
        u.pos = reverse(u.pos);
    for (auto& u : enemy_units)
        u.pos = reverse(u.pos);
    for (auto& p : resource_pos_in_sight)
        p = reverse(p);
}

Unit input_unit()
{
    Unit unit;
    int type;
    cin >> unit.id >> unit.pos.y >> unit.pos.x >> unit.hp >> type;
    unit.type = (UnitType)type;

    return unit;
}
InputResult input()
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
//     assert(end == "END");
    if (end != "END")
        result.current_stage_no = -1;

    return result;
}

void output(const map<int, char>& order, bool change_dir)
{
    cout << order.size() << endl;
    for (auto& it : order)
    {
        int id = it.first;
        char ord = it.second;

        const char* sdir = "RULD";
        const char* t = strchr(sdir, ord);
        if (change_dir && t)
            ord = sdir[(t - sdir + 2) % 4];
        cout << id << " " << ord << endl;
    }
    cout.flush();
}

vector<Unit> select(const vector<Unit>& units, const vector<UnitType>& unit_types)
{
    vector<Unit> selected;
    for (auto& unit : units)
        if (find(all(unit_types), unit.type) != unit_types.end())
            selected.push_back(unit);
    return selected;
}
vector<Unit> InputResult::get_my(const vector<UnitType>& unit_types) const
{
    return select(my_units, unit_types);
}
vector<Unit> InputResult::get_enemy(const vector<UnitType>& unit_types) const
{
    return select(enemy_units, unit_types);
}
