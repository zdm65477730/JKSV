#include "JKSV.hpp"
#include "config.hpp"
#include <switch.h>

int main(void)
{
    JKSV jksv{};
    while (appletMainLoop() && jksv.isRunning())
    {
        jksv.update();
        jksv.render();
    }
    config::save();
    return 0;
}
