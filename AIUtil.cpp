#include "AIUtil.hpp"

 
vector<int> min_assignment(const vector<vector<int>> &c)
{
    if (c.empty())
        return {};

    const int n = c.size(), m = c[0].size(); // assert(n <= m);
    if (n > m)
    {
        vector<vector<int>> tc(m, vector<int>(n));
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
    vector<int> v(m), dist(m);        // v: potential
    vector<int> matchL(n,-1), matchR(m,-1);  // matching pairs
    vector<int> index(m), prev(m);
    iota(all(index), 0);

    auto residue = [&](int i, int j) { return c[i][j] - v[j]; };
    for (int f = 0; f < n; ++f) {
        for (int j = 0; j < m; ++j) {
            dist[j] = residue(f, j);
            prev[j] = f;
        }
        int w;
        int j, l;
        for (int s = 0, t = 0;;) {
            if (s == t) {
                l = s; w = dist[index[t++]]; 
                for (int k = t; k < m; ++k) {
                    j = index[k];
                    int h = dist[j];
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
                int h = residue(i,j) - residue(i,q) + w;
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
    if (cur == to)
        return Dir(rand() % 4);

    Dir dir;
    if (abs(to.x - cur.x) > abs(to.y - cur.y))
        dir = to.x < cur.x ? LEFT : RIGHT;
    else
        dir = to.y < cur.y ? UP : DOWN;
    return dir;
}

map<int, char> search_moves(const vector<Unit>& units, const Board<bool>& mark, const int max_dist, const Pos remain_target)
{
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

Dir DijkstraResult::find_dir(const Pos& goal) const
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

