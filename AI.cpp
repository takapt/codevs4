#include "AI.hpp"
#include "Common.hpp"
#include "GameCommon.hpp"
#include "Pos.hpp"
#include "Unit.hpp"
#include "Board.hpp"
#include "IO.hpp"

#include "Random.hpp"



vector<int> hungarian(vector<vector<int>> a)
{
    const int inf = 114514;
    int n = a.size(), p, q;
    rep(i, n) rep(j, n)
        a[i][j] *= -1;

    vector<int> fx(n, inf), fy(n, 0);
    vector<int> x(n, -1), y(n, -1);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            fx[i] = max(fx[i], a[i][j]);
    for (int i = 0; i < n; ) {
        vector<int> t(n, -1), s(n+1, i);
        for (p = q = 0; p <= q && x[i] < 0; ++p)
            for (int k = s[p], j = 0; j < n && x[i] < 0; ++j)
                if (fx[k] + fy[j] == a[k][j] && t[j] < 0) {
                    s[++q] = y[j], t[j] = k;
                    if (s[q] < 0)
                        for (p = j; p >= 0; j = p)
                            y[j] = k = t[j], p = x[k], x[k] = j;
                }
        if (x[i] < 0) {
            int d = inf;
            for (int k = 0; k <= q; ++k)
                for (int j = 0; j < n; ++j)
                    if (t[j] < 0) d = min(d, fx[s[k]] + fy[j] - a[s[k]][j]);
            for (int j = 0; j < n; ++j) fy[j] += (t[j] < 0 ? 0 : d);
            for (int k = 0; k <= q; ++k) fx[s[k]] -= d;
        } else ++i;
    }
//     int ret = 0;
//     for (int i = 0; i < n; ++i) ret += a[i][x[i]];
//     return ret;

    return x;
}
vector<vector<Pos>> clustering(const vector<Pos>& pos, const int num_cluster)
{
    if (num_cluster == 0)
        return {};

    assert(pos.size() >= num_cluster);

    vector<vector<Pos>> clusters(num_cluster);
    rep(i, pos.size())
        clusters[rand() % num_cluster].push_back(pos[i]);

    rep(_, 100)
    {
        vector<Pos> center(num_cluster);
        rep(cluster_i, num_cluster)
        {
            auto& cluster = clusters[cluster_i];
            if (cluster.empty())
            {
                center[cluster_i] = Pos(rand() % BOARD_SIZE, rand() % BOARD_SIZE);
                continue;
            }

            center[cluster_i].x = center[cluster_i].y = 0;
            for (auto& p : cluster)
                center[cluster_i] += p;
            center[cluster_i] /= cluster.size();
        }

        rep(cluster_i, num_cluster)
            clusters[cluster_i].clear();

        for (auto& p : pos)
        {
            const int inf = 114514;
            int min_dist = inf;
            int best_cluster_i = -1;
            rep(cluster_i, num_cluster)
            {
                int d = p.dist(center[cluster_i]);
                if (d < min_dist)
                {
                    min_dist = d;
                    best_cluster_i = cluster_i;
                }
            }
            assert(best_cluster_i != -1);
            clusters[best_cluster_i].push_back(p);
        }
    }

    return clusters;
}

vector<Pos> list_near_pos(const Board<bool>& start, const Board<bool>& mark, const int max_dist)
{
    vector<Pos> near_pos;

    const int inf = 114514;
    Board<bool> dp(inf);
    queue<Pos> q;
    rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
    {
        if (start.at(x, y))
        {
            q.push(Pos(x, y));
            dp.at(x, y) = 0;
        }
    }
    while (!q.empty())
    {
        Pos p = q.front();
        q.pop();

        near_pos.push_back(p);

        for (auto& diff : NEXT_POS)
        {
            Pos np = p + diff;
            if (np.in_board() && mark.at(np) && dp.at(p) + 1 < dp.at(np))
            {
                q.push(np);
                dp.at(np) = dp.at(p) + 1;
            }
        }
    }
    return near_pos;
}


Dir decide_dir(const Pos& cur, const Pos& to)
{
    assert(cur != to);

    Dir dir;
    if (abs(to.x - cur.x) > abs(to.y - cur.y))
        dir = to.x < cur.x ? LEFT : RIGHT;
    else
        dir = to.y < cur.y ? UP : DOWN;
    return dir;
}

