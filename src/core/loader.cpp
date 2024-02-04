/*
 * Copyright (C) 2013-2014  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include "loader.hpp"
#include <modloader/modloader_re3.h>
#include <unicode.hpp>
using namespace modloader;

#ifndef NDEBUG
#include <debugger.hpp>
#endif

extern int InstallExceptionCatcher(void (*cb)(const char* buffer));

#define USE_TEST 0
REGISTER_ML_NULL();

// Mod Loader object
Loader loader;

/*
 * DllMain
 *      Entry-point
 */
extern "C"
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason == DLL_PROCESS_ATTACH)
    {
        char szEnvVarBuffer[8];
        const auto dwEnvVarLength = GetEnvironmentVariableA("MODLOADER_RE3", szEnvVarBuffer, sizeof(szEnvVarBuffer));
        if(dwEnvVarLength != 0 && dwEnvVarLength < sizeof(szEnvVarBuffer) && !strcmp(szEnvVarBuffer, "1"))
        {
            // Just wait for modloader_InitFromRE3 to be called
        }
        else
        {
            if(!loader.Patch())
                return FALSE;
        }
    }
    return TRUE;
}

extern "C" int __declspec(dllexport) modloader_InitFromRE3(modloader_re3_t* modloader_re3_share)
{
    return loader.InitFromRE3(modloader_re3_share) != 0;
}

bool Loader::InitFromRE3(modloader_re3_t* modloader_re3_share)
{
    // TODO check modloader_re3_share->size and substructs size
    // TODO check modloader_re3_share->re3_version?

    assert(modloader_re3_share != nullptr);

    auto& gvm = injector::address_manager::singleton();
    gvm.set_name("Mod Loader");

    // TODO InstallExceptionCatcher? maybe unecessary?

    // TODO non-static
    static modloader_re3_callback_table_t callback_table;
    callback_table.size = sizeof(callback_table);

    modloader_re3_share->modloader = &loader;
    modloader_re3_share->callback_table = &callback_table;
    assert(modloader_re3_share->re3_addr_table != nullptr);

    modloader_re3_share->Tick = +[](modloader_re3_t* modloader_re3_share) {
        static_cast<Loader*>(modloader_re3_share->modloader)->Tick();
    };

    modloader_re3_share->Shutdown = +[](modloader_re3_t* modloader_re3_share) {
        static_cast<Loader*>(modloader_re3_share->modloader)->Shutdown();
    };

    // TODO I don't think creating the shared data before startup is a good idea
    // but we need that before the plugins start patching stuff
    modloader_shdata_t* plugin_shdata = this->CreateSharedData("MODLOADER_RE3");
    plugin_shdata->type = MODLOADER_SHDATA_POINTER;
    plugin_shdata->p = modloader_re3_share;

    // TODO this should probably go somewhere else too
    this->p_gGameState = modloader_re3_share->re3_addr_table->p_gGameState;
    
    this->Startup(MODLOADER_GAME_RE3);

    return true;
}

/*
 *  Loader::Patch
 *      Patches the game code to run the loader
 */
