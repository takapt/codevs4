#ifndef INCLUDE_AI
#define INCLUDE_AI

#include "Common.hpp"
#include "IO.hpp"
#include "Board.hpp"

class AI
{
public:
    AI();

    map<int, char> solve(const InputResult& input);

private:
    Board<bool> known;
    vector<Pos> resource_pos;
    vector<Pos> resource_pos_in_enemy_area;

    Unit enemy_castle;
    vector<Unit> enemy_villages;

    set<int> once_behind;

    Board<bool> visited;

    Board<int> cost_table(const vector<Unit>& enemy_units) const;
    Board<int> down_pass_cost_table(const vector<Unit>& enemy_units) const;
    Board<int> right_pass_cost_table(const vector<Unit>& enemy_units) const;
    vector<int> down_scouter_ids;
    vector<int> right_scouter_ids;
    set<int> once_goal_scouter_ids;
    void move_scouters(map<int, char>& order, vector<Unit>& remain_workers, const vector<Unit>& enemy_units, const Pos& my_castle_pos);

    void move_for_resource(map<int, char>& order, vector<Unit>& remain_workers);

    bool go;

    map<int, Unit> prev_unit;

    bool is_lila;
    bool is_schwarz;
    bool fast_attack;

    vector<int> warrior_scouter_ids;

    map<Pos, int> num_workers_created_for_map;
    vector<int> worker_ids_for_map;

    map<int, set<int>> stalker_attacker_ids;

    map<pint, vector<pair<Unit, Unit>>> stalker_log; // (my id, enemy id)
};

#endif
