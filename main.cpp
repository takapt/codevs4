#include "IO.hpp"
#include "AI.hpp"

#include <unistd.h>

int main()
{
    cout << "UNKO" << endl;
    cout.flush();

    AI ai;
    for (;;)
    {
        InputResult input_result = input();
        if (input_result.current_stage_no == -1)
            break;

        if (input_result.current_turn == 0)
            ai = AI();

        auto order = ai.solve(input_result);
        output(order);
    }
}
