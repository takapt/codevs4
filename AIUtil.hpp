#include "Common.hpp"
#include "GameCommon.hpp"
#include "Unit.hpp"
#include "Board.hpp"


typedef int value_type;
vector<int> min_assignment(const vector<vector<value_type>> &c);

vector<vector<Pos>> clustering(const vector<Pos>& pos, const int num_cluster);

vector<Pos> list_near_pos(const Board<bool>& start, const Board<bool>& mark, const int max_dist);

Dir decide_dir(const Pos& cur, const Pos& to);

map<int, char> search_moves(const vector<Unit>& units, const Board<bool>& mark, const int max_dist = 200, const Pos remain_target = Pos(99, 99));


map<int, char> search_moves_by_min_assignment(const vector<Unit>& units, const Board<int>& pos_cost);


const int DIJKSTRA_INF = ten(7);
struct DijkstraResult
{
    Pos start;
    Board<int> cost;
    Board<Dir> prev_dir;

    Dir find_dir(const Pos& goal) const;
};
DijkstraResult dijkstra(const Pos& start, const Board<int>& cost, const vector<int>& move_cost);

vector<pair<Unit, Pos>> match_unit_goal(const vector<Unit>& units, const vector<Pos>& goals);

map<int, char> search_moves_by_dijkstra(const vector<pair<Unit, Pos>>& unit_goal, Board<int> cost);


