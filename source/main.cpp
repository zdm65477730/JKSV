#include "JKSV.hpp"

#include <switch.h>

int main(int argc, const char *argv[])
{
    JKSV jksv{};
    while (jksv.is_running())
    {
        jksv.update();
        jksv.render();
    }
    return 0;
}
