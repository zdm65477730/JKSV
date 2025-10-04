#include "JKSV.hpp"
#include "cmdargs.hpp"

#include <switch.h>

int main(int argc, const char *argv[])
{
    // Store the pointers here so JKSV can update itself from where ever the user might have placed it.
    cmdargs::store(argc, argv);

    JKSV jksv{};
    while (jksv.is_running())
    {
        jksv.update();
        jksv.render();
    }
    return 0;
}
