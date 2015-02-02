#include "AI.hpp"
#include "Common.hpp"
#include "GameCommon.hpp"
#include "Pos.hpp"
#include "Unit.hpp"
#include "Board.hpp"
#include "IO.hpp"
#include "AIUtil.hpp"

#include "Random.hpp"


AI::AI()
:
    known(false),
    visited(false),
    go(false),
    is_stalker(false),
    fast_attack(false)
{
    enemy_castle.id = -1;
}

Board<int> AI::cost_table(const vector<Unit>& enemy_units) const
{
    Board<int> sum_damage(0);
    for (auto& unit : enemy_units)
    {
        for (auto& diff : RANGE_POS[2])
        {
            Pos p = unit.pos + diff;
            if (p.in_board())
                sum_damage.at(p) += unit.type == WORKER ? 100 : 400;
        }
    }

    Board<int> cost(0);
    rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
    {
        if (sum_damage.at(x, y) >= 500)
            cost.at(x, y) = 500;
    }
    return cost;
}
Board<int> AI::down_pass_cost_table(const vector<Unit>& enemy_units) const
{
    auto cost = cost_table(enemy_units);
    rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
    {
        if (x > y)
            cost.at(x, y) += 100;
        else
            cost.at(x, y) += max(50, 100 - (y - x));
    }
    return cost;
}
Board<int> AI::right_pass_cost_table(const vector<Unit>& enemy_units) const
{
    auto cost = cost_table(enemy_units);
    rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
    {
        if (x < y)
            cost.at(x, y) += 100;
        else
            cost.at(x, y) += max(50, 100 - (x - y));
    }
    return cost;
}

void merge_remove(map<int, char>& order, vector<Unit>& units, const map<int, char>& add_order)
{
    for (auto& o : add_order)
    {
        int id;
        char ord;
        tie(id, ord) = o;

        assert(!order.count(id));
        order[id] = ord;

        Unit u;
        u.id = id;
        auto it = find(all(units), u);
        assert(it != units.end());
        units.erase(it);

        it = find(all(units), u);
        assert(it == units.end());
    }
}
void merge_remove(map<int, char>& order, vector<Unit>& units, int id, char ord)
{
    assert(!order.count(id));
    order[id] = ord;

    Unit u;
    u.id = id;
    auto it = find(all(units), u);
    assert(it != units.end());
    units.erase(it);

    it = find(all(units), u);
    assert(it == units.end());
}

vector<Unit> extract(vector<Unit>& units, const vector<int>& id)
{
    vector<Unit> e;
    for (auto& unit : units)
    {
        if (find(all(id), unit.id) != id.end())
            e.push_back(unit);
    }
    for (auto& unit : e)
    {
        assert(find(all(units), unit) != units.end());
        units.erase(find(all(units), unit));
    }
    return e;
}
void remove(vector<Unit>& units, const vector<Unit>& removed)
{
    for (auto& rem : removed)
    {
        auto it = find(all(units), rem);
        if (it != units.end())
            units.erase(it);
    }
}

void AI::move_for_resource(map<int, char>& order, vector<Unit>& remain_workers)
{
    map<Pos, vector<Unit>> pos_to_units;
    for (auto& u : remain_workers)
        pos_to_units[u.pos].push_back(u);

    vector<Pos> rsrc_pos;
    for (auto& pos : resource_pos)
    {
        if (pos.dist(Pos(0, 0)) > 99)
            continue;

        auto units = pos_to_units[pos];
        while (units.size() > MAX_RESOURCE_GAIN)
            units.pop_back();

        const int need = MAX_RESOURCE_GAIN - (int)units.size();
        if (need > 0)
        {
            rep(_, need)
                rsrc_pos.push_back(pos);
        }

        for (auto& u : units)
            remain_workers.erase(find(all(remain_workers), u));
    }

    const int n = max(rsrc_pos.size(), remain_workers.size());
    const int inf = 114514;
    vector<vector<int>> cost(n, vector<int>(n, inf));
    rep(worker_i, n) rep(rsrc_i, rsrc_pos.size())
    {
        if (worker_i >= remain_workers.size() || rsrc_i >= rsrc_pos.size())
            cost[worker_i][rsrc_i] = inf;
        else
            cost[worker_i][rsrc_i] = remain_workers[worker_i].pos.dist(rsrc_pos[rsrc_i]);
    }
    auto matching = min_assignment(cost);

    map<int, char> order_to_resource;
    rep(worker_i, remain_workers.size())
    {
        const Unit& worker = remain_workers[worker_i];
        const int rsrc_i = matching[worker_i];
        if (rsrc_i < rsrc_pos.size())
        {
            order_to_resource[worker.id] = to_order(decide_dir(worker.pos, rsrc_pos[rsrc_i]));
        }
    }

    merge_remove(order, remain_workers, order_to_resource);
}

