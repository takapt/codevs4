#include "Header.hpp"
#include "Pos.hpp"

template <typename T>
class Array2D
{
public:
    Array2D(int w, int h)
        : w_(w), h_(h)
    {
    }

    Array2D(int w, int h, const T& init_val)
        : w_(w), h_(h)
    {
        clear(init_val);
    }

    Array2D()
        : w_(-114514), h_(-1919810)
    {
    }

    int width() const { return w_; }
    int height() const { return h_; }

    T& at(int x, int y)
    {
        assert(in_rect(x, y, width(), height()));
        return a[y][x];
    }
    T& at(const Pos& pos)
    {
        return at(pos.x, pos.y);
    }

    void clear(const T& val)
    {
        rep(y, height()) rep(x, width())
            at(x, y) = val;
    }

private:
    int w_, h_;
    T a[100][128];
};

