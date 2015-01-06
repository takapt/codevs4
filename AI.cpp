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
    assert(num_cluster > 0);
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

map<int, char> search_moves(const vector<Unit>& units, const Board<bool>& start, const Board<bool>& mark, const int max_dist, const Pos remain_target = Pos(114, 514))
{
    const vector<Pos> near_mark_pos = list_near_pos(start, mark, max_dist);

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

    vector<Pos> target_pos(units.size());
    rep(unit_i, units.size())
    {
        const auto& target = nearest_pos[unit_i][matching[unit_i]];
        if (target == dummy_pos)
            target_pos[unit_i] = units[unit_i].pos;
        else
            target_pos[unit_i] = target;
    }

    map<int, char> order;
    rep(unit_i, units.size())
    {
        const Pos& cur = units[unit_i].pos;
        const Pos& to = target_pos[unit_i];
        if (cur != to)
            order[units[unit_i].id] = STR_DIR[decide_dir(cur, to)][0];
    }
    return order;
}

const int DIJKSTRA_INF = 1919810;
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
            if (p == start)
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
    group_sizes({10, 60, 1})
{
    enemy_castle.id = -1;

    rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
        enemy_castle_pos_cand.at(x, y) = Pos(x, y).dist(Pos(99, 99)) <= 40;

    next_attack_turn = -1919;
}

Board<int> AI::cost_table(const vector<Unit>& enemy_units) const
{
    Board<int> sum_damage(0);
    for (auto& unit : enemy_units)
    {
        for (auto& diff : RANGE_POS[unit.attack_range()])
        {
            Pos p = unit.pos + diff;
            if (p.in_board())
                sum_damage.at(p) += unit.type == WORKER ? 100 : 200;
        }
    }

    const int NORMAL_ATTACK_RANGE = 2;
    Board<int> cost(0);
    rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
    {
        if (sum_damage.at(x, y) >= 500)
            cost.at(x, y) = 1000;
    }
    return cost;
}
Board<int> AI::down_pass_cost_table(const vector<Unit>& enemy_units) const
{
    auto cost = cost_table(enemy_units);
    rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
    {
        if (x > y)
            cost.at(x, y) = 114514;
        else
            cost.at(x, y) = 100 - (y - x);
    }
    return cost;
}
Board<int> AI::right_pass_cost_table(const vector<Unit>& enemy_units) const
{
    auto cost = cost_table(enemy_units);
    rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
    {
        if (x < y)
            cost.at(x, y) = 114514;
        else
            cost.at(x, y) = 100 - (x - y);
    }
    return cost;
}

