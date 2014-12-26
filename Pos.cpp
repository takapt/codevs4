#include "Pos.hpp"
#include "GameCommon.hpp"

Pos::Pos(int x, int y)
: x(x), y(y)
{
}
Pos::Pos()
{
}

bool Pos::operator==(const Pos& other) const
{
    return x == other.x && y == other.y;
}
bool Pos::operator!=(const Pos& other) const
{
    return x != other.x || y != other.y;
}

void Pos::operator+=(const Pos& other)
{
    x += other.x;
    y += other.y;
}
void Pos::operator-=(const Pos& other)
{
    x -= other.x;
    y -= other.y;
}

void Pos::operator/=(int a)
{
    x /= a;
    y /= a;
}

Pos Pos::operator+(const Pos& other) const
{
    Pos res = *this;
    res += other;
    return res;
}
Pos Pos::operator-(const Pos& other) const
{
    Pos res = *this;
    res -= other;
    return res;
}
Pos Pos::operator*(int a) const
{
    return Pos(x * a, y * a);
}

bool Pos::operator<(const Pos& other) const
{
    if (x != other.x)
        return x < other.x;
    else
        return y < other.y;
}

bool Pos::in_board() const
{
    return 0 <= x && x < BOARD_SIZE && 0 <= y && y < BOARD_SIZE;
}

int Pos::dist(const Pos& pos) const
{
    return abs(pos.x - x) + abs(pos.y - y);
}

Dir Pos::dir(const Pos& to) const
{
    assert(dist(to) == 1);
    Pos diff = to - *this;
    if (diff.x == 0)
    {
        if (diff.y == -1)
            return UP;
        else
            return DOWN;
    }
    else
    {
        if (diff.x == 1)
            return RIGHT;
        else
            return LEFT;
    }
}

ostream& std::operator<<(ostream& os, const Pos& pos)
{
    char buf[256];
    sprintf(buf, "(%3d, %3d)", pos.x, pos.y);
    os << buf;
    return os;
}

vector<Pos> get_range_pos(int range)
{
    vector<Pos> range_pos;
    for (int y = -range; y <= range; ++y)
        for (int x = -range; x <= range; ++x)
            if (abs(x) + abs(y) <= range)
                range_pos.push_back(Pos(x, y));
    return range_pos;
}