bool Loader::Patch()
{
    typedef function_hooker_stdcall<0x8246EC, int(HINSTANCE, HINSTANCE, LPSTR, int)> winmain_hook;
    typedef function_hooker<0x53ECBD, void(int)> ridle_hook;
    typedef function_hooker<0x53ECCB, void(int)> rfidle_hook;   // Actually void() but... meh

    auto& gvm = injector::address_manager::singleton();
    gvm.set_name("Mod Loader");
  
    // Check if we have WinMain proc address, otherwise this game isn't supported
    if(try_address(winmain_hook::addr))
    {
        if(injector::ReadMemory<int>(0xC920E8)) // RwInitialized -- should enter only when using the mss32 loader on III/VC
        {
            const char* buf = "You installed Mod Loader wrongly!\n\n"
                              "You need Ultimate ASI Loader, and modloader.asi must be in the 'scripts/' directory.\n\n"
                              "Please refer to the 'Readme.txt' or 'Leia-me.txt' for more information.";
            this->Error(buf);
            return false;
        }

        // Hook WinMain to run mod loader
        injector::make_static_hook<winmain_hook>([this](winmain_hook::func_type WinMain,
                                                    HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
        {
            // Avoind circular looping forever
            static bool bRan = false;
            if(bRan) return WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
            bRan = true;

            const auto game_id = []() -> uint8_t {
                auto& gvm = injector::address_manager::singleton();
                if(gvm.IsSA())
                    return MODLOADER_GAME_SA;
                else if(gvm.IsVC())
                    return MODLOADER_GAME_VC;
                else if(gvm.IsIII())
                    return MODLOADER_GAME_III;
                else
                    return MODLOADER_GAME_UNK;
            }();
            
#ifndef NDEBUG
            auto& gvm = injector::address_manager::singleton();
            if(!gvm.IsSA())
            {
                static int& gGameState = *mem_ptr(0xC8D4C0).get<int>();

                gGameState = 5; // skip intro
                if(gvm.IsIII())
                    MakeNOP(raw_ptr(0x5811F8), 10);
                else if(gvm.IsVC())
                    MakeNOP(raw_ptr(0x601B3B), 10);

                if(gvm.IsIII())
                    MakeJMP(raw_ptr(0x405DB0), &Loader::Log);
                else if(gvm.IsVC())
                    MakeJMP(raw_ptr(0x401000), &Loader::Log);

                // Debugger does not attach properly in III/VC DxWnd, so we need to do it manually.
                LaunchDebugger();
            }
#endif

            // Install exception filter to log crashes
            InstallExceptionCatcher([](const char* buffer)
            {
                Log("\n\n");
                LogGameVersion();
                Log(buffer);
                loader.Shutdown();
            });

            // To be called each frame
            auto CallTick = [this](ridle_hook::func_type Idle, int& i)
            {
                this->Tick();
                return Idle(i);
            };

            // Do tick hook only if possible
            if(try_address(ridle_hook::addr))  make_static_hook<ridle_hook>(CallTick);
            if(try_address(rfidle_hook::addr)) make_static_hook<rfidle_hook>(CallTick);

            // Startup the loader and call WinMain, Shutdown the loader after WinMain.
            // If any mod hooked WinMain at Startup, no conflict will happen, we're takin' care of that
            {
            this->p_gGameState = mem_ptr(0xC8D4C0).get<int>();
            this->Startup(game_id);
            auto WinMain = (winmain_hook::func_type_raw) ReadRelativeOffset(winmain_hook::addr + 1).get();
            auto result = WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
            this->Shutdown();
            return result;
            }
        });

        return true;
    }
    else
    {
        char buf[128];
        this->Error("This game is not supported\nGame Info:\n%s", gvm.GetVersionText(buf));
        return false;
    }
}

/*
 *  Loader::Startup
 *      Starts the loader
 */
void Loader::Startup(uint8_t game_id)
{
    char rootPath[MAX_PATH];
    char appDataPath[MAX_PATH];

    // If not running yet and 'modloader' folder exists, let's start up
    if(!this->bRunning && IsDirectoryA("modloader"))
    {
        // Cleanup the base structure
        memset(this, 0, sizeof(modloader_t));

        // Initialise configs and counters
        this->vkRefresh      = VK_F4;
        this->bRunning       = false;
        this->bAutoRefresh   = true;
        this->bEnableMenu    = true;
        this->bEnableLog     = true;
        this->bEnablePlugins = true;
        this->maxBytesInLog  = 5242880;     // 5 MiB
        this->currentModId   = 0;
        this->currentFileId  = 0x8000000000000000;  // File id should have the hibit set
        this->game_id = game_id;

        // Open the log file
        OpenLog();
        LogGameVersion();

        // Setup root path variables
        GetCurrentDirectoryA(sizeof(rootPath), rootPath);
        MakeSureStringIsDirectory(this->gamePath = rootPath);

        // Setup "%ProgramData%/modloader/" variable
        if(SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, appDataPath)))
            MakeSureStringIsDirectory(MakeSureStringIsDirectory(this->commonAppDataPath = appDataPath).append("modloader"));

        // Setup "%LocalAppData%/modloader/" variable
        if(SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, appDataPath)))
            MakeSureStringIsDirectory(MakeSureStringIsDirectory(this->localAppDataPath = appDataPath).append("modloader"));

        // Setup basic path variables
        this->dataPath    = "modloader/.data/";
        this->pluginPath  = "modloader/.data/plugins/";
        this->profilesPath= "modloader/.profiles/";

        // Setup config file names
        this->folderConfigFilename = "modloader.ini";
        this->basicConfig          = dataPath + "config.ini";
        this->pluginConfigFilename = "plugins.ini";
        this->folderConfigDefault  = gamePath + dataPath + "modloader.ini.0";
        this->basicConfigDefault   = gamePath + dataPath + "config.ini.0";
        this->pluginConfigDefault  = gamePath + dataPath + "plugins.ini.0";

        // Make sure the important folders exist
        if(!MakeSureDirectoryExistA(dataPath.c_str())
        || !MakeSureDirectoryExistA(profilesPath.c_str())
        || !MakeSureDirectoryExistA(pluginPath.c_str())
        || !MakeSureDirectoryExistA(commonAppDataPath.c_str())
        || !MakeSureDirectoryExistA(localAppDataPath.c_str()))
        {
            Log("Warning: Mod Loader important directories could not be created (1).");
        }
        else
        {
            char hash_as_string[128];
            std::string normal_path = NormalizePath(this->gamePath);

#ifdef _WIN64
            static_assert(sizeof(size_t) == 8, "%.8x not correct");
#else
            static_assert(sizeof(size_t) == 4, "%.8x not correct");
#endif
            sprintf(hash_as_string, "%.8x", modloader::hash(normal_path));

            std::string unique_id;
            unique_id += "10"; // version of the hashing method, increase if it changes
            unique_id += hash_as_string;

            this->localAppDataPath += unique_id;
            this->commonAppDataPath += unique_id;

            MakeSureStringIsDirectory(this->localAppDataPath);
            MakeSureStringIsDirectory(this->commonAppDataPath);
            if(!MakeSureDirectoryExistA(localAppDataPath.c_str())
            || !MakeSureDirectoryExistA(commonAppDataPath.c_str()))
            {
                Log("Warning: Mod Loader important directories could not be created (2).");
            }
        }

        // Before loading inis, we should update from the old ini format to the new ini format (ofc only if the ini format is old)
        this->UpdateOldConfig();

        // Load the basic configuration file
        CopyFileA(basicConfigDefault.c_str(), basicConfig.c_str(), TRUE);
        this->ReadBasicConfig();
        
        // Check if logging is disabled by the basic config file
        if(!this->bEnableLog)
        {
            Log("Logging is disabled. Closing log file...");
            CloseLog();
        }

        // Register exported methods and vars
        modloader_t::has_game_started= false;
        modloader_t::has_game_loaded = false;
        modloader_t::gamepath        = this->gamePath.data();
        //modloader_t::cachepath       = this->cachePath.data();
        modloader_t::commonappdata   = this->commonAppDataPath.data();
        modloader_t::localappdata    = this->localAppDataPath.data();
        modloader_t::Log             = this->Log;
        modloader_t::vLog            = this->vLog;
        modloader_t::Error           = this->Error;
        modloader_t::CreateSharedData= this->CreateSharedData;
        modloader_t::DeleteSharedData= this->DeleteSharedData;
        modloader_t::FindSharedData  = this->FindSharedData;

        // Initialise sub systems
        this->ParseCommandLine();   // Parse command line arguments
        this->StartupMenu();
        this->LoadPlugins();        // Load plugins at /modloader/.data/plugins
        this->BeforeFirstScan();
        this->ScanAndUpdate();      // Search and install mods at /modloader
        this->StartupWatcher();     // Startups the automatic refresher

        // Startup successfully
        this->bRunning = true;
        Log("\nMod Loader has started up!\n");
    }
}

