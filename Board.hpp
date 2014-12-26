#ifndef INCLUDED_ARRAY2D
#define INCLUDED_ARRAY2D

#include "Common.hpp"
#include "Pos.hpp"
#include "GameCommon.hpp"

template <typename T>
class Board 
{
public:
    Board()
    {
    }

    Board(const T& init_val)
    {
        clear(init_val);
    }

    T& at(int x, int y)
    {
        return a[y][x];
    }
    T& at(const Pos& pos)
    {
        return at(pos.x, pos.y);
    }

    const T& at(int x, int y) const
    {
        return a[y][x];
    }
    const T& at(const Pos& pos) const
    {
        return at(pos.x, pos.y);
    }


    void clear(const T& val)
    {
        rep(y, BOARD_SIZE) rep(x, BOARD_SIZE)
            at(x, y) = val;
    }

    void print() const
    {
        print2d(a, BOARD_SIZE, BOARD_SIZE);
    }

private:
    T a[100][128];
};

#endif
