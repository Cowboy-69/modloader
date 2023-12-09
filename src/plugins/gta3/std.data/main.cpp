/*
 * Copyright (C) 2014  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include <unicode.hpp>
#include "data_traits.hpp"
using namespace modloader;


//
// TIP! TIP! TIP! TIP! TIP!! TIP! TIP! TIP!!!!!!!!!
// ------------------------------------------------------
//
// If you are working with Mod Loader's or this plugin's code you may truly want to exclude as many data_traits as you can from the build.
// Otherwise you'll get really high compilation and linking times.
//
// To do so in Visual Studio, select as many (or every) cpp source file in the data_traits/ directory at the solution explorer,
// then right-click and go to Properties, then General and mark "Yes" on "Exclude From Build".
//


void LazyGtaDatPatch();


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

DataPlugin dataplugin;
REGISTER_ML_PLUGIN(::dataplugin);

CEREAL_REGISTER_RTTI(void); // for DataPlugin::line_data_base


/*
 *  DataPlugin::GetInfo
 *      Returns information about this plugin 
 */
const DataPlugin::info& DataPlugin::GetInfo()
{
    static const char* extable[] = { "dat", "cfg", "ide", "ipl", "zon", "ped", "grp", "txt", 0 };
    static const info xinfo      = { "std.data", get_version_by_date(), "LINK/2012", -1, extable };
    return xinfo;
}



int32_t DataPlugin::RE3Detour_OpenFile_GtaDat(const char* filename, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto OpenFile0 = modloader_re3.re3_addr_table->CFileMgr_OpenFile;

    auto& base_detour = dataplugin.gtadat_detour;
    if(base_detour.NumInjections() == 1)
    {
        const auto& openFileDetour = static_cast<OpenFileSB&>(base_detour.GetInjection(0));
        if (openFileDetour.has_hooked())
            return openFileDetour.functor(OpenFile0, filename, mode);
    }

    return OpenFile0(filename, mode);
}

int32_t DataPlugin::RE3Detour_OpenFile_DefaultDat(const char* filename, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto OpenFile0 = modloader_re3.re3_addr_table->CFileMgr_OpenFile;

    auto& base_detour = dataplugin.defaultdat_detour;
    if(base_detour.NumInjections() == 1)
    {
        const auto& openFileDetour = static_cast<OpenFileSB&>(base_detour.GetInjection(0));
        if (openFileDetour.has_hooked())
            return openFileDetour.functor(OpenFile0, filename, mode);
    }

    return OpenFile0(filename, mode);
}

int32_t DataPlugin::RE3Detour_OpenFile_CarcolsDat(const char* filename, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto OpenFile0 = modloader_re3.re3_addr_table->CFileMgr_OpenFile;

    auto& base_detour = dataplugin.carcolsdat_detour;
    if(base_detour.NumInjections() == 1)
    {
        const auto& openFileDetour = static_cast<OpenFileSB&>(base_detour.GetInjection(0));
        if (openFileDetour.has_hooked())
            return openFileDetour.functor(OpenFile0, filename, mode);
    }

    return OpenFile0(filename, mode);
}

int32_t DataPlugin::RE3Detour_OpenFile_PedGrpDat(const char* filename, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto OpenFile0 = modloader_re3.re3_addr_table->CFileMgr_OpenFile;

    auto& base_detour = dataplugin.pedgrpdat_detour;
    if(base_detour.NumInjections() == 1)
    {
        const auto& openFileDetour = static_cast<OpenFileSB&>(base_detour.GetInjection(0));
        if (openFileDetour.has_hooked())
            return openFileDetour.functor(OpenFile0, filename, mode);
    }

    return OpenFile0(filename, mode);
}

int32_t DataPlugin::RE3Detour_OpenFile_CullzoneDat(const char* filename, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto OpenFile0 = modloader_re3.re3_addr_table->CFileMgr_OpenFile;

    auto& base_detour = dataplugin.cullzonedat_detour;
    if(base_detour.NumInjections() == 1)
    {
        const auto& openFileDetour = static_cast<OpenFileSB&>(base_detour.GetInjection(0));
        if (openFileDetour.has_hooked())
            return openFileDetour.functor(OpenFile0, filename, mode);
    }

    return OpenFile0(filename, mode);
}

int32_t DataPlugin::RE3Detour_OpenFile_WaterproDat(const char* filename, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto OpenFile0 = modloader_re3.re3_addr_table->CFileMgr_OpenFile;

    auto& base_detour = dataplugin.waterprodat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& openFileDetour = static_cast<OpenFileSB&>(base_detour.GetInjection(0));
        if (openFileDetour.has_hooked())
            return openFileDetour.functor(OpenFile0, filename, mode);
    }

    return OpenFile0(filename, mode);
}

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

