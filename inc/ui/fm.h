#pragma once

namespace ui
{
    void fmInit();
    void fmExit();
    void fmPrep(FsSaveDataType _type, const std::string &_dev, const std::string &_baseSDMC, bool _commit);
    void fmUpdate();
    void fmDraw(SDL_Texture *target);
} // namespace ui
