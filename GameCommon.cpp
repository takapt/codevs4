#include "GameCommon.hpp"
#include "Common.hpp"


int rev_dir(int dir)
{
    return (dir + 2) % 4;
}
Dir rev_dir(Dir dir)
{
    return Dir(rev_dir((int)dir));
}
namespace std
{
    ostream& operator<<(ostream& os, Dir dir)
    {
        assert(0 <= dir && dir < 5);
        os << (dir < 4 ? STR_DIR[dir] : "INVALID");
        return os;
    }
}


char to_order(Dir dir)
{
    assert(0 <= dir && dir < 4);
    return STR_DIR[dir][0];
}
