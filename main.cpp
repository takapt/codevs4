#include "IO.hpp"
#include "AI.hpp"

#include <unistd.h>

int main()
{
    cout << "UNK114514" << endl;
    cout.flush();

    AI ai;
    for (;;)
    {
        InputResult input_result = input();
        if (input_result.current_stage_no == -1)
            break;

        if (input_result.current_turn == 0)
            ai = AI();

        bool need_change_dir = input_result.is_2p();
        if (need_change_dir)
            input_result.change_dir();

        auto order = ai.solve(input_result);
        output(order, need_change_dir);
    }
}