void AI::move_scouters(map<int, char>& order, vector<Unit>& remain_workers, const vector<Unit>& enemy_units, const Pos& my_castle_pos)
{
    vector<Unit> workers_in_enemy_area;
    {
        const int NUM_SCOUTERS = 8;
        auto down_scouters = extract(remain_workers, down_scouter_ids);
        while (down_scouter_ids.size() < NUM_SCOUTERS / 2 && !remain_workers.empty())
        {
            down_scouter_ids.push_back(remain_workers.back().id);
            remain_workers.pop_back();
        }
        auto right_scouters = extract(remain_workers, right_scouter_ids);
        while (right_scouter_ids.size() < NUM_SCOUTERS / 2 && !remain_workers.empty())
        {
            right_scouter_ids.push_back(remain_workers.back().id);
            remain_workers.pop_back();
        }

        rep(i, down_scouters.size())
        {
            const Unit& u = down_scouters[i];

            vector<int> move_cost(4);
            move_cost[DOWN] = 10;
            move_cost[RIGHT] = 12;
            move_cost[LEFT] = move_cost[UP] = 50;

            const int relay_x = 4 + (max(my_castle_pos.x - 4, 0) / 9) * 9 + 9 * i;
            const Pos goal(90, 99 - 9 * i);

            if (u.pos == goal)
                once_goal_scouter_ids.insert(u.id);

            if (u.pos.x < relay_x)
                order[u.id] = to_order(RIGHT);
            else if (!once_goal_scouter_ids.count(u.id))
            {
                DijkstraResult res = dijkstra(u.pos, down_pass_cost_table(enemy_units), move_cost);
                order[u.id] = to_order(res.find_dir(goal));
            }
            else
                workers_in_enemy_area.push_back(u);
        }

        rep(i, right_scouters.size())
        {
            const Unit& u = right_scouters[i];

            vector<int> move_cost(4);
            move_cost[DOWN] = 12;
            move_cost[RIGHT] = 10;
            move_cost[LEFT] = move_cost[UP] = 50;

            const int relay_y = 4 + (max(my_castle_pos.y - 4, 0) / 9) * 9 + 9 * i;
            const Pos goal(99 - 9 * i, 90);

            if (u.pos == goal)
                once_goal_scouter_ids.insert(u.id);

            if (u.pos.y < relay_y)
                order[u.id] = to_order(DOWN);
            else if (!once_goal_scouter_ids.count(u.id))
            {
                DijkstraResult res = dijkstra(u.pos, right_pass_cost_table(enemy_units), move_cost);
                order[u.id] = to_order(res.find_dir(goal));
            }
            else
                workers_in_enemy_area.push_back(u);
        }
    }

    Board<bool> unknown_enemy_area;
    rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
        unknown_enemy_area.at(x, y) = Pos(x, y).dist(Pos(99, 99)) <= 40 && !known.at(x, y);
    merge_remove(order, workers_in_enemy_area, search_moves(workers_in_enemy_area, unknown_enemy_area));
}