/*
 *  Loader::Shutdown
 *      Shut downs the loader
 */
void Loader::Shutdown()
{
    if(this->bRunning)
    {
        // Unload the plugins
        Log("\nShutting down Mod Loader...");
        this->ShutdownWatcher();
        this->ShutdownMenu();
        this->UnloadPlugins();
        Log("Mod Loader has been shutdown.");
        
        // Finish containers
        this->plugins_priority.clear();
        this->extMap.clear();
        this->mods.Clear();
        
        // Close the log file
        this->CloseLog();
        this->bRunning = false;
    }
}

/*
 *  Loader::Shutdown
 *      This is called every frame
 */
void Loader::Tick()
{
    this->has_game_started = (*p_gGameState >= 7);
    this->has_game_loaded  = (*p_gGameState >= 9);
    this->CheckWatcher();   // updates from changes in the filesystem
    this->TestHotkeys();
}

/*
 *  Loader::TestHotkeys
 *      Test hotkeys for specific actions
 */
void Loader::TestHotkeys()
{
    if(false)   // Unecessary, we got a menu and we got a automatic refresher
    {
        static bool prevF4 = false; 
        static bool currF4 = false; 

        // Get current hotkey states
        currF4 = (GetKeyState(vkRefresh) & 0x8000) != 0;

        // Check hotkey states
        if(currF4 && !prevF4)
        {
            this->ScanAndUpdate();
        }

        // Save previous states
        prevF4 = currF4;
    }
}

