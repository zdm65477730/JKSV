#include "JKSV.hpp"
#include "config.hpp"
#include <switch.h>

int main(void)
{
    JKSV jksv{};
    while (appletMainLoop() && jksv.is_running())
    {
        jksv.update();
        jksv.render();
    }
    return 0;
}
