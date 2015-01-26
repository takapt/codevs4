#include "AI.hpp"
#include "Common.hpp"
#include "GameCommon.hpp"
#include "Pos.hpp"
#include "Unit.hpp"
#include "Board.hpp"
#include "IO.hpp"

#include "Random.hpp"



#define fst first
#define snd second
 
typedef int value_type;
vector<int> min_assignment(const vector<vector<value_type>> &c)
{
    if (c.empty())
        return {};

    const int n = c.size(), m = c[0].size(); // assert(n <= m);
    if (n > m)
    {
        vector<vector<value_type>> tc(m, vector<value_type>(n));
        rep(i, n) rep(j, m)
            tc[j][i] = c[i][j];
        auto matching = min_assignment(tc);
        assert(matching.size() == m);
        vector<int> res(n, -1);
        rep(j, m)
        {
            int i = matching[j];
            assert(0 <= i && i < n);
            res[i] = j;
        }
        return res;
    }

    assert(n <= m);
    vector<value_type> v(m), dist(m);        // v: potential
    vector<int> matchL(n,-1), matchR(m,-1);  // matching pairs
    vector<int> index(m), prev(m);
    iota(all(index), 0);

    auto residue = [&](int i, int j) { return c[i][j] - v[j]; };
    for (int f = 0; f < n; ++f) {
        for (int j = 0; j < m; ++j) {
            dist[j] = residue(f, j);
            prev[j] = f;
        }
        value_type w;
        int j, l;
        for (int s = 0, t = 0;;) {
            if (s == t) {
                l = s; w = dist[index[t++]]; 
                for (int k = t; k < m; ++k) {
                    j = index[k];
                    value_type h = dist[j];
                    if (h <= w) {
                        if (h < w) { t = s; w = h; }
                        index[k] = index[t]; index[t++] = j;
                    }
                }
                for (int k = s; k < t; ++k) {
                    j = index[k];
                    if (matchR[j] < 0) goto aug;
                }
            }
            int q = index[s++], i = matchR[q];
            for (int k = t; k < m; ++k) {
                j = index[k];
                value_type h = residue(i,j) - residue(i,q) + w;
                if (h < dist[j]) { 
                    dist[j] = h; prev[j] = i;
                    if (h == w) {
                        if (matchR[j] < 0) goto aug;
                        index[k] = index[t]; index[t++] = j;
                    }
                }
            }
        }
aug:for(int k = 0; k < l; ++k) 
        v[index[k]] += dist[index[k]] - w;
    int i;
    do {
        matchR[j] = i = prev[j]; swap(j, matchL[i]);
    } while (i != f);
    }
    return matchL;
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

//         // dummy cluster
//         while (clusters.size() < units.size())
//             clusters.push_back({dummy_pos}); 
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
    const vector<int> matching = min_assignment(cost);

    map<int, char> order;
    rep(unit_i, units.size())
    {
        const auto& target = nearest_pos[unit_i][matching[unit_i]];
        if (target != dummy_pos)
        {
            order[units[unit_i].id] = to_order(decide_dir(units[unit_i].pos, target));
        }
    }
    return order;
}


