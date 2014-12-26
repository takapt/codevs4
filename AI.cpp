#include "AI.hpp"
#include "Common.hpp"
#include "GameCommon.hpp"
#include "Pos.hpp"
#include "Unit.hpp"
#include "Board.hpp"
#include "IO.hpp"

vector<vector<Pos>> clustering(const vector<Pos>& pos, const int num_cluster)
{
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


AI::AI()
: known(false)
{
}

map<int, char> AI::solve(const InputResult& input)
{
    Unit my_castle;
    vector<Unit> my_workers, my_warriors, my_villages;
    vector<Unit> my_units = input.my_units;
    for (auto& unit : my_units)
    {
        if (unit.type == WORKER)
            my_workers.push_back(unit);
        else if (unit.type == CASTLE)
            my_castle = unit;
        else if (unit.type == VILLAGE)
            my_villages.push_back(unit);
        else
            my_warriors.push_back(unit);

        for (auto& diff : RANGE_POS[unit.sight_range()])
        {
            Pos p = unit.pos + diff;
            if (p.in_board())
                known.at(p) = true;
        }
    }

//     while (my_workers.size() > 2)
//         my_workers.pop_back();

    Board<bool> start(false);
    Board<bool> unknown(false);
    rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
    {
        if (abs(x) + abs(y) > 150)
            continue;

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

    vector<Pos> near_unknown_pos = list_near_pos(start, known, 10);
    vector<vector<Pos>> clusters;
    const Pos dummy_pos(1919, 810);
    if (near_unknown_pos.size() >= my_workers.size())
        clusters = clustering(near_unknown_pos, my_workers.size());
    else
    {
        for (auto& p : near_unknown_pos)
            clusters.push_back({p});
        // dummy cluster
        while (clusters.size() < my_workers.size())
            clusters.push_back({dummy_pos}); 
    }

    vector<vector<int>> cost(my_workers.size(), vector<int>(my_workers.size()));
    vector<vector<Pos>> nearest_pos(my_workers.size(), vector<Pos>(my_workers.size()));
    rep(worker_i, my_workers.size())
    {
        const Pos& pos = my_workers[worker_i].pos;
        rep(cluster_i, clusters.size())
        {
            const int inf = 114514;
            int nearest_dist = inf;
            Pos nearest;
            for (auto& p : clusters[cluster_i])
            {
                int d = pos.dist(p);
                if (d < nearest_dist)
                {
                    nearest_dist = d;
                    nearest = p;
                }
            }
            cost[worker_i][cluster_i] = nearest_dist;
            nearest_pos[worker_i][cluster_i] = nearest;
        }
    }
    vector<int> matching = hungarian(cost);
//     dump(matching);
//     print2d(cost, my_workers.size(), my_workers.size());

    vector<Pos> target_pos(my_workers.size());
    rep(worker_i, my_workers.size())
    {
        const auto& target = nearest_pos[worker_i][matching[worker_i]];
        if (target == dummy_pos)
            target_pos[worker_i] = my_workers[worker_i].pos;
        else
            target_pos[worker_i] = target;
    }

    map<int, char> order;
    rep(worker_i, my_workers.size())
    {
        const Pos& cur = my_workers[worker_i].pos;
        const Pos& to = target_pos[worker_i];
        if (cur != to)
        {
            Dir dir;
            if (abs(to.x - cur.x) > abs(to.y - cur.y))
//             if (to.y == cur.y || (to.x != cur.x && rand() % 2))
                dir = to.x < cur.x ? LEFT : RIGHT;
            else
                dir = to.y < cur.y ? UP : DOWN;

            order[my_workers[worker_i].id] = STR_DIR[dir][0];
        }
    }

    if (my_workers.size() < 20 && input.resources >= 40)
        order[my_castle.id] = '0';

    return order;
}

