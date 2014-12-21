#include "Pos.hpp"

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
bool Pos::operator !=(const Pos& other) const
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