/*
 *  Loader::LogGameVersion
 *      Logs the game version into the logging stream
 */
void Loader::LogGameVersion()
{
    char buffer[128];
    Log("Game version: %s", injector::address_manager::singleton().GetVersionText(buffer));
}

/*
 *  Loader::BeforeFirstScan
 *       Performs some important operations before the first scan such as loading modloader.ini and profiles
 */
void Loader::BeforeFirstScan()
{
    // Load modloader.ini and .profiles/*, then check out if we have a command line profile to take care of
    this->mods.LoadConfigFromINI();
    if(this->modprof_cmd.size())
    {
        if(auto* prof = this->mods.FindProfile(modprof_cmd))
            this->mods.SwitchToProfileAsAnonymous(*prof);
        else
            Log("Warning: Profile \"%s\" received from command line does not exist", modprof_cmd.c_str());
    }

    // Check for conditional profiles
    for(Profile& prof : this->mods.Profiles())
    {
        auto& module = prof.GetModuleCondition();
        if(module.size() && GetModuleHandleA(module.c_str()))
        {
            this->mods.SwitchToProfileAsAnonymous(prof);
            break;
        }
    }

}

/*
 *  Loader::ScanAndUpdate
 *       Rescans and Updates the mods
 */
void Loader::ScanAndUpdate()
{
    Updating xup;
    mods.Scan();
    mods.Update();
}

/*
 *  Loader::UpdateFromJournal
 *       Updates mods that changed in the last few seconds specified in the journal
 */
void Loader::UpdateFromJournal(const Journal& journal)
{
    Updating xup;
    mods.Scan(journal);
    mods.Update();
}

/*
 *  Loader::NotifyUpdateForPlugins
 *       Notify the plugins that we've installed/uninstalled stuff
 *       This normally happens after a rescan
 */
void Loader::NotifyUpdateForPlugins()
{
    for(auto& plugin : this->plugins)
        plugin.Update();
}


/*
 *  Loader::FindHandlerForFile
 *       Finds the plugin responssible for handling the file @m,
 *       also the plugins that wants to receive the file at @callme
 */
auto Loader::FindHandlerForFile(modloader::file& m, ref_list<PluginInformation>& callme) -> PluginInformation*
{
    PluginInformation* handler = nullptr;
    
    // Iterate on the plugins to find a handler for it
    for(PluginInformation& plugin : this->GetPluginsBy(m.filext()))
    {
        auto state = plugin.FindBehaviour(m);
        
        if(state == BehaviourType::Yes)
        {
            // We found a handler and behaviour, stop the search immediately, don't check for other callme's
            handler = &plugin;
            break;
        }
        else if(state == BehaviourType::CallMe)
        {
            // This plugin requests this file to be sent for some reason (readme files?)
            callme.emplace_back(plugin);
        }
    }
    
    return handler;
}


/*
 *  Loader::GetPluginsBy
 *       Gets a list of plugins sorted for file-behaviour-search.
 *       That's, sort by priority and extension.
 */
auto Loader::GetPluginsBy(const std::string& extension) -> ref_list<PluginInformation>
{
    SimplePriorityPred<PluginInformation> pred_base;
    auto list = refs(this->plugins);

    // Checks if the specified plugin list contains any plugin 'p'
    auto contains = [](ref_list<PluginInformation>& plugins, const PluginInformation& p)
    {
        return std::any_of(plugins.begin(), plugins.end(), [&p](const PluginInformation& a) { return a == p; });
    };

    // Predicate to execute the sorting
    auto pred = [&, this](const PluginInformation& a, const PluginInformation& b)
    {
        if(a.priority == b.priority)    // If priorities are equal, check for extension!
        {
            auto it = this->extMap.find(extension);
            if(it != extMap.end())  // handleabe extension should have priority over other extensions
            {
                bool ca = contains(it->second, a);
                bool cb = contains(it->second, b);
                if(ca && !cb) return true;          // a has priority over b
                else if(!ca && cb) return false;    // b has priority over a
            }
        }
        return pred_base(a, b);
    };

    // Sort and return
    std::sort(list.begin(), list.end(), pred);
    return list;
}
