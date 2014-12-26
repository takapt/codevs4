#ifndef INCLUDED_POS
#define INCLUDED_POS

#include "GameCommon.hpp"

struct Pos
{
    int x, y;

    Pos(int x, int y);
    Pos();

    bool operator==(const Pos& other) const;
    bool operator!=(const Pos& other) const;
    void operator+=(const Pos& other);
    void operator-=(const Pos& other);
    void operator/=(int a);
    Pos operator+(const Pos& other) const;
    Pos operator-(const Pos& other) const;
    Pos operator*(int a) const;
    bool operator<(const Pos& other) const;

    bool in_board() const;
    int dist(const Pos& pos) const;
    Dir dir(const Pos& to) const;
};
namespace std
{
    ostream& operator<<(ostream& os, const Pos& pos);
}

const vector<Pos> NEXT_POS = {
    Pos(1, 0),
    Pos(0, -1),
    Pos(-1, 0),
    Pos(0, 1),
};

vector<Pos> get_range_pos(int range);
const vector<Pos> RANGE_POS[11] = {
    get_range_pos(0),
    get_range_pos(1),
    get_range_pos(2),
    get_range_pos(3),
    get_range_pos(4),
    get_range_pos(5),
    get_range_pos(6),
    get_range_pos(7),
    get_range_pos(8),
    get_range_pos(9),
    get_range_pos(10),
};

#endif
