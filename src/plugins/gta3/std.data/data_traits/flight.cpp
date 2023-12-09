/*
 * Copyright (C) 2015  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include "../data_traits.hpp"
using namespace modloader;

struct plane_traits
{
    struct dtraits : modloader::dtraits::ReadAndInterpretTrackFile
    {
        static const char* what() { return "plane path"; }
    };
};

template<uintptr_t addr> // using tracks detour type, it works, cba adding one for flight
using OpenFlightDetour = modloader::ReadAndInterpretTrackFileDetour<addr, plane_traits::dtraits>;

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
        plugin_ptr->flightdat = modloader::hash("flight.dat");
        plugin_ptr->flight2dat = modloader::hash("flight2.dat");
        plugin_ptr->flight3dat = modloader::hash("flight3.dat");
        plugin_ptr->flight4dat = modloader::hash("flight4.dat");

        plugin_ptr->modloader_re3 = (modloader_re3_t*)plugin_ptr->loader->FindSharedData("MODLOADER_RE3")->p;

        plugin_ptr->modloader_re3->callback_table->LoadFile_FlightDat = plugin_ptr->RE3Detour_LoadFile_FlightDat;
        plugin_ptr->flightdat_detour.SetFileDetour(LoadFileDetourRE3());

        plugin_ptr->modloader_re3->callback_table->LoadFile_Flight2Dat = plugin_ptr->RE3Detour_LoadFile_Flight2Dat;
        plugin_ptr->flight2dat_detour.SetFileDetour(LoadFileDetourRE3());

        plugin_ptr->modloader_re3->callback_table->LoadFile_Flight3Dat = plugin_ptr->RE3Detour_LoadFile_Flight3Dat;
        plugin_ptr->flight3dat_detour.SetFileDetour(LoadFileDetourRE3());

        plugin_ptr->modloader_re3->callback_table->LoadFile_Flight4Dat = plugin_ptr->RE3Detour_LoadFile_Flight4Dat;
        plugin_ptr->flight4dat_detour.SetFileDetour(LoadFileDetourRE3());
    }
    else if(gvm.IsVC() || gvm.IsIII())
    {
        plugin_ptr->AddDetour("flight.dat", no_reinstall, OpenFlightDetour<xVc(0x5B220A)>());
        plugin_ptr->AddDetour("flight2.dat", no_reinstall, OpenFlightDetour<xVc(0x5B2475)>());
        plugin_ptr->AddDetour("flight3.dat", no_reinstall, OpenFlightDetour<xVc(0x5B24AE)>());
        if(gvm.IsIII()) plugin_ptr->AddDetour("flight4.dat", no_reinstall, OpenFlightDetour<xIII(0x54BB7C)>());
    }
});