map<int, char> AI::solve(const InputResult& input)
{
    Unit my_castle;
    vector<Unit> my_workers, my_warriors, my_villages, my_bases;
    vector<Unit> my_units = input.my_units;
    int remain_resources = input.resources;
    for (auto& unit : my_units)
    {
        if (unit.type == WORKER)
            my_workers.push_back(unit);
        else if (unit.type == CASTLE)
            my_castle = unit;
        else if (unit.type == VILLAGE)
            my_villages.push_back(unit);
        else if (unit.type == BASE)
            my_bases.push_back(unit);
        else
            my_warriors.push_back(unit);

        for (auto& diff : RANGE_POS[unit.sight_range()])
        {
            Pos p = unit.pos + diff;
            if (p.in_board())
                known.at(p) = true;
        }
    }
    const int num_workers = my_workers.size();

    for (auto& pos : input.resource_pos_in_sight)
        if (Pos(0, 0).dist(pos) <= 100)
            resource_pos.push_back(pos);
    uniq(resource_pos);

    vector<Unit> enemy_warriors;
    for (auto& unit : input.enemy_units)
    {
        if (unit.type == CASTLE)
            enemy_castle = unit;
        else if (unit.type == KNIGHT || unit.type == FIGHTER || unit.type == ASSASSIN)
            enemy_warriors.push_back(unit);
    }


    map<int, char> order;

    {
        if (my_bases.size() < 2)
        {
            bool build = false;
            if (remain_resources >= 500)
            {
                int max_dist = -1;
                Unit best_worker;
                for (auto& worker : my_workers)
                {
                    int d = worker.pos.dist(Pos(0, 0));
                    if (d > max_dist)
                    {
                        max_dist = d;
                        best_worker = worker;
                    }
                }
                if (max_dist != -1 && min(best_worker.pos.x, best_worker.pos.y) >= 80)
                {
                    remain_resources -= 500;
                    order[best_worker.id] = '6';
                    my_workers.erase(find(all(my_workers), best_worker));
                    build = true;
                }
            }

            if (my_workers.size() && !build)
            {
                sort(all(my_workers));
                order[my_workers[0].id] = (my_workers[0].pos.x < 99 ? 'R' : 'D');
                if (my_workers.size() > 1)
                    order[my_workers[1].id] = (my_workers[1].pos.y < 99 ? 'D' : 'R');
                my_workers.erase(my_workers.begin(), my_workers.begin() + min<int>(2, my_workers.size()));
            }
        }
    }

    {
        for (auto& base : my_bases)
        {
            if (remain_resources >= 20)
            {

                const UnitType warrior_types[] = { KNIGHT, FIGHTER, ASSASSIN };
                vector<double> ratio = { 5, 1, 3 };

                const int costs[] = { 20, 40, 60 };
                static Random ran;
                int t = ran.select(ratio);
                char type = "123"[t];

                if (type == '3' && remain_resources < 80)
                    type = '1';
                else if (type == '2' && remain_resources < 60)
                    type = '1';

                order[base.id] = type;
                if (type == '1')
                    remain_resources -= 20;
                else if (type == '2')
                    remain_resources -= 40;
                else
                    remain_resources -= 60;
            }
        }
    }

    {
        map<Pos, vector<Unit>> on_base_warriors;
        for (Unit& warrior : my_warriors)
        {
            for (Unit& base : my_bases)
                if (warrior.pos == base.pos)
                    on_base_warriors[base.pos].push_back(warrior);
        }
        for (auto& it : on_base_warriors)
        {
            auto& warriors = it.second;
            uniq(warriors);
            if (warriors.size() < group_sizes[0])
            {
                for (Unit& warrior : it.second)
                    my_warriors.erase(find(all(my_warriors), warrior));
            }
            else
            {
                if (group_sizes.size() > 1)
                    group_sizes.erase(group_sizes.begin());
            }
        }

        if (enemy_castle.id != -1)
        {
            int around_enemies = 0;
            for (auto& enemy_warrior : enemy_warriors)
                if (enemy_warrior.pos.dist(enemy_castle.pos) <= 2)
                    ++around_enemies;
            if (around_enemies >= 30)
                next_attack_turn = input.current_turn + 30;

            const bool back = input.current_turn < next_attack_turn;
            for (auto& warrior : my_warriors)
            {
                const int d = warrior.pos.dist(enemy_castle.pos);
                if (back && d <= 10)
                {
                    int best_dir = -1;
                    rep(dir, 4)
                        if (enemy_castle.pos.dist(warrior.pos + NEXT_POS[dir]) > d)
                            best_dir = dir;
                    assert(best_dir != -1);
                    order[warrior.id] = STR_DIR[best_dir][0];
                }
                else if ((!back && d > warrior.attack_range()) || (back && d > 10 + 1))
                    order[warrior.id] = STR_DIR[decide_dir(warrior.pos, enemy_castle.pos)][0];
            }
        }
        else
        {
            if (!my_warriors.empty())
            {
                Board<bool> start(false);
                Board<bool> mark(false);
                rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
                {
                    if (abs(x - 99) + abs(y - 99) <= 40)
                    {
                        mark.at(x, y) = !known.at(x, y);
                        start.at(x, y) = !known.at(x, y);
                    }
                }

                auto order_to_search_castle = search_moves(my_warriors, start, mark, 200, Pos(99, 99));
                for (auto& it : order_to_search_castle)
                    assert(!order.count(it.first));
                order.insert(all(order_to_search_castle));
            }
        }
    }

    vector<Unit> free_workers;
    {
        vector<Pos> rsrc_pos;
        rep(resource_i, resource_pos.size())
        {
            int fixed = 0;
            for (auto& worker : my_workers)
                if (fixed_worker_ids.count(worker.id) && worker.pos == resource_pos[resource_i])
                    ++fixed;

            for (auto& worker : my_workers)
            {
                if (fixed == MAX_RESOURCE_GAIN)
                    break;

                if (!fixed_worker_ids.count(worker.id) && worker.pos == resource_pos[resource_i])
                {
                    ++fixed;
                    fixed_worker_ids.insert(worker.id);
                }
            }

            const int need = MAX_RESOURCE_GAIN - fixed;
            rep(_, need)
                rsrc_pos.push_back(resource_pos[resource_i]);
        }

        vector<Unit> remain_workers;
        for (auto& worker : my_workers)
            if (!fixed_worker_ids.count(worker.id))
                remain_workers.push_back(worker);

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
                order_to_resource[worker.id] = STR_DIR[decide_dir(worker.pos, rsrc_pos[rsrc_i])][0];
                my_workers.erase(find(all(my_workers), worker));
            }
            else
                free_workers.push_back(worker);
        }
        order.insert(all(order_to_resource));
    }

    if (!free_workers.empty())
    {
        Board<bool> start(false);
        Board<bool> unknown(false);
        rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
        {
            if (x + y <= 100)
            {
                unknown.at(x, y) = !known.at(x, y);

                if (unknown.at(x, y))
                {
                    bool known_border = false;
                    for (auto& diff : NEXT_POS)
                    {
                        Pos p = Pos(x, y) + diff;
                        if (p.in_board() && known.at(p))
                            known_border = true;
                    }
                    start.at(x, y) = known_border;
                }
            }
        }
        auto order_to_search_resource = search_moves(free_workers, start, unknown, 100);
        for (auto& it : order_to_search_resource)
            assert(!order.count(it.first));
        order.insert(all(order_to_search_resource));
    }


    if (my_bases.empty() && num_workers < 45 && remain_resources >= 40)
    {
        order[my_castle.id] = '0';
    }

    return order;
}

