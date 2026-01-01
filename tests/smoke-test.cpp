#include <cmath>

int main()
{
    const double input = 0.5;
    const double expected = 0.25;
    const double actual = input * input;

    return std::abs(actual - expected) < 1.0e-12 ? 0 : 1;
}
