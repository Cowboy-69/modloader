/*
 * Copyright (C) 2013-2014  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 *  std.scm -- Standard SCM Loader Plugin for Mod Loader
 *
 */
#include <stdinc.hpp>
using namespace modloader;

/*
 *  The plugin object
 */
class ScmPlugin : public modloader::basic_plugin
{
    private:
        // re3 stuff
        uint32_t mainscm; // Hash for main.scm
        //file_overrider mainscm_detour;
        modloader_re3_t* modloader_re3{};
        modloader_reVC_t* modloader_reVC{};
        static int32_t RE3Detour_OpenFile_MainScm(const char*, const char*);
        static int32_t REVCDetour_OpenFile_MainScm(const char*, const char*);

        file_overrider overrider;

    public:
        const info& GetInfo();
        bool OnStartup();
        bool OnShutdown();
        int GetBehaviour(modloader::file&);
        bool InstallFile(const modloader::file&);
        bool ReinstallFile(const modloader::file&);
        bool UninstallFile(const modloader::file&);
        
} scm_plugin;

REGISTER_ML_PLUGIN(::scm_plugin);

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

int32_t ScmPlugin::RE3Detour_OpenFile_MainScm(const char* filename, const char* mode)
{
    const auto& modloader_re3 = *scm_plugin.modloader_re3;
    const auto OpenFile0 = modloader_re3.re3_addr_table->CFileMgr_OpenFile;

    auto& base_detour = scm_plugin.overrider;
    if (base_detour.NumInjections() == 1)
    {
        const auto& openFileDetour = static_cast<OpenFileSB&>(base_detour.GetInjection(0));
        if (openFileDetour.has_hooked())
            return openFileDetour.functor(OpenFile0, filename, mode);
    }

    return OpenFile0(filename, mode);
}

int32_t ScmPlugin::REVCDetour_OpenFile_MainScm(const char* filename, const char* mode)
{
    const auto& modloader_reVC = *scm_plugin.modloader_reVC;
    const auto OpenFile0 = modloader_reVC.reVC_addr_table->CFileMgr_OpenFile;

    auto& base_detour = scm_plugin.overrider;
    if (base_detour.NumInjections() == 1)
    {
        const auto& openFileDetour = static_cast<OpenFileSB&>(base_detour.GetInjection(0));
        if (openFileDetour.has_hooked())
            return openFileDetour.functor(OpenFile0, filename, mode);
    }

    return OpenFile0(filename, mode);
}

/*
 *  ScmPlugin::GetInfo
 *      Returns information about this plugin 
 */
const ScmPlugin::info& ScmPlugin::GetInfo()
{
    static const char* extable[] = { "scm", 0 };
    static const info xinfo      = { "std.scm", get_version_by_date(), "LINK/2012", -1, extable };
    return xinfo;
}





/*
 *  ScmPlugin::OnStartup
 *      Startups the plugin
 */
bool ScmPlugin::OnStartup()
{
    if (loader->game_id == MODLOADER_GAME_RE3)
    {
        mainscm = modloader::hash("main.scm");

        modloader_re3 = (modloader_re3_t*)loader->FindSharedData("MODLOADER_RE3")->p;
        modloader_re3->callback_table->OpenFile_MainScm = RE3Detour_OpenFile_MainScm;
        this->overrider.SetParams(file_overrider::params(true, true, true, true));
        this->overrider.SetFileDetour(OpenFileDetourRE());
        return true;
    }
    else if (loader->game_id == MODLOADER_GAME_REVC)
    {
        mainscm = modloader::hash("main.scm");

        modloader_reVC = (modloader_reVC_t*)loader->FindSharedData("MODLOADER_REVC")->p;
        modloader_reVC->callback_table->OpenFile_MainScm = REVCDetour_OpenFile_MainScm;
        this->overrider.SetParams(file_overrider::params(true, true, true, true));
        this->overrider.SetFileDetour(OpenFileDetourRE());
        return true;
    }
    else if(gvm.IsIII() || gvm.IsVC() || gvm.IsSA())
    {
        this->overrider.SetParams(file_overrider::params(true, true, true, true));
        this->overrider.SetFileDetour(OpenFileDetour<0x468EC9>(), OpenFileDetour<0x489A4A>());
        return true;
    }
    return false;
}

/*
 *  ScmPlugin::OnShutdown
 *      Shutdowns the plugin
 */
bool ScmPlugin::OnShutdown()
{
    return true;
}


/*
 *  ScmPlugin::GetBehaviour
 *      Gets the relationship between this plugin and the file
 */
int ScmPlugin::GetBehaviour(modloader::file& file)
{
    static const auto main_scm = modloader::hash("main.scm");
    if(!file.is_dir() && file.hash == main_scm)
    {
        file.behaviour = file.hash;
        return MODLOADER_BEHAVIOUR_YES;
    }
    return MODLOADER_BEHAVIOUR_NO;
}

/*
 *  ScmPlugin::InstallFile
 *      Installs a file using this plugin
 */
bool ScmPlugin::InstallFile(const modloader::file& file)
{
    return overrider.InstallFile(file);
}

/*
 *  ScmPlugin::ReinstallFile
 *      Reinstall a file previosly installed that has been updated
 */
bool ScmPlugin::ReinstallFile(const modloader::file& file)
{
    return overrider.ReinstallFile();
}

/*
 *  ScmPlugin::UninstallFile
 *      Uninstall a previosly installed file
 */
bool ScmPlugin::UninstallFile(const modloader::file& file)
{
    return overrider.UninstallFile();
}
