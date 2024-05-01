/*
 * Copyright (C) 2015  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include "../data_traits.hpp"
using namespace modloader;
using std::string;
using std::tuple;

struct object_traits : public data_traits
{
    static const bool has_sections      = false;
    static const bool per_line_section  = false;

    static const bool has_eof_string = true;
    static const char* eof_string() { return "*"; }

    struct dtraits : modloader::dtraits::SaOpenOr3VcLoadFileDetour
    {
        static const char* what()       { return "object data"; }
        static const char* datafile()   { return "object.dat"; }
    };
    
    using detour_type = modloader::SaOpenOr3VcLoadFileDetour<0x5B5444, dtraits>;

    using key_type      = std::size_t;
    using value_type    = data_slice<modelname,
                                real_t, real_t, real_t, real_t, real_t, real_t, real_t, int16_t, int16_t, char,
                                SAOnly<tuple<char, char>>,
                                delimopt,
                                SAOnly<tuple<vec3, insen<string>, real_t, vec3, real_t, char, char>>>;

    key_type key_from_value(const value_type& value)
    {
        return hash_model(get<0>(value));
    }

public: // eof_string related
    bool eof = false;

    template<class Archive>
    void serialize(Archive& archive)
    { archive(this->eof); }
};

using object_store = gta3::data_store<object_traits, std::map<
                        object_traits::key_type, object_traits::value_type
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
        plugin_ptr->objectdat = modloader::hash("object.dat");

        plugin_ptr->modloader_re3 = (modloader_re3_t*)plugin_ptr->loader->FindSharedData("MODLOADER_RE3")->p;
        plugin_ptr->modloader_re3->callback_table->LoadFile_ObjectDat = plugin_ptr->RE3Detour_LoadFile_ObjectDat;
        plugin_ptr->objectdat_detour.SetFileDetour(LoadFileDetourRE());
    }
    else if (plugin_ptr->loader->game_id == MODLOADER_GAME_REVC)
    {
        plugin_ptr->objectdat = modloader::hash("object.dat");

        plugin_ptr->modloader_reVC = (modloader_reVC_t*)plugin_ptr->loader->FindSharedData("MODLOADER_REVC")->p;
        plugin_ptr->modloader_reVC->callback_table->LoadFile_ObjectDat = plugin_ptr->REVCDetour_LoadFile_ObjectDat;
        plugin_ptr->objectdat_detour.SetFileDetour(LoadFileDetourRE());
    }
    else
    {
        // XXX a perfect refresh needs to set all the CBaseModelInfo::m_wObjectInfoIndex to -1 before reloading the data file
        // and clearing all bytes from CObjectData::ms_aObjectInfo[]
        auto ReloadObjectData = std::bind(injector::cstd<void(const char*, char)>::call<0x5B5360>, "data/object.dat", 0);
        plugin_ptr->AddMerger<object_store>("object.dat", true, false, false, reinstall_since_load, gdir_refresh(ReloadObjectData));
    }
});

