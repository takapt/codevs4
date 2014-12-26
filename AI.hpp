#ifndef INCLUDE_AI
#define INCLUDE_AI

#include "Common.hpp"
#include "IO.hpp"
#include "Board.hpp"

class AI
{
public:
    AI();

    map<int, char> solve(const InputResult& input);

private:
    Board<bool> known;
};

#endif
