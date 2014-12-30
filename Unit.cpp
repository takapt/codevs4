#include "Unit.hpp"

map<int, int> simulate_damage(const vector<Unit>& attackers, const vector<Unit>& defencers)
{
    map<Pos, int> defencers_in_pos;
    for (auto& defencer : defencers)
        ++defencers_in_pos[defencer.pos];

    map<int, int> id_to_damage;
    for (auto& attacker : attackers)
    {
        int k = 0;
        for (auto& it : defencers_in_pos)
            if (attacker.pos.dist(it.first) <= attacker.attack_range())
                k += min(10, it.second);

        for (auto& defencer : defencers)
            if (attacker.pos.dist(defencer.pos) <= attacker.attack_range())
                id_to_damage[defencer.id] += DAMAGE_TABLE[attacker.type][defencer.type] / k;
    }
    return id_to_damage;
}
