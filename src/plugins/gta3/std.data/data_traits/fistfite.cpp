/*
 * Copyright (C) 2016  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include "../data_traits.hpp"
using namespace modloader;
using std::string;
using std::tuple;

struct fistfite_traits : public data_traits
{
    static const bool has_sections      = false;
    static const bool per_line_section  = false;

    static const bool has_eof_string = true;
    static const char* eof_string() { return "ENDWEAPONDATA"; }

    struct dtraits : modloader::dtraits::SaOpenOr3VcLoadFileDetour
    {
        static const char* what()       { return "fight data"; }
        static const char* datafile()   { return "fistfite.cfg"; }
    };
    
    using detour_type = modloader::SaOpenOr3VcLoadFileDetour<xVc(0x527590), dtraits>;

    using key_type      = int;
    using value_type    = data_slice<string, pack<real_t, 4>, VCOnly<real_t>, char, string, int, int>;

    key_type key_from_value(const value_type&)
    {
        return fistfite_line++;
    }

public:
    bool eof = false;
    int fistfite_line = 0;

    template<class Archive>
    void serialize(Archive& archive)
    { archive(this->eof, this->fistfite_line); }
};

using fistfite_store = gta3::data_store<fistfite_traits, std::map<
                        fistfite_traits::key_type, fistfite_traits::value_type
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
        plugin_ptr->fistfitedat = modloader::hash("fistfite.dat");

        plugin_ptr->modloader_re3 = (modloader_re3_t*)plugin_ptr->loader->FindSharedData("MODLOADER_RE3")->p;
        plugin_ptr->modloader_re3->callback_table->LoadFile_FistfiteDat = plugin_ptr->RE3Detour_LoadFile_FistfiteDat;
        plugin_ptr->fistfitedat_detour.SetFileDetour(LoadFileDetourRE());
    }
    else if (plugin_ptr->loader->game_id == MODLOADER_GAME_REVC)
    {
        plugin_ptr->fistfitedat = modloader::hash("fistfite.dat");

        plugin_ptr->modloader_reVC = (modloader_reVC_t*)plugin_ptr->loader->FindSharedData("MODLOADER_REVC")->p;
        plugin_ptr->modloader_reVC->callback_table->LoadFile_FistfiteDat = plugin_ptr->REVCDetour_LoadFile_FistfiteDat;
        plugin_ptr->fistfitedat_detour.SetFileDetour(LoadFileDetourRE());
    }
    else if(gvm.IsIII() || gvm.IsVC())
    {
        auto ReloadFightData = injector::cstd<void()>::call<xVc(0x527570)>;
        plugin_ptr->AddMerger<fistfite_store>("fistfite.dat", true, false, false, reinstall_since_load, gdir_refresh(ReloadFightData));
    }
});