map<int, char> search_moves(const vector<Unit>& units, const Board<bool>& mark, const int max_dist = 200, const Pos remain_target = Pos(99, 99))
{
//     const vector<Pos> near_mark_pos = list_near_pos(start, mark, max_dist);
    vector<Pos> near_mark_pos;
    rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
        if (mark.at(x, y))
            near_mark_pos.push_back(Pos(x, y));

    vector<vector<Pos>> clusters;
    const Pos dummy_pos(1919, 810);
    if (near_mark_pos.size() >= units.size())
        clusters = clustering(near_mark_pos, units.size());
    else
    {
        for (auto& p : near_mark_pos)
            clusters.push_back({p});

        // dummy cluster
        while (clusters.size() < units.size())
            clusters.push_back({dummy_pos}); 
    }

    vector<vector<int>> cost(units.size(), vector<int>(units.size()));
    vector<vector<Pos>> nearest_pos(units.size(), vector<Pos>(units.size()));
    rep(unit_i, units.size())
    {
        const Pos& pos = units[unit_i].pos;
        rep(cluster_i, clusters.size())
        {
            const int inf = 114514;
            int nearest_dist = inf;
            Pos nearest;
            random_shuffle(all(clusters[cluster_i]));
            for (auto& p : clusters[cluster_i])
            {
                int d = pos.dist(p);
                if (d < nearest_dist)
                {
                    nearest_dist = d;
                    nearest = p;
                }
            }
            cost[unit_i][cluster_i] = nearest_dist;
            nearest_pos[unit_i][cluster_i] = nearest;
        }
    }
    const vector<int> matching = hungarian(cost);

//     vector<Pos> target_pos(units.size());
    map<int, char> order;
    rep(unit_i, units.size())
    {
        const auto& target = nearest_pos[unit_i][matching[unit_i]];
//         if (target == dummy_pos)
//             target_pos[unit_i] = units[unit_i].pos;
//         else
//             target_pos[unit_i] = target;
        if (target != dummy_pos)
        {
            order[units[unit_i].id] = to_order(decide_dir(units[unit_i].pos, target));
        }
    }
    return order;
}


const int DIJKSTRA_INF = ten(8);
struct DijkstraResult
{
    Pos start;
    Board<int> cost;
    Board<Dir> prev_dir;

    Dir find_dir(const Pos& goal) const
    {
        assert(prev_dir.at(goal) != INVALID);

        for (Pos p = goal; ; )
        {
            assert(prev_dir.at(p) != INVALID);

            Pos prev = p + NEXT_POS[prev_dir.at(p)];
            if (prev == start)
                return rev_dir(prev_dir.at(p));

            p = prev;
        }
        abort();
    }
};
DijkstraResult dijkstra(const Pos& start, const Board<int>& cost, const vector<int>& move_cost)
{
    Board<int> dp(DIJKSTRA_INF);
    Board<Dir> prev_dir(INVALID);
    typedef pair<int, Pos> P;
    priority_queue<P, vector<P>, greater<P>> q;
    dp.at(start) = 0;
    q.push(P(0, start));
    while (!q.empty())
    {
        int c;
        Pos cur;
        tie(c, cur) = q.top();
        q.pop();
        if (c > dp.at(cur))
            continue;

        rep(dir, 4)
        {
            Pos next = cur + NEXT_POS[dir];
            if (next.in_board())
            {
                int nc = c + cost.at(next) + move_cost[dir];
                if (nc < dp.at(next))
                {
                    dp.at(next) = nc;
                    prev_dir.at(next) = (Dir)rev_dir(dir);
                    q.push(P(nc, next));
                }
            }
        }
    }

    DijkstraResult res;
    res.start = start;
    res.cost = dp;
    res.prev_dir = prev_dir;
    return res;
}
vector<pair<Unit, Pos>> match_unit_goal(const vector<Unit>& units, const vector<Pos>& goals)
{
    vector<pair<Unit, Pos>> unit_goal;
    const int inf = 1919810;
    const int n = max(units.size(), goals.size());
    vector<vector<int>> cost_to_assign(n, vector<int>(n, inf));
    rep(i, units.size()) rep(j, goals.size())
        cost_to_assign[i][j] = units[i].pos.dist(goals[j]);

    vector<int> matching = hungarian(cost_to_assign);
    rep(i, units.size())
    {
        int j = matching[i];
        if (j < goals.size())
            unit_goal.push_back(make_pair(units[i], goals[j]));
    }
    return unit_goal;
}
map<int, char> search_moves_by_dijkstra(const vector<pair<Unit, Pos>>& unit_goal, Board<int> cost)
{
    vector<int> move_cost(4);
    move_cost[DOWN] = move_cost[RIGHT] = 5;
    move_cost[LEFT] = move_cost[UP] = 500;

    map<int, char> order;
    for (auto& it : unit_goal)
    {
        Unit unit;
        Pos goal;
        tie(unit, goal) = it;

        DijkstraResult res = dijkstra(unit.pos, cost, move_cost);
        order[unit.id] = res.find_dir(goal);
    }
    return order;
}

