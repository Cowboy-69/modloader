/*
 * Copyright (C) 2015  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include "../data_traits.hpp"
using namespace modloader;
using std::set;
using std::string;

enum class PedRelationship : uint8_t
{
    None,
    Hate, Dislike, Like, Respect,  // SA
    Threat, Avoid,                 // III/VC
};

template<>
struct enum_map<PedRelationship>
{
    static std::map<string, PedRelationship>& map()
    {
        static std::map<string, PedRelationship> xmap = {
            { "Hate", PedRelationship::Hate }, 
            { "Like", PedRelationship::Like }, 
            { "Dislike", PedRelationship::Dislike }, 
            { "Respect", PedRelationship::Respect }, 
            { "Threat", PedRelationship::Threat },
            { "Avoid", PedRelationship::Avoid },
        };
        return xmap;
    }
};


//
struct ped_traits : public data_traits
{
    static const bool has_sections      = false;
    static const bool per_line_section  = false;

    struct dtraits : modloader::dtraits::SaOpenOr3VcLoadFileDetour
    {
        static const char* what() { return "ped relationship data"; }
    };
    
    using detour_type = modloader::SaOpenOr3VcLoadFileDetour<0x608B45, dtraits>;

    using key_type      = std::pair<size_t, PedRelationship>;
    using value_type    = data_slice<either< std::tuple<PedRelationship, set<string>>, std::tuple<string, VC3Only<pack<real_t, 5>>> >>;

    // key_from_value
    struct key_from_value_visitor : gta3::data_section_visitor<key_type>
    {
        ped_traits& traits;
        key_from_value_visitor(ped_traits& traits) : traits(traits) {}

        key_type operator()(const std::tuple<PedRelationship, set<string>>& tuple) const
        {
            return key_type(traits.pedtype, get<0>(tuple));
        }

        key_type operator()(const std::tuple<string, VC3Only<pack<real_t, 5>>>& tuple) const
        {
            traits.pedtype = modloader::hash(get<0>(tuple));
            return key_type(traits.pedtype, PedRelationship::None); 
        }

        key_type operator()(const either_blank& slice) const
        { throw std::invalid_argument("blank type"); }
    };

    key_type key_from_value(const value_type& value)
    {
        return get<0>(value).apply_visitor(key_from_value_visitor(*this));
    }

    public:

        size_t pedtype = 0;   // Working ped type hash

        template<class Archive>
        void serialize(Archive& archive)
        { archive(this->pedtype); }
};

using ped_store = gta3::data_store<ped_traits, std::map<
                        ped_traits::key_type, ped_traits::value_type
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

using LoadFileDetourRE3 = modloader::basic_file_detour<dtraits::SaOpenOr3VcLoadFileDetour,
    LoadFileSB,
    int, const char*, uint8_t*, int, const char*>;

static auto xinit = initializer([](DataPlugin* plugin_ptr)
{
    if (plugin_ptr->loader->game_id == MODLOADER_GAME_RE3) {
        plugin_ptr->peddat = modloader::hash("ped.dat");

        plugin_ptr->modloader_re3 = (modloader_re3_t*)plugin_ptr->loader->FindSharedData("MODLOADER_RE3")->p;
        plugin_ptr->modloader_re3->callback_table->LoadFile_PedDat = plugin_ptr->RE3Detour_LoadFile_PedDat;
        plugin_ptr->peddat_detour.SetFileDetour(LoadFileDetourRE3());
    } else {
        auto ReloadPedRelationship = [] {}; // refreshing ped relationship during gameplay might break save game, don't do it at all
        plugin_ptr->AddMerger<ped_store>("ped.dat", true, false, false, reinstall_since_load, gdir_refresh(ReloadPedRelationship));
    }
});
