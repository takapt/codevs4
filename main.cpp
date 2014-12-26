#include "IO.hpp"
#include "AI.hpp"

int main()
{
    srand(time(NULL));

    cout << "UNKO" << endl;
    cout.flush();

    AI ai;
    for (;;)
    {
        InputResult input_result = input();
        auto order = ai.solve(input_result);
        output(order);
    }
}