AI::AI()
:
    known(false),
    visited(false),
    go(false),
    is_lila(false)
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
    auto matching = hungarian(cost);

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

void AI::move_scouters(map<int, char>& order, vector<Unit>& remain_workers, const vector<Unit>& enemy_units)
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

        vector<int> move_cost(4);
        move_cost[DOWN] = move_cost[RIGHT] = 10;
        move_cost[LEFT] = move_cost[UP] = 50;
        rep(i, down_scouters.size())
        {
            const Unit& u = down_scouters[i];

            //                     const Pos goal = Pos(99 - 40, 99) + Pos(40 / NUM_SCOUTERS * i, -40 / NUM_SCOUTERS * i);
            const Pos goal(90, 99 - 9 * i);
            //                     const Pos goal(90, 99);

            if (u.pos == goal)
                once_goal_scouter_ids.insert(u.id);

            if (!once_goal_scouter_ids.count(u.id))
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
            //                     const Pos goal = Pos(99, 99 - 40) + Pos(-40 / NUM_SCOUTERS * i, 40 / NUM_SCOUTERS * i);
            const Pos goal(99 - 9 * i, 90);
            //                     const Pos goal(99, 90);

            if (u.pos == goal)
                once_goal_scouter_ids.insert(u.id);

            if (!once_goal_scouter_ids.count(u.id))
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

    for (auto& unit : input.my_units)
    {
        for (auto& diff : RANGE_POS[unit.sight_range()])
        {
            Pos p = unit.pos + diff;
            if (p.in_board())
                known.at(p) = true;
        }
    }

    resource_pos.insert(resource_pos.end(), all(input.resource_pos_in_sight));
    uniq(resource_pos);


    {
        if (!input.get_enemy({CASTLE}).empty())
            enemy_castle = input.get_enemy({CASTLE})[0];

        auto ene_vs = input.get_enemy({VILLAGE});
        enemy_villages.insert(enemy_villages.end(), all(ene_vs));
        uniq(ene_vs);
    }


    int remain_resources = input.resources;

    map<int, char> order;

    if (my_workers.size() < 45 && input.current_turn < 150)
    {
        if (remain_resources >= CREATE_COST[WORKER])
        {
            order[my_castle.id] = CREATE_ORDER[WORKER];
            remain_resources -= CREATE_COST[WORKER];
        }
    }

    {
        for (auto& base : my_bases)
        {
            if (remain_resources >= 20)
            {
                const UnitType warrior_types[] = { KNIGHT, FIGHTER, ASSASSIN };
                vector<double> ratio = { 5, 1, 3 };

                static Random ran;
                int t = ran.select(ratio);
                UnitType type = warrior_types[t];

                if (type == ASSASSIN && remain_resources < 60 + 20 * (int)(my_bases.size() - 1))
                    type = KNIGHT;
                else if (type == FIGHTER && remain_resources < 40 + 20 * (int)(my_bases.size() - 1))
                    type = KNIGHT;

                order[base.id] = CREATE_ORDER[type];
                remain_resources -= CREATE_COST[type];
            }
        }
    }


    {
        auto remain_workers = input.get_my({WORKER});

        if (enemy_castle.id == -1)
        {
            if (remain_resources >= CREATE_COST[VILLAGE])
            {
//                 dump(input.current_turn);
                map<int, int> predict_damage = simulate_damage(enemy_units, my_units);
                for (auto& worker : remain_workers)
                {
//                     if (worker.id == 1)
//                     {
//                         assert(predict_damage.count(1));
//                         fprintf(stderr, "%4d: %4d, %4d\n", input.current_turn, worker.hp, prev_unit[worker.id].hp - predict_damage[worker.id]);
//                     }
                    if (prev_unit.count(worker.id) && worker.hp < prev_unit[worker.id].hp - predict_damage[worker.id])
                    {
                        merge_remove(order, remain_workers, worker.id, CREATE_ORDER[VILLAGE]);
                        break;
                    }
                }
            }

            move_scouters(order, remain_workers, enemy_units);
        }
        else
        {
            Board<bool> base_cand(false);
            bool found = false;
            rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
            {
                int ex = enemy_castle.pos.x;
                int ey = enemy_castle.pos.y;
                int d = enemy_castle.pos.dist(Pos(x, y));
                if (13 <= d && d <= 16 &&
                        x >= ex && y >= ey)
                {
                    base_cand.at(x, y) = true;
                    found = true;
                }
            }
            if (!found)
            {
                rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
                {
                    int ex = enemy_castle.pos.x;
                    int ey = enemy_castle.pos.y;
                    int d = enemy_castle.pos.dist(Pos(x, y));
                    if (13 <= d && d <= 16 &&
                        (x >= ex || y >= ey))
                    {
                        base_cand.at(x, y) = true;
                        found = true;
                    }
                }
            }

            if (my_bases.size() < 2 && remain_resources >= CREATE_COST[BASE])
//                 && (remain_resources >= CREATE_COST[BASE] + 300 || input.resources >= 40))
            {
                int best_dist = 810;
                Unit best_worker;
                for (auto& worker : remain_workers)
                {
                    int d = worker.pos.dist(enemy_castle.pos);
                    //                     if (d < best_dist && d > 10 && worker.pos.x >= enemy_castle.pos.x && worker.pos.y >= enemy_castle.pos.y)
                    if (base_cand.at(worker.pos))
                    {
                        best_dist = d;
                        best_worker = worker;
                    }
                }
                if (best_dist != 810)
                {
                    remain_resources -= CREATE_COST[BASE];
                    merge_remove(order, remain_workers, best_worker.id, CREATE_ORDER[BASE]);
                }
            }

            {
                map<int, char> base_pos_order;

                const auto cost = cost_table(enemy_units);
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
                        if (best_cost != DIJKSTRA_INF)
                        {
                            base_pos_order[worker.id] = to_order(res.find_dir(best_goal));
                        }
                    }
                }

                merge_remove(order, remain_workers, base_pos_order);
            }
        }

        move_for_resource(order, remain_workers);

        {
            Board<bool> mark(false);
            rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
                mark.at(x, y) = Pos(x, y).dist(Pos(0, 0)) <= 99 && !known.at(x, y);
            merge_remove(order, remain_workers, search_moves(remain_workers, mark));
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
                if (unit.type != CASTLE)
                {
                    if (unit.pos == enemy_castle.pos)
                        on_castle.push_back(unit);
                    if (unit.pos.dist(enemy_castle.pos) <= 2)
                        around_castle.push_back(unit);
                }
            }
            if (around_castle.size() >= 15)
                is_lila = true;

            auto remain_warriors = input.get_my({KNIGHT, FIGHTER, ASSASSIN});
            map<int, char> warrior_order;

            map<Pos, vector<Unit>> pos_to_warriors;
            for (auto warrior : remain_warriors)
                pos_to_warriors[warrior.pos].push_back(warrior);

            set<Pos> base_pos;
            for (auto& base : my_bases)
                base_pos.insert(base.pos);

            for (auto& it : pos_to_warriors)
            {
                Pos pos;
                vector<Unit> warriors;
                tie(pos, warriors) = it;

                if (is_lila)
                {
                    if (my_warriors.size() >= around_castle.size() + 40)
                        go = true;
                    else if (!go)
                    {
                        if (pos.dist(enemy_castle.pos) <= 10)
                        {
                            for (auto& warrior : warriors)
                                warrior_order[warrior.id] = to_order(rev_dir(decide_dir(warrior.pos, enemy_castle.pos)));
                        }
                    }

                    if (go)
                    {
                        if (pos.dist(enemy_castle.pos) > 2)
                        {
                            for (auto& warrior : warriors)
                                warrior_order[warrior.id] = to_order(decide_dir(warrior.pos, enemy_castle.pos));
                        }
                    }
                }
                else
                {
//                     const int go_line = (in_sight && on_castle.size() == 0 ? 5 : 60);
                    const int go_line = 60;
                    if (pos.dist(enemy_castle.pos) > 2 && (!base_pos.count(pos) || warriors.size() >= go_line || go))
                    {
                        go = true;
                        for (auto& warrior : warriors)
                            warrior_order[warrior.id] = to_order(decide_dir(warrior.pos, enemy_castle.pos));
                    }
                }
            }

            merge_remove(order, remain_warriors, warrior_order);
        }
    }


    prev_unit.clear();
    for (auto& unit : my_units)
        prev_unit[unit.id] = unit;
    //     if (prev_unit.count(1))
    //         fprintf(stderr, "%4d: %4d\n", input.current_turn, prev_unit[1].hp);

    return order;
}

