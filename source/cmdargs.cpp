#include "CmdArgs.hpp"

#include <vector>

namespace
{
    std::vector<const char *> s_args{};
}

void cmdargs::store(int argc, const char *argv[])
{
    for (int i = 0; i < argc; i++) { s_args.push_back(argv[i]); }
}

const char *cmdargs::get(int index)
{
    const int argCount = s_args.size();
    if (index < 0 || index >= argCount) { return nullptr; }

    return s_args[index];
}
