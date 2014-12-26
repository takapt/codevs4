#include "GameCommon.hpp"
#include "Common.hpp"

namespace std
{
    ostream& operator<<(ostream& os, Dir dir)
    {
        os << STR_DIR[dir];
        return os;
    }
}
