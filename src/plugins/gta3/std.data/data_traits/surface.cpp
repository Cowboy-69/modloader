/*
 * Copyright (C) 2015  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include "../data_traits.hpp"
using namespace modloader;

struct surface_traits : public data_traits
{
    static const bool has_sections      = false;
    static const bool per_line_section  = false;

    struct dtraits : modloader::dtraits::SaOpenOr3VcLoadFileDetour
    {
        static const char* what() { return "surface adhesion limits"; }
    };
    
    using detour_type = modloader::SaOpenOr3VcLoadFileDetour<0x55D100, dtraits>;

    // The first string from the line doesn't matter, it's just a helper to the human reader.
    // The following values are real numbers, '---' like values or nothing
    using key_type      = int;
    using value_type    = data_slice<dummy_string, std::list<either<real_t, std::string>>>; // (XXX list because of a move issue with either)

    key_type key_from_value(const value_type&)
    {
        return adhesion_line++;
    }

public:
    int adhesion_line = 0;

    template<class Archive>
    void serialize(Archive& archive)
    { archive(this->adhesion_line); }
};

using surface_store = gta3::data_store<surface_traits, std::map<
                        surface_traits::key_type, surface_traits::value_type
                        >>;

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

using LoadFileDetourRE = modloader::basic_file_detour<dtraits::SaOpenOr3VcLoadFileDetour,
    LoadFileSB,
    int, const char*, uint8_t*, int, const char*>;

static auto xinit = initializer([](DataPlugin* plugin_ptr)
{
    if (plugin_ptr->loader->game_id == MODLOADER_GAME_RE3)
    {
        plugin_ptr->surfacedat = modloader::hash("surface.dat");

        plugin_ptr->modloader_re3 = (modloader_re3_t*)plugin_ptr->loader->FindSharedData("MODLOADER_RE3")->p;
        plugin_ptr->modloader_re3->callback_table->LoadFile_SurfaceDat = plugin_ptr->RE3Detour_LoadFile_SurfaceDat;
        plugin_ptr->surfacedat_detour.SetFileDetour(LoadFileDetourRE());
    }
    else if (plugin_ptr->loader->game_id == MODLOADER_GAME_REVC)
    {
        plugin_ptr->surfacedat = modloader::hash("surface.dat");

        plugin_ptr->modloader_reVC = (modloader_reVC_t*)plugin_ptr->loader->FindSharedData("MODLOADER_REVC")->p;
        plugin_ptr->modloader_reVC->callback_table->LoadFile_SurfaceDat = plugin_ptr->REVCDetour_LoadFile_SurfaceDat;
        plugin_ptr->surfacedat_detour.SetFileDetour(LoadFileDetourRE());
    }
    else if(gvm.IsSA())
    {
        auto ReloadSurfaceInfo = std::bind(injector::thiscall<void(void*)>::call<0x55F420>, mem_ptr(0xB79538).get<void>());
        plugin_ptr->AddMerger<surface_store>("surface.dat", true, false, false, reinstall_since_start, gdir_refresh(ReloadSurfaceInfo));
    }
    else if(gvm.IsIII() || gvm.IsVC())
    {
        auto ReloadSurfaceTable = std::bind(injector::cstd<void(const char*)>::call<xVc(0x4CE8A0)>, "data/surface.dat");
        plugin_ptr->AddMerger<surface_store>("surface.dat", true, false, false, reinstall_since_start, gdir_refresh(ReloadSurfaceTable));
    }
});
