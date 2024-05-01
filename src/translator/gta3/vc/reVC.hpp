#pragma once
#include <modloader/modloader.hpp>
#include <modloader/modloader_reVC.h>

// GTA VC REVC table
static void VC_REVC(std::map<memory_pointer_raw, memory_pointer_raw>& map)
{
    modloader_reVC_t* modloader_reVC{};
    const auto* modloader_reVC_shdata = modloader::plugin_ptr->loader->FindSharedData("MODLOADER_REVC");
    assert(modloader_reVC_shdata != nullptr);
    assert(modloader_reVC_shdata->type == MODLOADER_SHDATA_POINTER);
    modloader_reVC = (modloader_reVC_t*)modloader_reVC_shdata->p;
    assert(modloader_reVC != nullptr);

    //
    // (Notice '->' means "exactly pointing to"!!!!!)
    //

    
    // std.fx
    if(true)
    {
		map[0x5BF8B7] = 0x4A4BB0;   // call    _ZN9CTxdStore7LoadTxdEiPKc ; "models/particle.txd"
       
        map[0x5BA850] = 0x55C8A0;   // _ZN4CHud10InitialiseEv
        map[0x588850] = 0x55C7F0;   // _ZN4CHud8ShutdownEv
        map[0x5827D0] = 0x4C5DC0;   // _ZN6CRadar12LoadTexturesEv
        map[0x5BA865] = 0x55C8B5;   // call    _ZN9CTxdStore7LoadTxdEiPKc ; "models/hud.txd"
 
        map[0x5BA690] = 0x552310;   // _ZN5CFont10InitialiseEv
        map[0x7189B0] = 0x5522E0;   // _ZN5CFont8ShutdownEv
        map[0x5BA6A4] = 0x552327;   // call    _ZN9CTxdStore7LoadTxdEiPKc ; "models/fonts.txd"
 
        map[0xBA6748] = 0x869630;   // CMenuManager FrontEndMenuManager
        map[0x572EC0] = 0x4A3A13;   // _ZN12CMenuManager15LoadAllTexturesEv
        map[0x574630] = 0x4A394D;   // _ZN12CMenuManager14UnloadTexturesEv
        map[0x572F1E] = 0x4A3AC3;   // call    _ZN9CTxdStore7LoadTxdEiPKc ; "models/fronten1.txd"
        map[0x573040] = 0x4A3B5D;   // call    _ZN9CTxdStore7LoadTxdEiPKc ; "models/fronten2.txd"

        // VC stuff
        map[xVc(0x570D8A)] = 0x570D8A;  // call    _ZN11CFileLoader21LoadAtomicFile2ReturnEPKc ; "models/generic/zonecylb.dff"
        map[xVc(0x570D7A)] = 0x570D7A;  // call    _ZN11CFileLoader21LoadAtomicFile2ReturnEPKc ; "models/generic/arrow.dff"
        map[xVc(0x4BBB30)] = 0x4BBB30;  // _ZN11CPlayerInfo14LoadPlayerSkinEv
        map[xVc(0x94AD28)] = 0x94AD28;  // CPlayerInfo _ZN6CWorld7PlayersE[]
        map[xVc(0x627EB8)] = 0x627EB8;  // push    offset "models\generic\player.bmp"
        map[0x7F3820]      = 0x64DEC0;  // _RwTextureDestroy
        map[xVc(0x627E7F)] = 0x627E7F;  // call    RwTextureRead    ; @CPlayerSkin::GetSkinTexture
    }

    // std.text
    if(true)
    {
        map[0x57A161] = 0x49EE23;   // call    _ZN5CText3GetEPKc+                       ; fxtp
        map[0x748CFB] = 0x600411;   // call    _Z14InitialiseGamev                      ; fxtp
        map[0x6A0228] = 0x5855F8;   // call    _ZN8CFileMgr8OpenFileEPKcS1_  ; @CText::Load
        map[0x69FD5A] = 0x5852A8;   // call    _ZN8CFileMgr8OpenFileEPKcS1_  ; @CText::LoadMissionText
        map[0x57326E] = 0x4A388C;   // jz      loc_573313                    ; Check for some menu stuff to avoid reloading text
        map[0xBA6748] = 0x869630;   // CMenuManager FrontEndMenuManager
        map[0x573260] = 0x4A3882;   // _ZN12CMenuManager33InitialiseChangedLanguageSettingsEv
        map[0x6A0050] = 0x584F30;   // _ZN5CText3GetEPKc
    }
 
    // std.movies
    if(true)
    {
        map[0x748B00] = 0x5FFFC3;   //  call    _CreateVideoPlayer  ; "movies/Logo.mpg"
        map[0x748BF9] = 0x600179;   //  call    _CreateVideoPlayer  ; "movies/GTAtitles.mpg"
    }

    // std.scm
    if(true)
    {
        map[0x468EC9] = 0x4506E6;   // call    _ZN8CFileMgr8OpenFileEPKcS1_  ; "main.scm" @CTheScripts
        map[0x489A4A] = 0x608C86;   // call    _ZN8CFileMgr8OpenFileEPKcS1_  ; "main.scm" @CRunningScript
    }

    // std.sprites
    if(true)
    {
        map[0x48418A] = 0x45944D;   // call    _ZN9CTxdStore7LoadTxdEiPKc    ; @CRunningScript
        map[xVc(0x4A6F4E)] = 0x4A6F4E;// call    _ZN9CTxdStore7LoadTxdEiPKc  ; @LoadSplash
    }

    // std.stream
    if(true)
    {
        static void* vcemu_pMaxInfoForModel;
        static DWORD uMaxResources = modloader_reVC->reVC_addr_table->uMaxResources;

        map[0x40D014] = &modloader_reVC->reVC_addr_table->ms_aInfoForModel;               // -> offset ms_aInfoForModel //doublecheck
        map[xVc(0x4102B2)] = &uMaxResources;          // -> DWORD MAX_RES ; max resources
        map[0x5B8AFC] = &vcemu_pMaxInfoForModel;// -> &ms_aInfoForModel[MAX_INFO_FOR_MODEL]
        // DONT CHANGE THE FOLLOWING LINE, CHANGE THE MAPPING ABOVE INSTEAD!!! Maybe change the 0x14 sizeof /////////////
#ifdef _WIN64
        vcemu_pMaxInfoForModel = *mem_ptr(0x40D014).get<char*>() + (0x1C * *mem_ptr(xVc(0x4102B2)).get<uint32_t>());
#else
        vcemu_pMaxInfoForModel = *mem_ptr(0x40D014).get<char*>() + (0x14 * *mem_ptr(xVc(0x4102B2)).get<uint32_t>());
#endif
        ///////////////////////////////////

        map[0x8E3FE0] = modloader_reVC->reVC_addr_table->p_gCdStreamFlags;   // DWORD StreamCreateFlags
        map[xVc(0x6F76F4)] = modloader_reVC->reVC_addr_table->p_gCdStreamSema; // HANDLE hCdSemaphore
        map[xVc(0x6F7700)] = modloader_reVC->reVC_addr_table->p_gChannelRequestQ; // Queue CdQueue;
        map[xVc(0x6F76FC)] = modloader_reVC->reVC_addr_table->p_gpReadInfo; // CdStream* channelFile
        map[xVc(0x6F7718)] = modloader_reVC->reVC_addr_table->p_gbCdStreamAsync; // BOOL streamingInitialized
        map[xVc(0x6F7714)] = modloader_reVC->reVC_addr_table->p_gbCdStreamOverlapped; // BOOL overlappedIO
        map[0x8E4CAC] = modloader_reVC->reVC_addr_table->p_ms_pStreamingBuffer;   // void* CStreaming::ms_pStreamingBuffer[2]
        map[0x8E4CA8] = modloader_reVC->reVC_addr_table->p_ms_streamingBufferSize;   // unsigned int CStreaming::ms_streamingBufferSize
        
        /*
        map[0x72F420] = nullptr;    // _ZN10CMemoryMgr6MallocEj // Doesn't exist!!? Doesn't matter. Used by us only in SA.
        map[0x72F4C0] = 0x526FD0;   // _ZN10CMemoryMgr11MallocAlignEjj
        map[0x72F4F0] = 0x527000;   // _ZN10CMemoryMgr9FreeAlignEPv
        UNEEDED map[0x532310] = 0x473600;   // _ZN10CDirectory7AddItemERKNS_13DirectoryInfoE
        UNEEDED map[0x532450] = gta3emu_CDirectory_FindItem2; // _ZN10CDirectory8FindItemEPKc
        */
        map[0x5324A0] = modloader_reVC->reVC_addr_table->CDirectory__FindItem4;   // _ZN10CDirectory8FindItemEPKcRjS2_

        /*
        map[0x406560] = 0x406140;   // _Z14CdStreamThreadPv
        map[0x5B8E1B] = 0x406A13;   // call    _ZN10CStreaming15LoadCdDirectoryEv       ; @CStreaming::Init //not found, check 405C80 
        map[0x40CF34] = 0x40A321;   // call    _Z12CdStreamReadiPvjj                    ; @CStreaming::RequestModelStream
        map[xVc(0x40B76A)] = 0x40A512;// call    _Z12CdStreamReadiPvjj                    ; @CStreaming::LoadAllRequestedModels
        map[xVc(0x40B780)] = 0x40A536;// call    _Z12CdStreamReadiPvjj                    ; @CStreaming::LoadAllRequestedModels
        map[xIII(0x40A128)] = 0x40A128; // call    _ZN14CStreamingInfo16GetCdPosnAndSizeERjS0_ ; @CStreaming::RequestModelStream
        map[xIII(0x40A4F3)] = 0x40A4F3; // call    _ZN14CStreamingInfo16GetCdPosnAndSizeERjS0_ ; @CStreaming::LoadAllRequestedModels
        map[xVc(0x408521)] = 0x405E71;// add     esi, edi    ; @CdStreamRead
        map[0x409F76] = 0x40A997;   // call    _ZN10CDirectory8FindItemEPKcRjS2_        ; @CStreaming::RequestSpecialModel
        map[0x409FD9] = 0x40A9E5;   // call    _ZN10CStreaming12RequestModelEii         ; @CStreaming::RequestSpecialModel
        map[0x5B6183] = 0x406DB3;   // call    _ZN8CFileMgr8OpenFileEPKcS1_             ; @CStreaming::LoadCdDirectory
        map[0x532361] = 0x473641;   // call    _ZN8CFileMgr8OpenFileEPKcS1_             ; @CDirectory::ReadDirFile
        map[0x5AFC9D] = 0x4046BF;   // call    _ZN8CFileMgr8OpenFileEPKcS1_             ; @CCutsceneMgr::LoadCutsceneData_postload
        map[xVc(0x627D79)] = 0x59BB39;// call    _ZN8CFileMgr8OpenFileEPKcS1_           ; @FindPlayerDff ; "models/gta3.dir"
        map[xIII(0x4BA6F6)] = 0x4BA6F6;// call    RwStreamOpen                          ; @CCutsceneHead::PlayAnimation ; "anim/cuts.img"
        map[0x40685E] = 0x406296;   // call    ds:__imp_CreateFileA                     ; @CdStreamAddImage
        map[0x5B8310] = 0x406D6A;   // call    _ZN10CStreaming15LoadCdDirectoryEPKci    ; @CStreaming::LoadCdDirectory
        map[0x5B61E1] = 0x406DC9;   // call    _ZN8CFileMgr4ReadEiPci                   ; @CStreaming::LoadCdDirectory  ; read entry @up
        map[xVc(0x40FDD9)] = 0x407043;// call    _ZN8CFileMgr4ReadEiPci             ; @CStreaming::LoadCdDirectory ; read entry @up
        map[0x5B627A] = 0x406F2B;   // call    _ZN10CDirectory7AddItemERKNS_13DirectoryInfoE ; @CStreaming::LoadCdDirectory
        map[xIII(0x406EB0)] = 0x406EB0; // call    _ZN14CStreamingInfo16GetCdPosnAndSizeERjS0_  ; dff
        map[xIII(0x406FD9)] = 0x406FD9; // call    _ZN14CStreamingInfo16GetCdPosnAndSizeERjS0_  ; txd
        @@map[xIII(0x4038FC)] = 0x4038FC;   // call    __loadIfp    ; @CAnimManager::LoadAnimFiles "anim/ped.ifp"
        ??map[0x40E2C5] = 0x40A571;   // call    _ZN10CStreaming21ConvertBufferToObjectEPcii
        ??map[0x40E1BE] = 0x40A585;   // call    _ZN10CStreaming22FinishLoadingLargeFileEPci
        DEADLOCK_FIX map[xVc(0x4088F7)] = 0x405B67;	// loc_4088F7
        DEADLOCK_FIX map[xVc(0x4088F7) + 10] = 0x405B67 + 10;  // call    ds:CreateSemaphoreA
        DEADLOCK_FIX map[xVc(0x4088F7) + 0x22] = 0x405B67 + 0x22;	// jnz     short loc_408930
        DEADLOCK_FIX map[xVc(0x4086B6)] = 0x405E16;	// mov     eax, [ecx+ebp+14h]
        DEADLOCK_FIX map[0x406460] = 0x406010;	// _Z12CdStreamSynci

        @@map[0xB74490] = 0x8F2C60;   // CPool<> *CPools::ms_pPedPool
        @@map[0xB74494] = 0x9430DC;   // CPool<> *CPools::ms_pVehiclePool
        @@map[0xB74498] = 0x8F2C04;   // CPool<> *CPools::ms_pBuildingPool
        @@map[0xB7449C] = 0x880E28;   // CPool<> *CPools::ms_pObjectPool
        // TODO X refresh.cpp map[0x4D6C30] = ;   // _Z32RpAnimBlendClumpGiveAssociationsP7RpClumpP21CAnimBlendAssociation //doesn't exist?
        // TODO X refresh.cpp map[0x4D6BE0] = ;   // _Z35RpAnimBlendClumpExtractAssociationsP7RpClump //doesn't exist?
        // TODO X refresh.cpp map[0x681810] = ;   // _ZNK12CTaskManager16GetTaskSecondaryEi //doesn't exist?
        */
        map[0x561A00] = modloader_reVC->reVC_addr_table->CTimer__Resume;   // _ZN6CTimer6ResumeEv
        map[0x5619D0] = modloader_reVC->reVC_addr_table->CTimer__Suspend;   // _ZN6CTimer7SuspendEv
        map[0x4087E0] = modloader_reVC->reVC_addr_table->CStreaming__RequestModel;   // _ZN10CStreaming12RequestModelEii
        map[0x4089A0] = modloader_reVC->reVC_addr_table->CStreaming__RemoveModel;   // _ZN10CStreaming11RemoveModelEi
        map[0x40EA10] = modloader_reVC->reVC_addr_table->CStreaming__LoadAllRequestedModels;   // _ZN10CStreaming22LoadAllRequestedModelsEb
        map[0x40E460] = modloader_reVC->reVC_addr_table->CStreaming__FlushChannels;   // _ZN10CStreaming13FlushChannelsEv
        map[0x40CF80] = modloader_reVC->reVC_addr_table->CStreaming__RemoveAllUnusedModels;   // _ZN10CStreaming21RemoveAllUnusedModelsEv
        map[0x40CFD0] = modloader_reVC->reVC_addr_table->CStreaming__RemoveLeastUsedModel;   // _ZN10CStreaming20RemoveLeastUsedModelEj
        /*
        @@map[xVc(0x59E2B0)] = 0x52D210; // _ZN11CAutomobile20SetupSuspensionLinesEv
        // Non streamed resources
        map[0x5B9188] = 0x4763B2 ;  // call    _ZN11CFileLoader17LoadCollisionFileEPKch
        map[0x5B91B0] = 0x4763E2;   // call    _ZN11CFileLoader14LoadAtomicFileEPKc
        map[0x5B91DB] = 0x47640F;   // call    _ZN11CFileLoader13LoadClumpFileEPKc
        map[0x5B910A] = 0x476358;   // call    _ZN11CFileLoader17LoadTexDictionaryEPKc
        map[xIII(0x476589)] = 0x476589; // call    _ZN11CFileLoader17LoadCollisionFileEPKch
        */

        // Removed in reVC source code if built for Mod Loader
        /*
        map[xVc(0x410814)] = 0x48C108; // call    _ZN8CFileMgr8OpenFileEPKcS1_; "models/txd.img"
        map[xVc(0x41083A)] = 0x48C12E; // call    __CreateCacheTxdImage
        */
    }

    
    // std.data
    if(true)
    {
        map[0x464D50] = 0x44FE30;   // _ZN11CTheScripts18IsPlayerOnAMissionEv
        map[0x5B6890] = 0x578CC0;   // _ZN17CVehicleModelInfo18LoadVehicleColoursEv
        map[0x5B68AB] = 0x578CE3;   // call    _ZN8CFileMgr8OpenFileEPKcS1_             ; @CVehicleModelInfo::LoadVehicleColours
        map[0x5B905E] = 0x48D97C;   // call    _ZN8CFileMgr8OpenFileEPKcS1_             ; @CFileLoader::LoadLevel
        map[0x5BD830] = 0x5AAE20;   // _ZN16cHandlingDataMgr16LoadHandlingDataEv
        map[0x5BD850] = 0x5AAE4E;   // call    _ZN8CFileMgr12LoadTextFileEPKcPhi        ; @cHandlingDataMgr::LoadHandlingData
        map[0xC2B9C8] = 0x978E58;   // mod_HandlingManager CHandlingData
        map[0x5B8428] = 0x48C846;   // call    _ZN8CFileMgr8OpenFileEPKcS1_             ; @CFileLoader::LoadObjectTypes
        map[0x5B871A] = 0x48B079;   // call    _ZN8CFileMgr8OpenFileEPKcS1_             ; @CFileLoader::LoadScene
        map[0x5BF750] = 0x5D5750;   // _ZN11CWeaponInfo10InitialiseEv
        map[0x5BE68A] = 0x5D527B;   // call    _ZN8CFileMgr12LoadTextFileEPKcPhi        ; @CWeaponInfo::Initialise
        map[0x5BBAC0] = 0x4D05E0;   // _ZN10CTimeCycle10InitialiseEb
        map[xVc(0x4D0614)] = 0x4D0614; // call    _ZN8CFileMgr12LoadTextFileEPKcPhi     ; @CTimeCycle::Initialise
        map[0x6EAE80] = 0x5C3940;   // _ZN11CWaterLevel20WaterLevelInitialiseEv
        map[0x6EAF4D] = 0x5C395A;   // call    _ZN8CFileMgr8OpenFileEPKcS1_             ; @CWaterLevel::WaterLevelInitialise
        map[0x5BCFE0] = 0x53E9C0;   // _ZN11CPopulation13LoadPedGroupsEv
        map[0x5BCFFB] = 0x53E9DF;   // call    _ZN8CFileMgr8OpenFileEPKcS1_             ; @CPopulation::LoadPedGroups
        map[0x5BB890] = 0x530020;   // _ZN9CPedStats12LoadPedStatsEv
        map[0x5BB89F] = 0x530054;   // call    _ZN8CFileMgr12LoadTextFileEPKcPhi        ; @CPedStats::LoadPedStats
        map[0x5B5360] = 0x4E4560;   // _ZN11CObjectData10InitialiseEPcb
        map[0x5B5444] = 0x4E476D;   // call    _ZN8CFileMgr12LoadTextFileEPKcPhi        ; @CObjectData::Initialise
        map[0x608B45] = 0x530BD7; // call    _ZN8CFileMgr12LoadTextFileEPKcPhi          ; @CPedType::LoadPedData
        //map[0x6F7440] = 0x5B2CA0;   // _ZN6CTrain10InitTrainsEv //doesn't exist -- just a nullsub
        //map[0x6F7470] = 0x;   // call    _ZN6CTrain25ReadAndInterpretTrackFileEPcPP10CTrainNodePiPfi ; "tracks.dat" //doesn't exist
        //map[0x6F74BC] = 0x;   // call    _ZN6CTrain25ReadAndInterpretTrackFileEPcPP10CTrainNodePiPfi ; "tracks2.dat" //doesn't exist
        //map[0x6F7496] = 0x;   // call    _ZN6CTrain25ReadAndInterpretTrackFileEPcPP10CTrainNodePiPfi ; "tracks3.dat" //doesn't exist
        //map[0x6F74E2] = 0x;   // call    _ZN6CTrain25ReadAndInterpretTrackFileEPcPP10CTrainNodePiPfi ; "tracks4.dat" //doesn't exist
        map[0x748CFB] = 0x600411;   // call    _Z14InitialiseGamev
        //map[0x590D2A] = 0x;   // mov     eax, 8Ch                                 ; 8Ch = Loading Screen Max Progress //doesn't exist
        //map[0x590D67] = 0x;   // cmp     eax, 8Ch                                 ; 8Ch = Loading Screen Max Progress //doesn't exist
        //map[0x5B906A] = 0x;   // call    _ZN11CFileLoader8LoadLineEi              ; @CFileLoader::LoadLevel -- loop begin //check 48DDB8
        //map[0x5B92E6] = 0x;   // call    _ZN11CFileLoader8LoadLineEi              ; @CFileLoader::LoadLevel -- loop end //check 48DDB8
        map[0x5B92F9] = 0x48DDFD;   // call    _ZN8CFileMgr9CloseFileEi                 ; @CFileLoader::LoadLevel
        map[0x53BC95] = 0x4A4C99;   // call    _ZN11CFileLoader9LoadLevelEPKc           ; @CGame::Initialise -- default.dat
        map[0x53BC9B] = 0x4A4CA0;   // call    _ZN11CFileLoader9LoadLevelEPKc           ; @CGame::Initialise -- gta.dat
        map[0x4C5940] = 0x55F7D0;   // _ZN10CModelInfo12GetModelInfoEPKcPi
        map[0x5B925F] = 0x48DCF2;   // call    _ZN11CObjectData10InitialiseEPc          ; @CFileLoader::LoadLevel

        map[xVc(0x5C395A)] = 0x5C395A; // call    _ZN8CFileMgr8OpenFileEPKcS1_          ; "data/waterpro.dat"
        map[xVc(0x4CE8A0)] = 0x4CE8A0; // _ZN13CSurfaceTable10InitialiseEPc
        map[0x55D100] = 0x4CE8CC; // call    _ZN8CFileMgr12LoadTextFileEPKcPhi          ; @CSurfaceTable::Initialise
        map[xVc(0x565940)] = 0x565940; // _ZN9CParticle12ReloadConfigEv
        map[xVc(0x565B2E)] = 0x565B2E; // call    _ZN8CFileMgr12LoadTextFileEPKcPhi     ; @cParticleSystemMgr::LoadParticleData
        map[xVc(0x527570)] = 0x527570; // _ZN4CPed13LoadFightDataEv
        map[xVc(0x527590)] = 0x527590; // call    _ZN8CFileMgr12LoadTextFileEPKcPhi     ; @CPed::LoadFightData

        map[xVc(0x5B220A)] = 0x5B220A; // call    _ZN6CPlane8LoadPathEPKcRiRfb  ; "flight.dat"
        map[xVc(0x5B2475)] = 0x5B2475; // call    _ZN6CPlane8LoadPathEPKcRiRfb  ; "flight2.dat"
        map[xVc(0x5B24AE)] = 0x5B24AE; // call    _ZN6CPlane8LoadPathEPKcRiRfb  ; "flight3.dat"
    }

    // traits
    if(true)
    {
        map[0x5B62CF] = 0x40FCDD;   // -> DWORD 4E20h   ; TXD Start Index
        map[0x5B6314] = 0x40FD1B;   // -> DWORD 61A8h   ; COL Start Index
        map[0x5B63C5] = 0x40FD48;   // -> DWORD 63E7h   ; IFP Start Index
        map[0x408897] = 0x4014EF+1; // -> offset _ZN10CModelInfo16ms_modelInfoPtrsE
    }

    // AbstractFrontend | TheMenu
    if(true)
    {
        // Not available for VC
    }
}
