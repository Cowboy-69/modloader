/*
 * Standard Text Loader Plugin for Mod Loader
 * Copyright (C) 2013-2014  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 *  std.text -- Standard GTA Text Loader Plugin for Mod Loader
 *
 */
#include <stdinc.hpp>
#include <modloader/gta3/fxt.hpp>
using namespace modloader;

static const uint64_t hash_mask   = 0x00000000FFFFFFFF;     // Mask for the hash on the behaviour
static const uint64_t is_fxt_mask = 0x0000000100000000;     // Mask for is_fxt on the behaviour

/*
 *  The plugin object
 */
class TextPlugin : public modloader::basic_plugin
{
    private:
        modloader_re3_t* modloader_re3{};
        modloader_reVC_t* modloader_reVC{};

        modloader::fxt_manager fxt;                     // FXT Files Manager

        LoadFileDetour<0x6A0228> gxt_d1_gta3;           // CText::Load detour for GTA III
        OpenFileDetour<0x6A0228> gxt_d1;                // CText::Load detour
        OpenFileDetour<0x69FD5A> gxt_d2;                // CText::LoadMissionText detour

        void ReloadGXT();                               // Reload current language file

    public:
        std::map<uint32_t, const modloader::file*> gxt; // GXT Files Map<hash, file>

         // Standard plugin methods
        const info& GetInfo();
        bool OnStartup();
        bool OnShutdown();
        int GetBehaviour(modloader::file&);
        bool InstallFile(const modloader::file&);
        bool ReinstallFile(const modloader::file&);
        bool UninstallFile(const modloader::file&);

} text_plugin;

REGISTER_ML_PLUGIN(::text_plugin);

/*
 *  TextPlugin::GetInfo
 *      Returns information about this plugin 
 */
const TextPlugin::info& TextPlugin::GetInfo()
{
    static const char* extable[] = { "gxt", "fxt", 0 };
    static const info xinfo      = { "std.text", get_version_by_date(), "LINK/2012", -1, extable };
    return xinfo;
}

const char* RegisterAndGetGxtFilePathRE(const char* filepath)
{
    std::string filepathString = static_cast<std::string>(filepath);
    auto filename = NormalizePath(filepathString.substr(GetLastPathComponent(filepathString)));
    auto it = text_plugin.gxt.find(modloader::hash(filename, ::tolower));
    auto halfpath = std::string(it != text_plugin.gxt.end() ? it->second->filepath() : "");
    auto gamepath = std::string(text_plugin.loader->gamepath);

    std::string fullpath;
    if (!halfpath.empty()) {
        fullpath.append(gamepath);
        fullpath.append(halfpath);
    }

    static std::string static_result;
    static_result = fullpath;
    return static_result.empty() ? filepath : static_result.c_str();
}

/*
 *  TextPlugin::OnStartup
 *      Startups the plugin
 */
bool TextPlugin::OnStartup()
{
    if (loader->game_id == MODLOADER_GAME_RE3)
    {
        text_plugin.modloader_re3 = (modloader_re3_t*)plugin_ptr->loader->FindSharedData("MODLOADER_RE3")->p;
        text_plugin.modloader_re3->callback_table->RegisterAndGetGxtFile_Unsafe = +[](const char* filepath) {
            return RegisterAndGetGxtFilePathRE(filepath);
        };
        return true;
    }
    else if (loader->game_id == MODLOADER_GAME_REVC)
    {
        text_plugin.modloader_reVC = (modloader_reVC_t*)plugin_ptr->loader->FindSharedData("MODLOADER_REVC")->p;
        text_plugin.modloader_reVC->callback_table->RegisterAndGetGxtFile_Unsafe = +[](const char* filepath) {
            return RegisterAndGetGxtFilePathRE(filepath);
        };
        return true;
    }

    if(gvm.IsIII() || gvm.IsVC() || gvm.IsSA())
    {
        this->fxt.make_samp_compatible();

        // Returns the overriden gxt file path (relative to gamedir)
        auto transformer = [this](std::string filename)
        {
            auto it = gxt.find(modloader::hash(filename, ::tolower));
            return std::string(it != gxt.end()? it->second->filepath() : "");
        };
        
        // Patch the game with detours
        if(gvm.IsIII())
        {
            gxt_d1_gta3.make_call();
            gxt_d1_gta3.OnTransform(transformer);
        }
        else
        {
            gxt_d1.make_call();
            gxt_d1.OnTransform(transformer);
            gxt_d2.make_call();
            gxt_d2.OnTransform(transformer);
        }
        return true;
    }
    return false;
}

/*
 *  TextPlugin::OnShutdown
 *      Shutdowns the plugin
 */
bool TextPlugin::OnShutdown()
{
    return true;
}


/*
 *  TextPlugin::GetBehaviour
 *      Gets the relationship between this plugin and the file
 */
int TextPlugin::GetBehaviour(modloader::file& file)
{
    if(!file.is_dir())
    {
        if(file.is_ext("fxt"))
        {
            file.behaviour = file.hash | is_fxt_mask;
            return MODLOADER_BEHAVIOUR_YES;
        }
        else if(file.is_ext("gxt"))
        {
            file.behaviour = file.hash;
            return MODLOADER_BEHAVIOUR_YES;
        }
    }
    return MODLOADER_BEHAVIOUR_NO;
}


/*
 *  TextPlugin::InstallFile
 *      Installs a file using this plugin
 */
bool TextPlugin::InstallFile(const modloader::file& file)
{
    if(file.behaviour & is_fxt_mask)    // Is FXT
    {
        auto table = (uint32_t)(file.behaviour & hash_mask);

        // Remove the previous table related to this FXT then load it again
        this->fxt.remove_table(table);
        if(ParseFXT(this->fxt, file.fullpath().c_str(), table))
            return true;

        Log("Failed to inject FXT file \"%s\"", file.filepath());
    }
    else // Is GXT
    {
        this->gxt[file.hash] = &file;
        this->ReloadGXT();
        return true;
    }
    return false;
}

/*
 *  TextPlugin::ReinstallFile
 *      Reinstall a file previosly installed that has been updated
 */
bool TextPlugin::ReinstallFile(const modloader::file& file)
{
    return InstallFile(file);
}

/*
 *  TextPlugin::UninstallFile
 *      Uninstall a previosly installed file
 */
bool TextPlugin::UninstallFile(const modloader::file& file)
{
    if(file.behaviour & is_fxt_mask)
    {
        // Simply remove it from the fxt manager table
        this->fxt.remove_table(file.hash);
        return true;
    }
    else
    {
        //  Erase from our gxt map and reload the current gxt
        this->gxt.erase(file.hash);
        this->ReloadGXT();
        return true;
    }
    return false;
}

/*
 *  TextPlugin::ReloadGXT
 *      Reloads the current GXT file
 */
void TextPlugin::ReloadGXT()
{
    if(loader->has_game_started)    // Only if game has started up...
    {
        injector::scoped_nop<10> nop_check;
        nop_check.make_nop(0x57326E, gvm.IsIII()? 2 : 6);                       // NOP some menu field check that avoids reloading of text
        void* menumgr = lazy_pointer<0xBA6748>().get();                         // FrontEndManager
        if(gvm.IsSA())
            injector::thiscall<void(void*, bool)>::call<0x573260>(menumgr, false);  // CMenuManager::InitialiseChangedLanguageSettings
        else
            injector::thiscall<void(void*)>::call<0x573260>(menumgr);  // CMenuManager::InitialiseChangedLanguageSettings (?)
    }
}
