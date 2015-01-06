#include "GameCommon.hpp"
#include "Common.hpp"


int rev_dir(int dir)
{
    return (dir + 2) % 4;
}
Dir rev_dir(Dir dir)
{
    return Dir(rev_dir(dir));
}
namespace std
{
    ostream& operator<<(ostream& os, Dir dir)
    {
        assert(0 <= dir && dir < 4);
        os << STR_DIR[dir];
        return os;
    }
}


char move_order(Dir dir)
{
    assert(0 <= dir && dir < 4);
    return STR_DIR[dir][0];
}
