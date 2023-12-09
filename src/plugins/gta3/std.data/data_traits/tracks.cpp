/*
 * Copyright (C) 2015  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include "../data_traits.hpp"
using namespace modloader;

struct tracks_traits
{
    struct dtraits : modloader::dtraits::ReadAndInterpretTrackFile
    {
        static const char* what() { return "train track"; }
    };
};

template<uintptr_t addr>
using OpenTracksDetour = modloader::ReadAndInterpretTrackFileDetour<addr, tracks_traits::dtraits>;

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
        plugin_ptr->tracksdat = modloader::hash("tracks.dat");
        plugin_ptr->tracks2dat = modloader::hash("tracks2.dat");

        plugin_ptr->modloader_re3 = (modloader_re3_t*)plugin_ptr->loader->FindSharedData("MODLOADER_RE3")->p;

        plugin_ptr->modloader_re3->callback_table->LoadFile_TracksDat = plugin_ptr->RE3Detour_LoadFile_TracksDat;
        plugin_ptr->tracksdat_detour.SetFileDetour(LoadFileDetourRE3());

        plugin_ptr->modloader_re3->callback_table->LoadFile_Tracks2Dat = plugin_ptr->RE3Detour_LoadFile_Tracks2Dat;
        plugin_ptr->tracks2dat_detour.SetFileDetour(LoadFileDetourRE3());
    }
    else if(gvm.IsSA() || gvm.IsIII())
    {
        //auto ReloadTracks = injector::cstd<void()>::call<0x6F7440>;
        plugin_ptr->AddDetour("tracks.dat", no_reinstall, OpenTracksDetour<0x6F7470>());
        plugin_ptr->AddDetour("tracks2.dat", no_reinstall, OpenTracksDetour<0x6F74BC>());
        if(gvm.IsSA()) plugin_ptr->AddDetour("tracks3.dat", no_reinstall, OpenTracksDetour<0x6F7496>());
        if(gvm.IsSA()) plugin_ptr->AddDetour("tracks4.dat", no_reinstall, OpenTracksDetour<0x6F74E2>());
    }
});