// TODO: markをcostテーブルかなんかにする
map<int, char> search_moves_by_min_assignment(const vector<Unit>& units, const Board<int>& pos_cost)
{
    vector<Pos> targets;
    const int sight_width = SIGHT_RANGE[WORKER] * 2 + 1;
    for (int y = SIGHT_RANGE[WORKER]; y < BOARD_SIZE; y += sight_width)
    {
        for (int x = SIGHT_RANGE[WORKER]; x < BOARD_SIZE; x += sight_width)
        {
            if (pos_cost.at(x, y) != -1)
                targets.push_back(Pos(x, y));
        }
    }
    const Pos dummy_pos(-1, -1);
    while (targets.size() < units.size())
        targets.push_back(dummy_pos);

    const int inf = ten(7);
    vector<vector<int>> cost(units.size(), vector<int>(targets.size()));
    rep(i, units.size()) rep(j, targets.size())
    {
        if (targets[j] == dummy_pos)
            cost[i][j] = inf;
        else
            cost[i][j] = 1000 * units[i].pos.dist(targets[j]) + pos_cost.at(targets[j]);
    }

    vector<int> matching = min_assignment(cost);
    map<int, char> order;
    rep(i, units.size())
    {
        int j = matching[i];
        if (cost[i][j] != inf)
        {
            assert(units[i].pos != targets[j]);
            order[units[i].id] = to_order(decide_dir(units[i].pos, targets[j]));
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

    vector<int> matching = min_assignment(cost_to_assign);
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
    is_lila(false),
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
            if (!is_lila)
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

    if (!order.count(my_castle.id) && my_workers.size() < 15 && input.current_turn < 150 && enemy_castle.id == -1)
    {
        if (remain_resources >= CREATE_COST[WORKER])
        {
            order[my_castle.id] = CREATE_ORDER[WORKER];
            remain_resources -= CREATE_COST[WORKER];

            ++num_workers_created_for_map[my_castle.pos];
        }
    }

    {
        // TODO: for_mapに追加
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

        if (input.current_turn <= 130)
        {
//             fprintf(stderr, "%4d: %4d, %2d\n", input.current_turn, remain_resources, (int)resource_pos.size());
            vector<Pos> added_villages_pos;
            for (Pos& pos : resource_pos)
            {
                if (remain_resources < CREATE_COST[VILLAGE])
                    break;

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
                            merge_remove(order, remain_workers, best_worker.id, CREATE_ORDER[VILLAGE]);
                            remain_resources -= CREATE_COST[VILLAGE];
                            added_villages_pos.push_back(best_worker.pos);
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
        if (in_sight && around_castle.size() == 0)
            ratio = {1, 0, 0};
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

                if (stalkers - 1 >= (int)scouters.size() && my_bases.size() < 2 && scouters.size() <= 3 && remain_resources >= CREATE_COST[BASE])
                {
                    int best_dist = 810;
                    Unit best_worker;
                    for (auto& worker : remain_workers)
                    {
                        int d = worker.pos.dist(Pos(99, 99));
                        if (d < best_dist)
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
            }

            move_scouters(order, remain_workers, enemy_units);

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


            if (is_lila && remain_resources >= CREATE_COST[VILLAGE])
            {
                Board<bool> safe_scout_pos(false);
                const int cx = enemy_castle.pos.x, cy = enemy_castle.pos.y;
                for (auto& diff : RANGE_POS[enemy_castle.sight_range()])
                {
                    Pos p = enemy_castle.pos + diff;
                    if (p.in_board())
                    {
                        if (p.x >= cx + 5 || p.y >= cy + 5)
                            safe_scout_pos.at(p) = true;
                    }
                }
                bool any_safe_scout_village = false;
                for (auto& v : my_villages)
                    any_safe_scout_village |= safe_scout_pos.at(v.pos);

                if (!any_safe_scout_village)
                {
                    Unit creater;
                    creater.id = -1;
                    for (auto& worker : remain_workers)
                    {
                        if (safe_scout_pos.at(worker.pos))
                            creater = worker;
                    }
                    if (creater.id != -1)
                    {
                        merge_remove(order, remain_workers, creater.id, CREATE_ORDER[VILLAGE]);
                    }
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
                    if (!is_lila)
                    {
                        low = 12;
                        high = 25;
                    }
                    else
                    {
                        low = 11;
                        high = 13;
                    }
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
                        if (!is_lila)
                        {
                            low = 12;
                            high = 25;
                        }
                        else
                        {
                            low = 11;
                            high = 13;
                        }
                        if (low <= d && d <= high &&
                                (x >= ex + k|| y >= ey + k))
                        {
                            base_cand.at(x, y) = true;
                            found = true;
                        }
                    }
                }
            }

            if (!is_lila)
            {
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


                    if (((all_stalked && scouters.size() <= 2 && enemy_castle.pos.dist(worker.pos) > 11) || base_cand.at(worker.pos)) && score < best_score)
                    {
                        best_score = score;
                        best_worker = worker;
                    }
                }
                if (best_score != 1919810)
                {
                    remain_resources -= CREATE_COST[BASE];
                    merge_remove(order, remain_workers, best_worker.id, CREATE_ORDER[BASE]);
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
                rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
                    if (!base_cand.at(x, y))
                        cost.at(x, y) = 100;
                for (auto& worker : remain_workers)
                {
                    if (!base_cand.at(worker.pos) &&
                            (find(all(right_scouter_ids), worker.id) != right_scouter_ids.end() ||
                             find(all(down_scouter_ids), worker.id) != down_scouter_ids.end()))
                    {
                        // TODO: 過去のストーカー情報を持って、3ターン連続でストークならとかにする
//                         Unit stalker;
//                         stalker.id = -1;
//                         for (auto& w : enemy_warriors)
//                             if (w.pos.dist(worker.pos) <= w.sight_range())
//                                 stalker = w;
//                         if (stalker.id != -1 && worker.pos != stalker.pos)
//                         {
//                             merge_remove(order, remain_workers, worker.id, to_order(decide_dir(worker.pos, stalker.pos)));
//                             continue;
//                         }

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

//         move_for_resource(order, remain_workers);

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
            if (around_castle.size() >= 15)
                is_lila = true;

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
                    if ((in_sight && on_castle.size() <= 10 && my_warriors.size() >= 70) || false&&my_warriors.size() >= around_castle.size() + 80)
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
                    int around_my_castle = 0;
                    for (auto& w : enemy_warriors)
                        if (my_castle.pos.dist(w.pos) <= 10)
                            ++around_my_castle;
                    if (around_my_castle >= 3)
                        go = true;

                    // 10: 80 or 60(微妙にこっちのがいい。誤差レベル)
                    // 30: 80?? or 90(90のが1勝おおかった)
                    const int go_line = (in_sight && on_castle.size() == 0 ? 1 : 80);
                    if (my_warriors.size() >= go_line)
                        go = true;
                    if (pos.dist(enemy_castle.pos) > 2 && (!base_pos.count(pos) || my_warriors.size() >= go_line || go))
                    {
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

    return order;
}

