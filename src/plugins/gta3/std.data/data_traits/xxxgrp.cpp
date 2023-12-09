/*
 * Copyright (C) 2015  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include "../data_traits.hpp"
using namespace modloader;
using std::set;
using std::vector;
using std::string;

namespace datalib
{
    template<>
    struct data_info<udata<modelname>> : data_info<modelname>
    {
        static const bool ignore = true;
    };
}


// Traits for pedgrp.dat and cargrp.dat
struct xxxgrp_traits : public data_traits
{
    static const bool has_sections      = false;
    static const bool per_line_section  = false;
           
    //
    using key_type   = std::pair<int, size_t>; // grpindex, model_hash
    using value_type = data_slice<either<vector<modelname>, udata<modelname>>>;

    key_type key_from_value(const value_type&)
    {
        return std::make_pair(grpindex++, 0);
    }

    template<typename StoreType, typename TData>
    static bool setbyline(StoreType& store, TData& data, const gta3::section_info* section, const std::string& line)
    {
        if(gvm.IsVC())
        {
            // VC uses '//' at the end of the line.
            // The game doesn't even reach the point of reading this char (it only reads 16 tokens),
            // but our method to avoid it is erasing.
            size_t comment_pos = line.find("//");
            if(comment_pos != line.npos)
            {
                std::string fixed_line = line;
                fixed_line.erase(comment_pos);
                return data_traits::setbyline(store, data, section, fixed_line);
            }
        }
        return data_traits::setbyline(store, data, section, line);
    }

    // (SA-only, III/VC grps aren't variable length)
    // Before the merging process transform the set of models into individual models in each key of the container
    // This allows merging of individual entries in a group not just the entire group container itself
    template<class StoreType>
    static bool premerge(StoreType& store)
    {
        if(gvm.IsSA())
        {
            StoreType::container_type newcontainer;

            // Builds the new container, which contains a single model instead of the set of models
            std::for_each(store.container().begin(), store.container().end(), [&](StoreType::pair_type& pair)
            {
                auto& vec = *get<std::vector<modelname>>(&pair.second.get<0>());
                for(auto& model : vec)
                {
                    // the key will make the vector unique, no need to do a find.
                    newcontainer.emplace(
                        key_type(pair.first.first, hash_model(model)),
                        value_type(make_udata<modelname>(model)));
                }
            });

            store.container() = std::move(newcontainer);
        }
        return true;
    }

    // (SA-only, III/VC grps aren't variable length)
    // Now, before writing the content to the merged file we should reverse the transformation we did in premerge.
    // So this time we should take each model with the same group in the key and put in a set
    template<class StoreType, class MergedList, class FuncDoWrite>
    static bool prewrite(MergedList list, FuncDoWrite dowrite)
    {
        std::map<key_type, value_type> grp_content; // must be outside the if (because of std::ref)
        if(gvm.IsSA())
        {
            std::for_each(list.begin(), list.end(), [&](MergedList::value_type& pair)
            {
                auto& model = get(*get<udata<modelname>>(&pair.second.get().get<0>()));
                auto key = key_type(pair.first.get().first, 0);

                // If the key still doesn't exist, make it to be have it's mapped type to be a set of models
                if(grp_content.count(key) == 0)
                {
                    grp_content.emplace(key, value_type(vector<modelname>()));
                }

                get<vector<modelname>>(&grp_content[key].get<0>())->emplace_back(model);
            });

            list.clear();
            for(auto& x : grp_content)
               list.emplace_back(std::cref(x.first), std::ref(x.second));
        }
        return dowrite(list);
    }

    public: // traits data

        int grpindex = 0;   // Line index we are going tho for groups

        template<class Archive>
        void serialize(Archive& archive)
        { archive(this->grpindex); }
};

struct cargrp_traits : public xxxgrp_traits
{
    struct dtraits : modloader::dtraits::OpenFile
    {
        static const char* what()       { return "car groups"; }
        static const char* datafile()   { return "cargrp.dat"; }
    };
    
    using detour_type = modloader::OpenFileDetour<0x5BD1BB, dtraits>;
};

struct pedgrp_traits : public xxxgrp_traits
{
    struct dtraits : modloader::dtraits::OpenFile
    {
        static const char* what()       { return "ped groups"; }
        static const char* datafile()   { return "pedgrp.dat"; }
    };
    
    using detour_type = modloader::OpenFileDetour<0x5BCFFB, dtraits>;
};


template<class Traits>
using xxxgrp_store = gta3::data_store<Traits, std::map<
                        typename Traits::key_type, typename Traits::value_type
                        >>;

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

using OpenFileDetourRE3 = modloader::basic_file_detour<dtraits::OpenFile,
    OpenFileSB,
    int32_t, const char*, const char*>;

template<class Traits>
static void initialise(DataPlugin* plugin_ptr, std::function<void()> refresher)
{
    using store_type = xxxgrp_store<Traits>;

    if (plugin_ptr->loader->game_id == MODLOADER_GAME_RE3) {
        plugin_ptr->pedgrpdat = modloader::hash("pedgrp.dat");

        plugin_ptr->modloader_re3 = (modloader_re3_t*)plugin_ptr->loader->FindSharedData("MODLOADER_RE3")->p;
        plugin_ptr->modloader_re3->callback_table->OpenFile_PedGrpDat = plugin_ptr->RE3Detour_OpenFile_PedGrpDat;
        plugin_ptr->pedgrpdat_detour.SetFileDetour(OpenFileDetourRE3());
    } else {
        plugin_ptr->AddMerger<store_type>(Traits::dtraits::datafile(), true, false, false, reinstall_since_load, refresher);
    }
}

static auto xinit = initializer([](DataPlugin* plugin_ptr)
{
    if(true)       initialise<pedgrp_traits>(plugin_ptr, gdir_refresh(injector::cstd<void()>::call<0x5BCFE0>));
    if(gvm.IsSA()) initialise<cargrp_traits>(plugin_ptr, gdir_refresh(injector::cstd<void()>::call<0x5BD1A0>));
});
