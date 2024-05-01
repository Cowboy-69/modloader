/*
* Copyright (C) 2015  LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License, see LICENSE at top level directory.
*
*/
#include <stdinc.hpp>
#include "../data_traits.hpp"
using namespace modloader;

struct waterpro_traits
{
    struct dtraits : modloader::dtraits::OpenFile
    {
        static const char* what() {
            return "compiled water level";
        }
    };
};

using OpenWaterProDetour = modloader::OpenFileDetour<xVc(0x5C395A), waterpro_traits::dtraits>;

class OpenFileSB : public injector::scoped_base
{
public:
    using func_type = std::function<int32_t(const char*, const char*)>;
    using functor_type = std::function<int32_t(func_type, const char*&, const char*&)>;

    functor_type functor;

    OpenFileSB() = default;
    OpenFileSB(const OpenFileSB&) = delete;
    OpenFileSB(OpenFileSB&& rhs) : functor(std::move(rhs.functor)) {}
    OpenFileSB& operator=(const OpenFileSB&) = delete;
    OpenFileSB& operator=(OpenFileSB&& rhs) { functor = std::move(rhs.functor); }

    virtual ~OpenFileSB()
    {
        plugin_ptr->loader->Log(">>>>>>>>>>>>>dtor");
        restore();
    }

    void make_call(functor_type functor)
    {
        plugin_ptr->loader->Log(">>>>>>>>>>>>>make_call");
        this->functor = std::move(functor);
    }

    bool has_hooked() const
    {
        plugin_ptr->loader->Log(">>>>>>>>>>>>>has_hooked");
        return !!functor;
    }

    void restore() override
    {
        this->functor = nullptr;
        plugin_ptr->loader->Log(">>>>>>>>>>>>>restore");
    }
};

using OpenFileDetourRE = modloader::basic_file_detour<dtraits::OpenFile,
    OpenFileSB,
    int32_t, const char*, const char*>;

static auto xinit = initializer([](DataPlugin* plugin_ptr)
{
    if (plugin_ptr->loader->game_id == MODLOADER_GAME_RE3)
    {
        plugin_ptr->waterprodat = modloader::hash("waterpro.dat");

        plugin_ptr->modloader_re3 = (modloader_re3_t*)plugin_ptr->loader->FindSharedData("MODLOADER_RE3")->p;
        plugin_ptr->modloader_re3->callback_table->OpenFile_WaterproDat = plugin_ptr->RE3Detour_OpenFile_WaterproDat;
        plugin_ptr->waterprodat_detour.SetFileDetour(OpenFileDetourRE());
    }
    else if (plugin_ptr->loader->game_id == MODLOADER_GAME_REVC)
    {
        plugin_ptr->waterprodat = modloader::hash("waterpro.dat");

        plugin_ptr->modloader_reVC = (modloader_reVC_t*)plugin_ptr->loader->FindSharedData("MODLOADER_REVC")->p;
        plugin_ptr->modloader_reVC->callback_table->OpenFile_WaterproDat = plugin_ptr->REVCDetour_OpenFile_WaterproDat;
        plugin_ptr->waterprodat_detour.SetFileDetour(OpenFileDetourRE());
    }
    else if(gvm.IsIII() || gvm.IsVC())
    {
        plugin_ptr->AddDetour("waterpro.dat", no_reinstall, OpenWaterProDetour());
    }
});
