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
    set<int> fixed_worker_ids;

    Unit enemy_castle;

    set<int> once_behind;

    Board<bool> visited;
    Board<bool> enemy_castle_pos_cand;

//     map<int, map<int, Unit>> unit_log;

    vector<int> group_sizes;
};

#endif
