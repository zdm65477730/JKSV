#include "JKSV.hpp"

#include <switch.h>

int main()
{
    JKSV jksv{};
    while (appletMainLoop() && jksv.is_running())
    {
        jksv.update();
        jksv.render();
    }
    return 0;
}