int DataPlugin::RE3Detour_LoadFile_FistfiteDat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.fistfitedat_detour;
    if(base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_HandlingCfg(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.handlingcfg_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_PedDat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.peddat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_ObjectDat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    dataplugin.has_model_info = true;

    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.objectdat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_PedStatsDat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.pedstatsdat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_WeaponDat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.weapondat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_ParticleCfg(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.particlecfg_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_SurfaceDat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.surfacedat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_TimecycDat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.timecycdat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_FlightDat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.flightdat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_Flight2Dat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.flight2dat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_Flight3Dat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.flight3dat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_Flight4Dat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.flight4dat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_TracksDat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.tracksdat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

int DataPlugin::RE3Detour_LoadFile_Tracks2Dat(const char* filename, uint8_t* buf, int maxlen, const char* mode)
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto LoadFile0 = modloader_re3.re3_addr_table->CFileMgr_LoadFile;

    auto& base_detour = dataplugin.tracks2dat_detour;
    if (base_detour.NumInjections() == 1)
    {
        const auto& loadFileDetour = static_cast<LoadFileSB&>(base_detour.GetInjection(0));
        if (loadFileDetour.has_hooked())
            return loadFileDetour.functor(LoadFile0, filename, buf, maxlen, mode);
    }

    return LoadFile0(filename, buf, maxlen, mode);
}

void DataPlugin::RE3Detour_InitialiseGame()
{
    const auto& modloader_re3 = *dataplugin.modloader_re3;
    const auto InitialiseGame0 = modloader_re3.re3_addr_table->InitialiseGame;
    InitialiseGame0();

    dataplugin.cache.DeleteUnusedCaches();

    if (dataplugin.changed_readme_data)
    {
        if (!dataplugin.maybe_readme.empty() || dataplugin.had_cached_readme)
            dataplugin.WriteReadmeCache();
    }
}


/*
 *  DataPlugin::OnStartup
 *      Startups the plugin
 */
bool DataPlugin::OnStartup()
{
    void* p = mem_ptr(0x748CFB).get<void>();

    if( gvm.IsIII() || gvm.IsVC() || gvm.IsSA() || loader->game_id == MODLOADER_GAME_RE3)
    {
        this->readme_magics.reserve(20);

        if (plugin_ptr->loader->game_id == MODLOADER_GAME_RE3) {
            modloader_re3 = (modloader_re3_t*)plugin_ptr->loader->FindSharedData("MODLOADER_RE3")->p;
            modloader_re3->callback_table->InitialiseGame = +[] {
                RE3Detour_InitialiseGame();
            };
        }

        // Initialise the caching
        if(!cache.Startup())
            return false;

        // Initialises all the merges and overrides (see 'data_traits/' directory for those)
        for(auto& p : initializer::list())
            p->initialise(this);

        // Installs the hooks in any case, so we have the log always logging the loading of data files
        for(auto& pair : this->ovmap)
            pair.second.InstallHook();

        // Makes default.dat/gta.dat load in a lazy way
        LazyGtaDatPatch();

        if (plugin_ptr->loader->game_id == MODLOADER_GAME_RE3) {
            // Moved to DataPlugin::RE3Detour_LoadFile_ObjectDat
        } else {
            // Hook allowing us to know when we are ready to know about the model names of the game
            using modelinfo_hook =  function_hooker<0x5B925F, void(const char*)>;
            make_static_hook<modelinfo_hook>([this](modelinfo_hook::func_type CObjectData__Initialise, const char* p)
            {
                this->has_model_info = true;
                return CObjectData__Initialise(p);
            });
        }

        if (plugin_ptr->loader->game_id == MODLOADER_GAME_RE3) {
            // Moved to DataPlugin::RE3Detour_InitialiseGame
        } else {
            // Hook after the loading screen to write a readme cache
            using initialise_hook = injector::function_hooker<0x748CFB, void()>;
            make_static_hook<initialise_hook>([this](initialise_hook::func_type InitialiseGame)
            {
                InitialiseGame();
                if(this->changed_readme_data)
                {
                    // If we have a empty list of readme data and we previosly had a cached readme, overwrite it with empty data
                    // In the case the list is not empty, overwrite with new data
                    if(!this->maybe_readme.empty() || this->had_cached_readme)
                        this->WriteReadmeCache();
                }
            });
        }

        // When there's no cache present mark changed_readme_data as true because we'll need to generate a cache
        this->had_cached_readme   = IsPathA(cache.GetCachePath("readme.ld").data()) != 0;
        this->changed_readme_data = !had_cached_readme;

        return true;
    }
    return false;
}

/*
 *  DataPlugin::OnShutdown
 *      Shutdowns the plugin
 */
bool DataPlugin::OnShutdown()
{
    cache.Shutdown();
    return true;
}


/*
 *  DataPlugin::GetBehaviour
 *      Gets the relationship between this plugin and the file
 */
int DataPlugin::GetBehaviour(modloader::file& file)
{
    if (loader->game_id == MODLOADER_GAME_RE3) {
        if(!file.is_dir())
        {
            if(file.hash == gtadat)
            {
                file.behaviour = gtadat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if(file.hash == defaultdat)
            {
                file.behaviour = defaultdat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == carcolsdat)
            {
                file.behaviour = carcolsdat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            //else if (file.hash == IDE)
            else if (file.is_ext("ide"))
            {
                /*static const files_behv_t* ide_behv = FindBehv(ide_merger_name);
                if (ide_behv) {
                    file.behaviour = ide_behv->canmerge ?
                        SetType(modloader::hash(file.filepath()), ide_behv->index) :
                        SetType(file.hash, ide_behv->index);
                }*/

                //file.behaviour = IDE;
                //return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.is_ext("ipl"))
            {
                //file.behaviour = IPL;
                //return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == pedgrpdat)
            {
                file.behaviour = pedgrpdat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == cullzonedat)
            {
                file.behaviour = cullzonedat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == fistfitedat)
            {
                file.behaviour = fistfitedat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == handlingcfg)
            {
                file.behaviour = handlingcfg;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == peddat)
            {
                file.behaviour = peddat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == objectdat)
            {
                file.behaviour = objectdat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == pedstatsdat)
            {
                file.behaviour = pedstatsdat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == weapondat)
            {
                file.behaviour = weapondat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == particlecfg)
            {
                file.behaviour = particlecfg;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == surfacedat)
            {
                file.behaviour = surfacedat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == timecycdat)
            {
                file.behaviour = timecycdat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == waterprodat)
            {
                file.behaviour = waterprodat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == flightdat)
            {
                file.behaviour = flightdat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == flight2dat)
            {
                file.behaviour = flight2dat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == flight3dat)
            {
                file.behaviour = flight3dat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == flight4dat)
            {
                file.behaviour = flight4dat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == tracksdat)
            {
                file.behaviour = tracksdat;
                return MODLOADER_BEHAVIOUR_YES;
            }
            else if (file.hash == tracks2dat)
            {
                file.behaviour = tracks2dat;
                return MODLOADER_BEHAVIOUR_YES;
            }
        }
    }

    static const files_behv_t* ipl_behv = FindBehv(ipl_merger_name);
    static const files_behv_t* ide_behv = FindBehv(ide_merger_name);
    static const files_behv_t* decision_behv = FindBehv(decision_merger_hash);

    // Setups the behaviour of a file based on the specified behv object (which can be null for none)
    // Each specific behv should have a unique identifier, for mergable files the filepath is used to identify
    // the file (as many files of the same name can come at us) otherwise the filename is used as an identifier.
    auto setup_behaviour = [](modloader::file& file, const files_behv_t* behv)
    {
        if(behv)
        {
            file.behaviour = behv->canmerge?
                SetType(modloader::hash(file.filepath()), behv->index) :
                SetType(file.hash, behv->index);    // filename hash
            return true;
        }
        return false;
    };

    if(!file.is_dir())
    {
        if(file.is_ext("txt"))
        {
            return MODLOADER_BEHAVIOUR_CALLME;
        }
        else if(file.is_ext("ide"))
        {
            if(setup_behaviour(file, ide_behv))
                return MODLOADER_BEHAVIOUR_YES;
        }
        else if(file.is_ext("ipl") || file.is_ext("zon"))
        {
             if(gvm.IsSA() && file.is_ext("ipl"))
             {
                // Make sure this is not binary IPL by reading the file magic
                char buf[4];
                if(FILE* f = fopen(file.fullpath().c_str(), "rb"))
                {
                    if(fread(buf, 4, 1, f) && !memcmp(buf, "bnry", 4))
                    {
                        fclose(f);
                        return MODLOADER_BEHAVIOUR_NO;
                    }
                    fclose(f);
                }
            }

            if(setup_behaviour(file, ipl_behv))
                return MODLOADER_BEHAVIOUR_YES;
        }
        else if(file.is_ext("ped") || file.is_ext("grp"))
        {
            // must be in a decision/allowed/ directory
            static auto regex = make_regex(R"___(^(?:.*[\\/])?decision[\\/]allowed[\\/]\w+\.(?:ped|grp)$)___", 
                                            sregex::ECMAScript|sregex::optimize|sregex::icase);

            if(regex_match(std::string(file.filedir()), regex))
            {
                if(setup_behaviour(file, decision_behv))
                    return MODLOADER_BEHAVIOUR_YES;
            }
        }
        else if(setup_behaviour(file, FindBehv(file)))
            return MODLOADER_BEHAVIOUR_YES;
    }

    return MODLOADER_BEHAVIOUR_NO;
}

/*
 *  DataPlugin::InstallFile
 *      Installs a file using this plugin
 */
bool DataPlugin::InstallFile(const modloader::file& file)
{
    if (loader->game_id == MODLOADER_GAME_RE3) {
        if (file.behaviour == gtadat)
        {
            return gtadat_detour.InstallFile(file);
        }
        else if (file.behaviour == defaultdat)
        {
            return defaultdat_detour.InstallFile(file);
        }
        else if (file.behaviour == carcolsdat)
        {
            return carcolsdat_detour.InstallFile(file);
        }
        else if (file.is_ext("ide"))
        {
            ideFiles[file.filename()] = &file;
            return true;
        }
        else if (file.is_ext("ipl") || file.is_ext("zon"))
        {
            iplFiles[file.filename()] = &file;
            return true;
        }
        else if (file.behaviour == pedgrpdat)
        {
            return pedgrpdat_detour.InstallFile(file);
        }
        else if (file.behaviour == cullzonedat)
        {
            return cullzonedat_detour.InstallFile(file);
        }
        else if (file.behaviour == fistfitedat)
        {
            return fistfitedat_detour.InstallFile(file);
        }
        else if (file.behaviour == handlingcfg)
        {
            return handlingcfg_detour.InstallFile(file);
        }
        else if (file.behaviour == peddat)
        {
            return peddat_detour.InstallFile(file);
        }
        else if (file.behaviour == objectdat)
        {
            return objectdat_detour.InstallFile(file);
        }
        else if (file.behaviour == pedstatsdat)
        {
            return pedstatsdat_detour.InstallFile(file);
        }
        else if (file.behaviour == weapondat)
        {
            return weapondat_detour.InstallFile(file);
        }
        else if (file.behaviour == particlecfg)
        {
            return particlecfg_detour.InstallFile(file);
        }
        else if (file.behaviour == surfacedat)
        {
            return surfacedat_detour.InstallFile(file);
        }
        else if (file.behaviour == timecycdat)
        {
            return timecycdat_detour.InstallFile(file);
        }
        else if (file.behaviour == waterprodat)
        {
            return waterprodat_detour.InstallFile(file);
        }
        else if (file.behaviour == flightdat)
        {
            return flightdat_detour.InstallFile(file);
        }
        else if (file.behaviour == flight2dat)
        {
            return flight2dat_detour.InstallFile(file);
        }
        else if (file.behaviour == flight3dat)
        {
            return flight3dat_detour.InstallFile(file);
        }
        else if (file.behaviour == flight4dat)
        {
            return flight4dat_detour.InstallFile(file);
        }
        else if (file.behaviour == tracksdat)
        {
            return tracksdat_detour.InstallFile(file);
        }
        else if (file.behaviour == tracks2dat)
        {
            return tracks2dat_detour.InstallFile(file);
        }
    }

    if(file.is_ext("txt"))
    {
        this->readme_toinstall.emplace(&file, 0);
        this->readme_touninstall.erase(&file);
        return true;
    }
    else
    {
        static const files_behv_t* ipl_behv = FindBehv(ipl_merger_name);
        static const files_behv_t* ide_behv = FindBehv(ide_merger_name);
        static const files_behv_t* decision_behv = FindBehv(decision_merger_hash);

        auto type = GetType(file.behaviour);
        if((ipl_behv && type == ipl_behv->index) || (ide_behv && type == ide_behv->index))
        {
            auto hash = (type == (ipl_behv? ipl_behv->index : -1)? ipl_merger_hash : ide_merger_hash);
            return this->InstallFile(file, hash, find_gta_path(file.filedir()), file.filepath());
        }
        else if(decision_behv && type == decision_behv->index)
        {
            return this->InstallFile(file, decision_merger_hash, file.filename(), file.filepath());
        }
        else
        {
            return this->InstallFile(file, file.hash, file.filename(), file.filepath());
        }
    }
    return false;
}


/*
 *  DataPlugin::ReinstallFile
 *      Reinstall a file previosly installed that has been updated
 */
bool DataPlugin::ReinstallFile(const modloader::file& file)
{
    if (loader->game_id == MODLOADER_GAME_RE3) {
        if (file.behaviour == gtadat) {
            return gtadat_detour.ReinstallFile();
        }
        else if (file.behaviour == defaultdat)
        {
            return defaultdat_detour.ReinstallFile();
        }
        else if (file.behaviour == carcolsdat)
        {
            return carcolsdat_detour.ReinstallFile();
        }
        else if (file.is_ext("ide"))
        {
            InstallFile(file);
        }
        else if (file.is_ext("ipl") || file.is_ext("zon"))
        {
            InstallFile(file);
        }
        else if (file.behaviour == pedgrpdat)
        {
            return pedgrpdat_detour.ReinstallFile();
        }
        else if (file.behaviour == cullzonedat)
        {
            return cullzonedat_detour.ReinstallFile();
        }
        else if (file.behaviour == fistfitedat)
        {
            return pedgrpdat_detour.ReinstallFile();
        }
        else if (file.behaviour == handlingcfg)
        {
            return handlingcfg_detour.ReinstallFile();
        }
        else if (file.behaviour == peddat)
        {
            return peddat_detour.ReinstallFile();
        }
        else if (file.behaviour == objectdat)
        {
            return objectdat_detour.ReinstallFile();
        }
        else if (file.behaviour == pedstatsdat)
        {
            return pedstatsdat_detour.ReinstallFile();
        }
        else if (file.behaviour == weapondat)
        {
            return weapondat_detour.ReinstallFile();
        }
        else if (file.behaviour == particlecfg)
        {
            return particlecfg_detour.ReinstallFile();
        }
        else if (file.behaviour == surfacedat)
        {
            return surfacedat_detour.ReinstallFile();
        }
        else if (file.behaviour == timecycdat)
        {
            return timecycdat_detour.ReinstallFile();
        }
        else if (file.behaviour == waterprodat)
        {
            return waterprodat_detour.ReinstallFile();
        }
        else if (file.behaviour == flightdat)
        {
            return flightdat_detour.ReinstallFile();
        }
        else if (file.behaviour == flight2dat)
        {
            return flight2dat_detour.ReinstallFile();
        }
        else if (file.behaviour == flight3dat)
        {
            return flight3dat_detour.ReinstallFile();
        }
        else if (file.behaviour == flight4dat)
        {
            return flight4dat_detour.ReinstallFile();
        }
        else if (file.behaviour == tracksdat)
        {
            return tracksdat_detour.ReinstallFile();
        }
        else if (file.behaviour == tracks2dat)
        {
            return tracks2dat_detour.ReinstallFile();
        }
    }

    if(file.is_ext("txt"))
    {
        this->readme_touninstall.emplace(&file, 0);
        this->readme_toinstall.emplace(&file, 0);
        return true;
    }
    else
    {
        static const files_behv_t* ipl_behv = FindBehv(ipl_merger_name);
        static const files_behv_t* ide_behv = FindBehv(ide_merger_name);
        static const files_behv_t* decision_behv = FindBehv(decision_merger_hash);

        auto type = GetType(file.behaviour);
        if((ipl_behv && type == ipl_behv->index) || (ide_behv && type == ide_behv->index))
        {
            auto hash = (type == (ipl_behv? ipl_behv->index : -1)? ipl_merger_hash : ide_merger_hash);
            return this->ReinstallFile(file, hash);
        }
        else if(decision_behv && type == decision_behv->index)
        {
            return this->ReinstallFile(file, decision_merger_hash);
        }
        else
        {
            return this->ReinstallFile(file, file.hash);
        }
    }
    return false;
}

/*
 *  DataPlugin::UninstallFile
 *      Uninstall a previosly installed file
 */
bool DataPlugin::UninstallFile(const modloader::file& file)
{
    if (loader->game_id == MODLOADER_GAME_RE3) {
        if (file.behaviour == gtadat)
        {
            return gtadat_detour.UninstallFile();
        }
        else if (file.behaviour == defaultdat)
        {
            return defaultdat_detour.UninstallFile();
        }
        else if (file.behaviour == carcolsdat)
        {
            return carcolsdat_detour.UninstallFile();
        }
        else if (file.is_ext("ide"))
        {
            
        }
        else if (file.is_ext("ipl") || file.is_ext("zon"))
        {
        
        }
        else if (file.behaviour == pedgrpdat)
        {
            return pedgrpdat_detour.UninstallFile();
        }
        else if (file.behaviour == cullzonedat)
        {
            return cullzonedat_detour.UninstallFile();
        }
        else if (file.behaviour == fistfitedat)
        {
            return fistfitedat_detour.UninstallFile();
        }
        else if (file.behaviour == handlingcfg)
        {
            return handlingcfg_detour.UninstallFile();
        }
        else if (file.behaviour == peddat)
        {
            return peddat_detour.UninstallFile();
        }
        else if (file.behaviour == objectdat)
        {
            return objectdat_detour.UninstallFile();
        }
        else if (file.behaviour == pedstatsdat)
        {
            return pedstatsdat_detour.UninstallFile();
        }
        else if (file.behaviour == weapondat)
        {
            return weapondat_detour.UninstallFile();
        }
        else if (file.behaviour == particlecfg)
        {
            return particlecfg_detour.UninstallFile();
        }
        else if (file.behaviour == surfacedat)
        {
            return surfacedat_detour.UninstallFile();
        }
        else if (file.behaviour == timecycdat)
        {
            return timecycdat_detour.UninstallFile();
        }
        else if (file.behaviour == waterprodat)
        {
            return waterprodat_detour.UninstallFile();
        }
        else if (file.behaviour == flightdat)
        {
            return flightdat_detour.UninstallFile();
        }
        else if (file.behaviour == flight2dat)
        {
            return flight2dat_detour.UninstallFile();
        }
        else if (file.behaviour == flight3dat)
        {
            return flight3dat_detour.UninstallFile();
        }
        else if (file.behaviour == flight4dat)
        {
            return flight4dat_detour.UninstallFile();
        }
        else if (file.behaviour == tracksdat)
        {
            return tracksdat_detour.UninstallFile();
        }
        else if (file.behaviour == tracks2dat)
        {
            return tracks2dat_detour.UninstallFile();
        }
    }

    if(file.is_ext("txt"))
    {
        this->readme_touninstall.emplace(&file, 0);
        this->readme_toinstall.erase(&file);
        return true;
    }
    else
    {
        static const files_behv_t* ipl_behv = FindBehv(ipl_merger_name);
        static const files_behv_t* ide_behv = FindBehv(ide_merger_name);
        static const files_behv_t* decision_behv = FindBehv(decision_merger_hash);

        auto type = GetType(file.behaviour);
        if((ipl_behv && type == ipl_behv->index) || (ide_behv && type == ide_behv->index))
        {
            auto hash = (type == (ipl_behv? ipl_behv->index : -1)? ipl_merger_hash : ide_merger_hash);
            return this->UninstallFile(file, hash, find_gta_path(file.filedir()));
        }
        else if(decision_behv && type == decision_behv->index)
        {
            return this->UninstallFile(file, decision_merger_hash, file.filename());
        }
        else
        {
            return this->UninstallFile(file, file.hash, file.filename());
        }
    }
    return false;
}

/*
 *  DataPlugin::Update
 *      Updates the state of this plugin after a serie of install/uninstalls
 */
void DataPlugin::Update()
{
    plugin_ptr->Log("Updating %s state...", this->data->name);

    bool has_readme_changes = (this->readme_toinstall.size() || this->readme_touninstall.size());

    // Perform the Update of readmes before refreshing!
    if(has_readme_changes)
        this->UpdateReadmeState();

    // Refresh every overriden of multiple files right here
    // Note: Don't worry about this being called before the game evens boot up, the ov->Refresh() method takes care of it
    for(auto& ov : this->ovrefresh)
    {
        if(!ov->Refresh())
            plugin_ptr->Log("Warning: Failed to refresh some data file.");   // very useful warning indeed
    }
    this->ovrefresh.clear();

    // Free up the temporary readme_buffer that may have been allocated in ParseReadme()
    this->readme_buffer.reset();
    this->readme_buffer_utf8.clear();
    this->readme_buffer_utf8.shrink_to_fit();

    // If anything changed in the readmes state (i.e. installed, removed or reinstalled a readme)
    // then rewrite it's cache
    if(has_readme_changes)
    {
        if(this->MayWriteReadmeCache()) // write caches on Update() only if we did pass tho the loading screen
            this->WriteReadmeCache();
    }

    plugin_ptr->Log("Done updating %s state.", this->data->name);
}


///////////////////////////


/*
 *  DataPlugin::InstallFile (Effectively)
 *      Installs a file assuming it's Behv and/or Merger hash to be 'merger_hash', 'fspath' the file's path in our virtual filesystem and
 *      fullpath the actual absolute path to the file in disk.
 *
 *      The 'isreinstall' parameter is a helper (defaults to false) to allow both install and reinstall in the same function;
 *      NOTE: fspath and fullpath can safely be empty when isreinstall=true
 */
bool DataPlugin::InstallFile(const modloader::file& file, size_t merger_hash, std::string fspath, std::string fullpath, bool isreinstall)
{
    // NOTE: fspath and fullpath can safely be empty when isreinstall=true

    if(FindBehv(merger_hash)->canmerge)
    {
        auto m = FindMerger(merger_hash);
        if(m->CanInstall())
        {
            if(!isreinstall)
                fs.add_file(std::move(fspath), std::move(fullpath), &file);

            // Delay in-game installs to go through DataPlugin::Update()
            ovrefresh.emplace(m);
            return true;
        }
    }
    else
    {
        if(!isreinstall)
            return FindMerger(merger_hash)->InstallFile(file);
        else
            return FindMerger(merger_hash)->ReinstallFile();
    }
    return false;
}

/*
 *  DataPlugin::ReinstallFile (Effectively)
 *      Reinstalls a file assuming it's Behv and/or Merger hash to be 'merger_hash'
 */
bool DataPlugin::ReinstallFile(const modloader::file& file, size_t merger_hash)
{
    // Just forward the call to InstallFile. It does not need the fspath and fullpath parameter on reinstall.
    return this->InstallFile(file, merger_hash, "", "", true);
}

/*
 *  DataPlugin::UninstallFile (Effectively)
 *      Uninstalls a file assuming it's Behv and/or Merger hash to be 'merger_hash', 'fspath' the file's path in our virtual filesystem and
 *      fullpath the actual absolute path to the file in disk.
 */
bool DataPlugin::UninstallFile(const modloader::file& file, size_t merger_hash, std::string fspath)
{
    if(FindBehv(merger_hash)->canmerge)
    {
        auto m = FindMerger(merger_hash);
        if(m->CanUninstall())
        {
            // Removing it from our virtual filesystem and possibily refreshing should do it
            if(fs.rem_file(std::move(fspath), &file))
            {
                ovrefresh.emplace(m);
                return true;
            }
        }
    }
    else
    {
        return FindMerger(merger_hash)->UninstallFile();
    }
    return false;
}

/*
 *  DataPlugin::InstallReadme
 *      Makes sure the specified mergers get refreshed
 */
void DataPlugin::InstallReadme(const std::set<size_t>& mergers)
{
    for(auto merger_hash : mergers)
    {
        auto m = FindMerger(merger_hash);
        if(m && m->CanInstall())
            ovrefresh.emplace(m);
    }
}

/*
 *  DataPlugin::UninstallReadme
 *      Uninstalls a readme file which may previosly gave us some data lines
 */
void DataPlugin::UninstallReadme(const modloader::file& file)
{
    // Finds all the mergers related to the specified readme file
    // and signals the updater that they should be refreshed
    for(auto& data : this->maybe_readme[&file])
    {
        if(data.merger_hash)   // maybe has a merger associated with this data?
        {
            auto m = FindMerger(data.merger_hash.get());
            if(m && m->CanUninstall())
                ovrefresh.emplace(m);
        }
    }

    // Remove all the content related to this readme file
    this->RemoveReadmeData(file);
}

/*
 *  DataPlugin::UpdateReadmeState
 *      Installs / Uninstalls / Reinstalls pending readmes
 */
void DataPlugin::UpdateReadmeState()
{
    // First of all, uninstalls all the readmes that needs to be uninstalled
    // Do so first because it may be a reinstall, during a reinstall the file is both in the uninstall and the install list.
    for(auto& r : this->readme_touninstall)
        this->UninstallReadme(*r.first);

    if(readme_toinstall.size())
    {
        readme_listing_type cached_readme_listing = this->ReadCachedReadmeListing();
        readme_listing_type installing_listing;
        readme_data_store   cached_readme_store;
        
        // Builds the listing about the readmes going to be installed...
        installing_listing.reserve(this->readme_toinstall.size());
        std::transform(readme_toinstall.begin(), readme_toinstall.end(), std::back_inserter(installing_listing),
            [](const std::pair<const modloader::file*, int>& file) -> const modloader::file& {
                return *file.first;
        });

        if(cached_readme_listing.size())    // Do we have any cached readme?
        {
            // Yeah, we do have cached readmes!!! Read the cached readmes content, i.e. lines itself and their data
            cached_readme_store = this->ReadCachedReadmeStore();
            if(cached_readme_store.size() != cached_readme_listing.size())
            {
                plugin_ptr->Log("Warning: Cached readme listing seems to not match the cached store, something is really wrong here!");
                cached_readme_listing.clear();  // problems with the cache, so no cached readme ;)
                cached_readme_store.clear();    // ^^
            }
        }

        // Installs the pending readmes, either by parsing the readme file again or by fetching the data from the cache
        auto install_it = readme_toinstall.begin();
        for(size_t i = 0; i < readme_toinstall.size(); ++i, ++install_it)
        {
            auto& file = *install_it->first;
            auto it = std::find(cached_readme_listing.begin(), cached_readme_listing.end(), installing_listing[i]);
            if(it == cached_readme_listing.end())
            {
                this->Log("Parsing readme file \"%s\"", file.filepath());
                this->InstallReadme(ParseReadme(file));
            }
            else
            {
                auto old_state = this->changed_readme_data; // AddReadmeData changes this, but we are over cache
                this->Log("Parsing cached readme data for \"%s\"", file.filepath());
                auto index = std::distance(cached_readme_listing.begin(), it);
                this->InstallReadme(AddReadmeData(file, std::move(cached_readme_store[index])));
                this->changed_readme_data = old_state;
            }
        }
    }

    // Clear the update lists
    this->readme_touninstall.clear();
    this->readme_toinstall.clear();
}


/*
 *  DataPlugin::ParseReadme
 *      Parses the specified readme file and returns a list of mergers that are related to data found in this file
 */
std::set<size_t> DataPlugin::ParseReadme(const modloader::file& file)
{
    static const size_t max_readme_size = 60000; // ~60KB, don't increase too much to avoid reading too big files.
                                                 // Plus we may have two buffers with this size while readmes are being read.

    if(file.size <= max_readme_size)
    {
        std::ifstream stream(file.fullpath(), std::ios::binary);
        if(stream)
        {
            // Allocate buffer to work with readme files.
            // This buffer will be later freed at Update() time.
            if(readme_buffer == nullptr)
                readme_buffer.reset(new char[max_readme_size]);

            if(stream.read(&readme_buffer[0], file.size))
            {
                const char* start = &readme_buffer[0];
                const char* end   = &readme_buffer[(size_t)(file.size)];

                // If the text is not in UTF-8, convert it to UTF-8!!!!!!
                auto encoding = unicode::detect_encoding(start, end);
                if(encoding != unicode::encoding::utf8)
                {
                    // Ensure emptyness and capacity for UTF-8 intermediary buffer.
                    // This buffer will be later freed at Update() time.
                    readme_buffer_utf8.clear();
                    readme_buffer_utf8.reserve(max_readme_size);

                    // Convert from the detected encoding to UTF-8 by ignoring any invalid code point (unchecked).
                    unicode::unchecked::any_to_utf8(encoding, start, end, std::back_inserter(readme_buffer_utf8));

                    // Resetup start and end of text pointers.
                    start = (const char*)(readme_buffer_utf8.data() + 0);
                    end   = (const char*)(readme_buffer_utf8.data() + readme_buffer_utf8.size());
                }

                // Skip BOM
                if(std::distance(start, end) >= 3 && utf8::is_bom(start))
                    start = start + 3;

                return this->ParseReadme(file, std::make_pair(start, end));
            }
            else
                this->Log("Warning: Failed to read from \"%s\".", file.filepath());
        }
        else
            this->Log("Warning: Failed to open \"%s\" for reading.", file.filepath());
    }
    else
        this->Log("Ignoring text file \"%s\" because it's too big.", file.filepath());

    return std::set<size_t>();
}

/*
 *  DataPlugin::ParseReadme
 *      Parses the specified readme buffer (the 'buffer' pair represents begin and end respectively) from the readme file
 *      and returns a list of mergers that are related to data found in this file.
 *
 *      NOTE: buffer will be interpreted as ASCII (to follow GTA), so do any unicode specific handling before calling this!
 */
std::set<size_t> DataPlugin::ParseReadme(const modloader::file& file, std::pair<const char*, const char*> buffer)
{
    std::string line; line.reserve(256);
    std::set<size_t> mergers;
    size_t line_number = 0;

    this->AddDummyReadme(file); // this makes even empty readmes be cached (so it doesn't re-read it again)

    while(datalib::gta3::getline(buffer, line))
    {
        ++line_number;
        if(datalib::gta3::trim_config_line(line).size())    // remove trailing spaces, comments and replace ',' with ' '
        {
            for(auto& reader_pair : this->readers)
            {
                auto& reader = reader_pair.second;
                if(auto merger_hash = reader(file, line, line_number)) // calls one of the readme files handlers
                {
                    mergers.emplace(merger_hash.get());
                    break;
                }
            }
        }
    }

    return mergers;
}

/*
 *  DataPlugin::VerifyCachedReadme
 *      Reads the cache header and make sure it's compatible with the current build.
 *      Also fetches all the RTTI type indices possibily used by the cache so we can skip them later on.
 */
bool DataPlugin::VerifyCachedReadme(std::ifstream& ss, cereal::BinaryInputArchive& archive)
{
    decltype(this->readme_magics) magics;
    size_t magic;

    archive(magic); // magic for this translation unit in specific
    if(magic == build_identifier())
    {
        try {
            block_reader magics_block(ss);
            archive(magics);                    // magic for the other translation units related to the readmes
            if(magics == this->readme_magics)   // notice order matters
                return true;
        } catch(const cereal::Exception&) {
            // Invalid typeid serialized, so the cache is incompatible
        };
    }

    this->Log("Warning: Incompatible readme cache version, a new cache will be generated.");
    return false;
}

/*
 *  DataPlugin::ReadCachedReadmeListing
 *      Reads and outputs the readme cache file listing.
 */
auto DataPlugin::ReadCachedReadmeListing() -> readme_listing_type
{
    readme_listing_type cached_readme_listing;

    std::ifstream ss(cache.GetCachePath("readme.ld"), std::ios::binary);
    if(ss.is_open())
    {
        cereal::BinaryInputArchive archive(ss);
        if(VerifyCachedReadme(ss, archive))
        {
            block_reader listing_block(ss);
            archive(cached_readme_listing);
        }
    }
    return cached_readme_listing;
}

/*
 *  DataPlugin::ReadCachedReadmeStore
 *      Reads and outputs the data_line objects stored in the readme cache
 */
auto DataPlugin::ReadCachedReadmeStore() -> readme_data_store
{
    readme_data_store store_lines;

    std::ifstream ss(cache.GetCachePath("readme.ld"), std::ios::binary);
    if(ss.is_open())
    {
        cereal::BinaryInputArchive archive(ss);
        if(VerifyCachedReadme(ss, archive))
        {
            block_reader::skip(ss); // skip listing block
            block_reader lines_block(ss);
            archive(store_lines);
        }
    }
    return store_lines;
}

/*
 *  DataPlugin::WriteReadmeCache
 *      Writes the 'this->maybe_readme' object into a readme cache
 */
void DataPlugin::WriteReadmeCache()
{
    std::ofstream ss(cache.GetCachePath("readme.ld"), std::ios::binary);
    if(ss.is_open())
    {
        cereal::BinaryOutputArchive archive(ss);

        archive(build_identifier());

        // magics
        {
            block_writer magics_block(ss);
            archive(this->readme_magics);
        }

        // readmes listing
        {
            readme_listing_type files;
            files.reserve(maybe_readme.size());

            std::transform(maybe_readme.begin(), maybe_readme.end(), std::back_inserter(files),
                [](const maybe_readme_type::value_type& pair) -> const modloader::file&
                { return *pair.first; });

            block_writer listing_block(ss);
            archive(files);
        }

        // readme data_line stores
        {
            readme_data_store stores;
            stores.reserve(maybe_readme.size());

            // each item in 'stores' stores a list of data from lines in files
            for(auto& m : this->maybe_readme)
            {
                stores.emplace_back();
                stores.back().reserve(m.second.size());
                std::transform(m.second.begin(), m.second.end(), std::back_inserter(stores.back()), [](const line_data& line) {
                    return line.base();
                });
            }

            block_writer store_block(ss);
            archive(stores);
        }

        this->changed_readme_data = false;
        this->had_cached_readme = true;
    }
}
