/*
 * Copyright (C) 2015  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include "../data_traits.hpp"
using namespace modloader;

struct timecyc_traits
{
    struct dtraits : modloader::dtraits::OpenFile
    {
        static const char* what() { return "time cycle properties"; }
    };
};

using OpenTimecycDetourSA = modloader::OpenFileDetour<0x5BBADE, timecyc_traits::dtraits>;
using OpenTimecycDetour3VC = modloader::LoadFileDetour<xVc(0x4D0614), timecyc_traits::dtraits>;

class LoadFileSB : public injector::scoped_base
{
public:
    using func_type = std::function<int(const char*, uint8_t*, int, const char*)>;
    using functor_type = std::function<int(func_type, const char*&, uint8_t*&, int&, const char*&)>;

    functor_type functor;

    LoadFileSB() = default;
    LoadFileSB(const LoadFileSB&) = delete;
    LoadFileSB(LoadFileSB&& rhs) : functor(std::move(rhs.functor)) {}
    LoadFileSB& operator=(const LoadFileSB&) = delete;
    LoadFileSB& operator=(LoadFileSB&& rhs) { functor = std::move(rhs.functor); }

    virtual ~LoadFileSB()
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

using LoadFileDetourRE3 = modloader::basic_file_detour<dtraits::SaOpenOr3VcLoadFileDetour,
    LoadFileSB,
    int, const char*, uint8_t*, int, const char*>;

static auto xinit = initializer([](DataPlugin* plugin_ptr)
{
    if (plugin_ptr->loader->game_id == MODLOADER_GAME_RE3) {
        plugin_ptr->timecycdat = modloader::hash("timecyc.dat");

        plugin_ptr->modloader_re3 = (modloader_re3_t*)plugin_ptr->loader->FindSharedData("MODLOADER_RE3")->p;
        plugin_ptr->modloader_re3->callback_table->LoadFile_TimecycDat = plugin_ptr->RE3Detour_LoadFile_TimecycDat;
        plugin_ptr->timecycdat_detour.SetFileDetour(LoadFileDetourRE3());
    }
    else
    {
        auto ReloadTimeCycle = []
        {
            injector::cstd<void()>::call<0x5BBAC0>();   // CTimeCycle::Initialise
            // XXX there are some vars at the end of that function body which should not be reseted while the game runs...
            // doesn't cause serious issues but well... shall we take care of them?
        };

        if(gvm.IsIII() || gvm.IsVC())
        {
            plugin_ptr->AddDetour("timecyc.dat", reinstall_since_start, OpenTimecycDetour3VC(), gdir_refresh(ReloadTimeCycle));
        }
        else if(gvm.IsSA())
        {
            auto& timecyc_ov = plugin_ptr->AddDetour("timecyc.dat", reinstall_since_start, OpenTimecycDetourSA(), gdir_refresh(ReloadTimeCycle));
            if(GetModuleHandleA("samp"))
            {
                //
                // SAMP changes the path from any file named timecyc.dat to a custom path. This hook takes place
                // at kernel32:CreateFileA, so the only way to fix it is telling SAMP to load the timecyc using a different filename.
                //
                // So, the approach used here to get around this issue is, when the game is running under SAMP copy the
                // custom timecyc (which is somewhere at the modloader directory) into the gta3.std.data cache with a different extension and voilá. 
                //
                auto& timecyc_detour = static_cast<OpenTimecycDetourSA&>(timecyc_ov.GetInjection(0));
                timecyc_detour.OnPosTransform([plugin_ptr](std::string file) -> std::string
                {
                    if(file.size())
                    {
                        std::string fullpath = get<2>(plugin_ptr->cache.AddCacheFile("timecyc.samp", true));

                        if(!CopyFileA(
                            std::string(plugin_ptr->loader->gamepath).append(file).c_str(),
                            (fullpath).c_str(),
                            FALSE))
                            plugin_ptr->Log("Warning: Failed to make timecyc for SAMP.");
                        else
                            file = std::move(fullpath);
                    }
                    return file;
                });
            }
        }
    }
});