map<int, char> AI::solve(const InputResult& input)
{
    const vector<Unit> my_units = input.my_units;
    const vector<Unit> enemy_units = input.enemy_units;
    const Unit my_castle = input.get_my({CASTLE})[0];
    const vector<Unit> my_workers = input.get_my({WORKER});
    const vector<Unit> my_warriors = input.get_my({KNIGHT, FIGHTER, ASSASSIN});
    const vector<Unit> my_bases = input.get_my({BASE});
    const vector<Unit> my_villages = input.get_my({VILLAGE});
    const vector<Unit> enemy_warriors = input.get_enemy({KNIGHT, FIGHTER, ASSASSIN});

    for (auto& unit : input.my_units)
    {
        visited.at(unit.pos) = true;

        for (auto& diff : RANGE_POS[unit.sight_range()])
        {
            Pos p = unit.pos + diff;
            if (p.in_board())
                known.at(p) = true;
        }
    }

    for (auto& pos : input.resource_pos_in_sight)
        if (pos.dist(Pos(0, 0)) <= 105)
            resource_pos.push_back(pos);
    uniq(resource_pos);


    {
        if (!input.get_enemy({CASTLE}).empty())
            enemy_castle = input.get_enemy({CASTLE})[0];

        auto ene_vs = input.get_enemy({VILLAGE});
        enemy_villages.insert(enemy_villages.end(), all(ene_vs));
        uniq(ene_vs);
    }

    {
        if (input.current_turn == 0)
            num_workers_created_for_map[my_castle.pos] = 5;

        for (auto& worker : my_workers)
        {
            if (num_workers_created_for_map.count(worker.pos) && num_workers_created_for_map[worker.pos] > 0)
            {
                worker_ids_for_map.push_back(worker.id);
                --num_workers_created_for_map[worker.pos];
            }
        }
        num_workers_created_for_map.clear();
    }

    {
        set<pint> lost;
        for (auto& it : stalker_log)
            lost.insert(it.first);
        for (auto& w : my_workers)
        {
            for (auto& e : enemy_warriors)
            {
                pint key(w.id, e.id);
                if (w.pos.dist(e.pos) <= e.sight_range())
                {
                    stalker_log[key].push_back(make_pair(w, e));
                    lost.erase(key);
                }
            }
        }
        for (auto& key : lost)
            stalker_log.erase(key);


        if (input.current_turn <= 150)
        {
            set<int> stalked_ids;
            for (auto& it : stalker_log)
            {
                auto& log = it.second;
                assert(!log.empty());

                vector<int> dir_freq(4);
                rep(i, sz(log) - 1)
                    if (log[i].first.pos != log[i + 1].first.pos)
                        ++dir_freq[decide_dir(log[i].first.pos, log[i + 1].first.pos)];

                if (log.back().second.pos.dist(Pos(99, 99)) <= 70 &&
                    max(dir_freq[RIGHT], dir_freq[DOWN]) >= 5)
                {
                    stalked_ids.insert(log[0].first.id);
                }
            }
            if (stalked_ids.size() >= 3)
            {
                is_stalker = true;
            }
        }
    }

    if (is_stalker)
        fast_attack = false;

    int remain_resources = input.resources;
    auto remain_workers = input.get_my({WORKER});
    auto remain_warriors = input.get_my({KNIGHT, FIGHTER, ASSASSIN});

    map<int, char> order;

    if (input.current_turn < 200)
    {
        bool found = false;
        for (auto& u : enemy_warriors)
            found |= u.pos.dist(my_castle.pos) <= 10;

        if (found)
        {
            go = true;

            int num_on_villages = 0;
            for (auto& village : input.get_my({VILLAGE}))
                if (village.pos == my_castle.pos)
                    ++num_on_villages;

            vector<Unit> on_workers;
            for (auto& worker : remain_workers)
            {
                if (worker.pos == my_castle.pos)
                    on_workers.push_back(worker);
            }


            if (on_workers.empty())
            {
                if (!order.count(my_castle.id) && remain_resources >= CREATE_COST[WORKER])
                {
                    order[my_castle.id] = CREATE_ORDER[WORKER];
                    remain_resources -= CREATE_COST[WORKER];
                }
            }
            else
            {
                for (auto& on_worker : on_workers)
                {
                    if (num_on_villages <= 2 && remain_resources >= CREATE_COST[VILLAGE])
                    {
                        merge_remove(order, remain_workers, on_worker.id, CREATE_ORDER[VILLAGE]);
                        remain_resources -= CREATE_COST[VILLAGE];

                        ++num_on_villages;
                    }
                    else
                    {
                        auto it = find(all(remain_workers), on_worker);
                        assert(it != remain_workers.end());
                        remain_workers.erase(it);
                    }
                }
            }
        }
    }

    {
        map<Pos, int> num_workers;
        for (auto& worker : my_workers)
            ++num_workers[worker.pos];

        for (auto& pos : resource_pos)
        {
            if (num_workers[pos] == 0)
            {
                const int inf = 1919810;
                int best_dist = inf;
                Unit best_worker;
                for (auto& worker : my_workers)
                {
                    if (find(all(right_scouter_ids), worker.id) == right_scouter_ids.end() &&
                        find(all(down_scouter_ids), worker.id) == down_scouter_ids.end())
                    {
                        int d = pos.dist(worker.pos);
                        if (d <= SIGHT_RANGE[WORKER] && d < best_dist)
                        {
                            best_dist = d;
                            best_worker = worker;
                        }
                    }
                }
                if (best_dist != inf)
                {
                    auto it = find(all(worker_ids_for_map), best_worker.id);
                    if (it != worker_ids_for_map.end())
                        worker_ids_for_map.erase(it);
                }
            }
        }
    }


    {
        vector<Unit> stay_workers;
        map<Pos, int> num_workers_for_resource;
        {
            vector<Unit> workers_for_resource;
            for (auto& w : remain_workers)
                if (find(all(worker_ids_for_map), w.id) == worker_ids_for_map.end())
                    workers_for_resource.push_back(w);

            map<Pos, vector<Unit>> pos_to_units;
            for (auto& u : workers_for_resource)
                pos_to_units[u.pos].push_back(u);

            vector<Pos> rsrc_pos;
            for (auto& pos : resource_pos)
            {
                auto units = pos_to_units[pos];
                while (units.size() > MAX_RESOURCE_GAIN)
                    units.pop_back();
                num_workers_for_resource[pos] += units.size();

                const int need = MAX_RESOURCE_GAIN - (int)units.size();
                if (need > 0)
                {
                    rep(_, need)
                        rsrc_pos.push_back(pos);
                }

                for (auto& u : units)
                {
                    workers_for_resource.erase(find(all(workers_for_resource), u));
                    stay_workers.push_back(u);
                }
            }

            vector<vector<int>> cost(workers_for_resource.size(), vector<int>(rsrc_pos.size()));
            rep(i, workers_for_resource.size()) rep(j, rsrc_pos.size())
                cost[i][j] = workers_for_resource[i].pos.dist(rsrc_pos[j]);
            vector<int> matching = min_assignment(cost);
            rep(i, workers_for_resource.size())
            {
                auto& w = workers_for_resource[i];
                int j = matching[i];
                if (j != -1)
                {
                    Pos pos = rsrc_pos[j];
                    merge_remove(order, remain_workers, w.id, to_order(decide_dir(w.pos, pos)));

                    ++num_workers_for_resource[pos];
                    assert(num_workers_for_resource[pos] <= MAX_RESOURCE_GAIN);
                }
            }
        }


        if (!go)
        {
            if (input.current_turn <= 130)
            {
                vector<Pos> added_villages_pos;
                for (Pos& pos : resource_pos)
                {
                    if (num_workers_for_resource[pos] < MAX_RESOURCE_GAIN)
                    {
                        int min_dist = my_castle.pos.dist(pos);
                        for (auto& v : my_villages)
                            upmin(min_dist, v.pos.dist(pos));
                        for (auto& p : added_villages_pos)
                            upmin(min_dist, p.dist(pos));

                        if (min_dist > 20)
                        {
                            const int inf = 1919810;
                            int best_dist = inf;
                            Unit best_worker;
                            for (auto& worker : remain_workers)
                            {
                                {
                                    int d = pos.dist(worker.pos);
                                    if (d <= SIGHT_RANGE[WORKER] && d < best_dist)
                                    {
                                        best_dist = d;
                                        best_worker = worker;
                                    }
                                }
                            }
                            if (best_dist != inf)
                            {
                                if (remain_resources >= CREATE_COST[VILLAGE])
                                {
                                    merge_remove(order, remain_workers, best_worker.id, CREATE_ORDER[VILLAGE]);
                                    remain_resources -= CREATE_COST[VILLAGE];
                                    added_villages_pos.push_back(best_worker.pos);
                                }
                                else
                                {
                                    // 貯めるため
                                    remain_resources -= CREATE_COST[VILLAGE];
                                }
                            }
                        }
                    }
                }
            }

            if (input.current_turn <= 140)
            {
                for (Pos& pos : resource_pos)
                {
                    if (remain_resources >= CREATE_COST[WORKER] && num_workers_for_resource[pos] < MAX_RESOURCE_GAIN)
                    {
                        const int inf = 1919810;
                        int best_dist = inf;
                        Unit best_creater;
                        best_creater.id = -1;
                        if (!order.count(my_castle.id) && pos.dist(my_castle.pos) <= 20)
                        {
                            best_creater = my_castle;
                            best_dist = pos.dist(my_castle.pos);
                        }

                        for (auto& v : my_villages)
                        {
                            int d = pos.dist(v.pos);
                            if (!order.count(v.id) && d <= 20 && d < best_dist)
                            {
                                best_dist = d;
                                best_creater = v;
                            }
                        }

                        if (best_creater.id != -1)
                        {
                            order[best_creater.id] = CREATE_ORDER[WORKER];
                            remain_resources -= CREATE_COST[WORKER];
                        }
                    }
                }
            }
        }

        for (auto& w : stay_workers)
        {
            auto it = find(all(remain_workers), w);
            if (it != remain_workers.end())
                remain_workers.erase(it);
        }
    }


    if (!order.count(my_castle.id) && my_workers.size() < 12 && input.current_turn < 150 && enemy_castle.id == -1)
    {

        if (remain_resources >= CREATE_COST[WORKER])
        {
            order[my_castle.id] = CREATE_ORDER[WORKER];
            remain_resources -= CREATE_COST[WORKER];

            ++num_workers_created_for_map[my_castle.pos];
        }
    }




    {
        if (enemy_castle.id == -1)
        {
            bool set_village = false;
            if (remain_resources >= CREATE_COST[VILLAGE])
            {
                map<int, int> predict_damage = simulate_damage(enemy_units, my_units);
                for (auto& worker : remain_workers)
                {
                    if (worker.pos.dist(Pos(99, 99)) <= 40 + 10 && prev_unit.count(worker.id) && worker.hp < prev_unit[worker.id].hp - predict_damage[worker.id])
                    {
                        merge_remove(order, remain_workers, worker.id, CREATE_ORDER[VILLAGE]);
                        remain_resources -= CREATE_COST[VILLAGE];
                        set_village = true;
                        break;
                    }
                }
            }

            if (!set_village && my_bases.empty() && remain_resources >= CREATE_COST[BASE])
            {
                vector<Unit> scouters;
                for (auto& w : remain_workers)
                {
                    if (find(all(right_scouter_ids), w.id) != right_scouter_ids.end() ||
                        find(all(down_scouter_ids), w.id) != down_scouter_ids.end())
                        scouters.push_back(w);
                }
                int stalkers = 0;
                for (auto& w : remain_workers)
                {
                    for (auto e : enemy_warriors)
                    {
                        if (e.pos.dist(w.pos) <= e.sight_range())
                        {
                            ++stalkers;
                            break;
                        }
                    }
                }

                if (is_stalker || ((scouters.size() == 1 || stalkers - 1 >= (int)scouters.size()) && my_bases.size() < 2 && scouters.size() <= 3 && remain_resources >= CREATE_COST[BASE]))
                {
                    int best_dist = 810;
                    Unit best_worker;
                    for (auto& worker : remain_workers)
                    {
                        int d = worker.pos.dist(Pos(99, 99));
                        if (d <= 40 && d < best_dist)
                        {
                            best_dist = d;
                            best_worker = worker;
                        }
                    }
                    if (best_dist != 810)
                    {
                        merge_remove(order, remain_workers, best_worker.id, CREATE_ORDER[BASE]);
                        remain_resources -= CREATE_COST[BASE];
                    }
                }
            }

            move_scouters(order, remain_workers, enemy_units, my_castle.pos);

            if (!my_warriors.empty())
            {
                for (auto& w : my_warriors)
                {
                    if (warrior_scouter_ids.size() < 5 && find(all(warrior_scouter_ids), w.id) == warrior_scouter_ids.end())
                        warrior_scouter_ids.push_back(w.id);
                }

                auto warrior_scouters = extract(remain_warriors, warrior_scouter_ids);
                Board<bool> unknown_enemy_area;
                rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
                    unknown_enemy_area.at(x, y) = Pos(x, y).dist(Pos(99, 99)) <= 40 && !known.at(x, y);
                merge_remove(order, warrior_scouters, search_moves(warrior_scouters, unknown_enemy_area));
            }
        }
        else
        {
            const bool in_sight = input.get_enemy({CASTLE}).size() > 0;
            vector<Unit> on_castle;
            vector<Unit> around_castle;
            for (auto& unit : enemy_units)
            {
                if (unit.type != CASTLE && unit.type != WORKER)
                {
                    if (unit.pos == enemy_castle.pos)
                        on_castle.push_back(unit);
                    if (unit.pos.dist(enemy_castle.pos) <= 2)
                        around_castle.push_back(unit);
                }
            }


            Board<bool> base_cand(false);
            bool found = false;
            for (int k = 5; k >= 0 && !found; --k)
            {
                rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
                {
                    int ex = enemy_castle.pos.x;
                    int ey = enemy_castle.pos.y;
                    int d = enemy_castle.pos.dist(Pos(x, y));
                    int low, high;
                    low = 11;
                    high = 25;
                    if (low <= d && d <= high &&
                            x >= ex + k && y >= ey + k)
                    {
                        base_cand.at(x, y) = true;
                        found = true;
                    }
                }
            }
            if (!found)
            {
                for (int k = 10; k >= 0 && !found; --k)
                {
                    rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
                    {
                        int ex = enemy_castle.pos.x;
                        int ey = enemy_castle.pos.y;
                        int d = enemy_castle.pos.dist(Pos(x, y));
                        int low, high;
                        low = 11;
                        high = 25;
                        if (low <= d && d <= high &&
                                (x >= ex + k|| y >= ey + k))
                        {
                            base_cand.at(x, y) = true;
                            found = true;
                        }
                    }
                }
            }

            for (auto& u : enemy_villages)
            {
                for (auto& diff : RANGE_POS[u.sight_range()])
                {
                    Pos p = u.pos + diff;
                    if (p.in_board())
                        base_cand.at(p) = false;
                }
            }
            for (auto& u : enemy_units)
            {
                for (auto& diff : RANGE_POS[u.sight_range()])
                {
                    Pos p = u.pos + diff;
                    if (p.in_board())
                        base_cand.at(p) = false;
                }
            }

            if (in_sight && around_castle.size() == 0)
            {
                fast_attack = true;
                rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
                {
                    int ex = enemy_castle.pos.x;
                    int ey = enemy_castle.pos.y;
                    int d = enemy_castle.pos.dist(Pos(x, y));
                    if (d < 13)
                    {
                        base_cand.at(x, y) = true;
                    }
                }
            }

            int unknown = 0;
            bool any_cand = false;
            rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
            {
                any_cand |= base_cand.at(x, y);
                if (Pos(x, y).dist(Pos(99, 99)) <= 40 && !known.at(x, y))
                    ++unknown;
            }
            if (!any_cand && unknown <= 20)
            {
                rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
                {
                    if (Pos(x, y).dist(Pos(99, 99)) <= 40)
                        base_cand.at(x, y) = true;
                }
                for (auto& u : enemy_villages)
                {
                    for (auto& diff : RANGE_POS[u.sight_range()])
                    {
                        Pos p = u.pos + diff;
                        if (p.in_board())
                            base_cand.at(p) = false;
                    }
                }
                for (auto& u : enemy_units)
                {
                    for (auto& diff : RANGE_POS[u.sight_range()])
                    {
                        Pos p = u.pos + diff;
                        if (p.in_board())
                            base_cand.at(p) = false;
                    }
                }
            }

            if (my_bases.size() < 2 && remain_resources >= CREATE_COST[BASE])
            {
                vector<Unit> scouters;
                for (auto& w : remain_workers)
                {
                    if (find(all(right_scouter_ids), w.id) != right_scouter_ids.end() ||
                        find(all(down_scouter_ids), w.id) != down_scouter_ids.end())
                        scouters.push_back(w);
                }
                bool all_stalked = true;
                for (auto& w : scouters)
                {
                    bool stalked = false;
                    for (auto& e : enemy_warriors)
                        if (w.pos.dist(e.pos) <= e.sight_range())
                            stalked = true;
                    all_stalked &= stalked;
                }

                int best_score = 1919810;
                Unit best_worker;
                for (auto& worker : scouters)
                {
                    int d = worker.pos.dist(enemy_castle.pos);
                    int score = d;

                    if (!fast_attack)
                    {
                        if (worker.pos.x < enemy_castle.pos.x)
                            score += 1000;
                        if (worker.pos.y < enemy_castle.pos.y)
                            score += 1000;
                    }


                    if (((all_stalked && scouters.size() <= 2 && enemy_castle.pos.dist(worker.pos) > 11) || base_cand.at(worker.pos) || is_stalker) && score < best_score)
                    {
                        best_score = score;
                        best_worker = worker;
                    }
                }
                if (best_score != 1919810)
                {
                    merge_remove(order, remain_workers, best_worker.id, CREATE_ORDER[BASE]);
                    remain_resources -= CREATE_COST[BASE];
                }
            }

            if (fast_attack)
            {
                for (auto& worker : remain_workers)
                {
                    if (find(all(right_scouter_ids), worker.id) != right_scouter_ids.end() ||
                        find(all(down_scouter_ids), worker.id) != down_scouter_ids.end())
                    {
                        if (!order.count(worker.id) && enemy_castle.pos.dist(worker.pos) > 2)
                        {
                            merge_remove(order, remain_workers, worker.id, to_order(decide_dir(worker.pos, enemy_castle.pos)));
                        }
                    }
                }
            }

            if (my_bases.empty())
            {
                map<int, char> base_pos_order;

                vector<Unit> scouters;
                for (auto& w : remain_workers)
                {
                    if (find(all(right_scouter_ids), w.id) != right_scouter_ids.end() ||
                            find(all(down_scouter_ids), w.id) != down_scouter_ids.end())
                        scouters.push_back(w);
                }

                Board<int> cost(0);
                for (auto& diff : RANGE_POS[enemy_castle.sight_range()])
                {
                    Pos p = enemy_castle.pos + diff;
                    if (p.in_board())
                        cost.at(p) += 100000;
                }
                for (auto& u : enemy_villages)
                {
                    for (auto& diff : RANGE_POS[u.sight_range()])
                    {
                        Pos p = u.pos + diff;
                        if (p.in_board())
                            cost.at(p) += 100000;
                    }
                }
                for (auto& u : enemy_units)
                {
                    for (auto& diff : RANGE_POS[u.sight_range()])
                    {
                        Pos p = u.pos + diff;
                        if (p.in_board())
                            cost.at(p) += 100000;
                    }
                }
                for (auto& diff : RANGE_POS[13])
                {
                    Pos p = enemy_castle.pos + diff;
                    if (p.in_board())
                        cost.at(p) += 5;
                }
                for (auto& worker : remain_workers)
                {
                    if (!base_cand.at(worker.pos) &&
                            (find(all(right_scouter_ids), worker.id) != right_scouter_ids.end() ||
                             find(all(down_scouter_ids), worker.id) != down_scouter_ids.end()))
                    {
                        DijkstraResult res = dijkstra(worker.pos, cost, vector<int>(4, 10));

                        int best_cost = DIJKSTRA_INF;
                        Pos best_goal;
                        rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
                        {
                            if (base_cand.at(x, y))
                            {
                                int c = res.cost.at(x, y);
                                if (c < best_cost)
                                {
                                    best_cost = c;
                                    best_goal = Pos(x, y);
                                }
                            }
                        }
                        if (best_cost != DIJKSTRA_INF && worker.pos != best_goal)
                        {
                            base_pos_order[worker.id] = to_order(res.find_dir(best_goal));
                        }
                    }
                }

                merge_remove(order, remain_workers, base_pos_order);
            }
            else
            {
                for (auto& worker : remain_workers)
                {
                    if (!order.count(worker.id) &&
                        !base_cand.at(worker.pos) &&
                        (find(all(right_scouter_ids), worker.id) != right_scouter_ids.end() ||
                            find(all(down_scouter_ids), worker.id) != down_scouter_ids.end()))
                    {
                        Unit stalker;
                        stalker.id = -1;
                        for (auto& w : enemy_warriors)
                            if (w.pos.dist(worker.pos) <= w.sight_range())
                                stalker = w;
                        if (stalker.id != -1 && worker.pos != stalker.pos)
                        {
                            merge_remove(order, remain_workers, worker.id, to_order(decide_dir(worker.pos, stalker.pos)));
                        }
                    }
                }
            }
        }

        {
            Board<int> pos_cost(-1);
            rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
            {
                if (!visited.at(x, y) && Pos(x, y).dist(Pos(0, 0)) <= 99)
                    pos_cost.at(x, y) = min(x, y);
            }
            merge_remove(order, remain_workers, search_moves_by_min_assignment(remain_workers, pos_cost));
        }
    }


    {
        const bool in_sight = input.get_enemy({CASTLE}).size() > 0;
        vector<Unit> on_castle;
        vector<Unit> around_castle;
        for (auto& unit : enemy_units)
        {
            if (unit.type != CASTLE && unit.type != WORKER)
            {
                if (unit.pos == enemy_castle.pos)
                    on_castle.push_back(unit);
                if (unit.pos.dist(enemy_castle.pos) <= 2)
                    around_castle.push_back(unit);
            }
        }

        const UnitType warrior_types[] = { KNIGHT, FIGHTER, ASSASSIN };
        vector<double> ratio;
        if (in_sight && around_castle.size() == 0 || go || fast_attack)
            ratio = {5, 1, 1};
        else
            ratio = {5, 1, 5};
        for (auto& base : my_bases)
        {
            vector<bool> to_make(3, false);
            if (!my_warriors.empty())
            {
                rep(i, 3)
                {
                    double current_ratio = (double)input.get_my({warrior_types[i]}).size() / my_warriors.size();
                    to_make[i] = current_ratio <= ratio[i] / accumulate(all(ratio), 0.0);
                }
            }
            else
                to_make = vector<bool>(3, true);

            rep(i, 3)
            {
                UnitType type = warrior_types[i];
                if (to_make[i] && remain_resources >= CREATE_COST[type])
                {
                    order[base.id] = CREATE_ORDER[type];
                    remain_resources -= CREATE_COST[type];
                    break;
                }
            }
        }
    }


    {
        if (enemy_castle.id != -1)
        {
            const bool in_sight = input.get_enemy({CASTLE}).size() > 0;
            vector<Unit> on_castle;
            vector<Unit> around_castle;
            for (auto& unit : enemy_units)
            {
                if (unit.type != CASTLE && unit.type != WORKER)
                {
                    if (unit.pos == enemy_castle.pos)
                        on_castle.push_back(unit);
                    if (unit.pos.dist(enemy_castle.pos) <= 2)
                        around_castle.push_back(unit);
                }
            }

            map<int, char> warrior_order;

            map<Pos, vector<Unit>> pos_to_warriors;
            for (auto warrior : remain_warriors)
                pos_to_warriors[warrior.pos].push_back(warrior);

            set<Pos> base_pos;
            for (auto& base : my_bases)
                base_pos.insert(base.pos);

            int around_my_castle = 0;
            for (auto& w : enemy_warriors)
                if (my_castle.pos.dist(w.pos) <= 10)
                    ++around_my_castle;
            if (around_my_castle >= 1)
                go = true;

            for (auto& it : pos_to_warriors)
            {
                Pos pos;
                vector<Unit> warriors;
                tie(pos, warriors) = it;

                // 10: 80 or 60(微妙にこっちのがいい。誤差レベル)
                // 30: 80?? or 90(90のが1勝おおかった)
                const int go_line = (in_sight && on_castle.size() == 0 ? 1 : 110);
                if (input.current_turn >= 270)
                    go = true;
                if (my_warriors.size() >= go_line)
                    go = true;
                if (is_stalker && my_warriors.size() >= 60)
                    go = true;
                if (pos.dist(enemy_castle.pos) > 2 && (!base_pos.count(pos) || my_warriors.size() >= go_line || go))
                {
                    for (auto& warrior : warriors)
                        warrior_order[warrior.id] = to_order(decide_dir(warrior.pos, enemy_castle.pos));
                }
            }

            merge_remove(order, remain_warriors, warrior_order);
        }
    }

    // grun only
    if (!my_bases.empty() && !go)
    {
        vector<Unit> stalkers;
        for (auto& u : enemy_units)
        {
            if (u.type != CASTLE && u.type != BASE && u.pos.dist(my_bases[0].pos) <= u.sight_range())
                stalkers.push_back(u);
        }

        set<int> used_ids;
        for (auto& it : stalker_attacker_ids)
        {
            for (auto& id : it.second)
                used_ids.insert(id);
        }

        for (auto& stalker : stalkers)
        {
            vector<Unit> attackers;
            auto& attacker_ids = stalker_attacker_ids[stalker.id];
            for (auto& w : my_warriors)
            {
                if (attacker_ids.size() < 2 && !used_ids.count(w.id))
                {
                    used_ids.insert(w.id);
                    attacker_ids.insert(w.id);
                }

                if (attacker_ids.count(w.id))
                    attackers.push_back(w);
            }

            for (auto& w : attackers)
            {
                if (w.pos != stalker.pos)
                    order[w.id] = to_order(decide_dir(w.pos, stalker.pos));
            }
        }

        for (auto& w : my_warriors)
        {
            if (used_ids.count(w.id) && !order.count(w.id))
            {
                if (w.pos != my_bases[0].pos)
                    order[w.id] = to_order(rev_dir(decide_dir(w.pos, my_bases[0].pos)));
            }
        }
    }


    prev_unit.clear();
    for (auto& unit : my_units)
        prev_unit[unit.id] = unit;

    return order;
}

