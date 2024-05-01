/* 
 * Standard Streamer Plugin for Mod Loader
 * Copyright (C) 2013-2014  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include "streaming.hpp"
using namespace modloader;

extern "C"
{
    // Assembly hooks at "asm/" folder
#ifndef _WIN64
    extern void HOOK_RegisterNextModelRead();
    extern void HOOK_NewFile();
    extern void HOOK_RegisterNextModelRead_VC();
    extern void HOOK_NewFile_3VC();
    extern void HOOK_FixBikeSuspLines();
#endif

    // Next model read registers. It's important to have those two vars! Don't leave only one!
    int  iNextModelBeingLoaded = -1;            // Set by RegisterNextModelRead, will then be sent to iModelBeingLoaded
    int  iModelBeingLoaded = -1;                // Model currently passing throught CdStreamRead

    // Note: Don't perform pointer arithimetic, or indexing, with this pointer! This have a sizeof = 0 (4 actually).
    // Use the InfoForModel(id) and InfoForModelIndex(info) functions!!!
    CStreamingInfo* ms_aInfoForModel;

    DWORD *pStreamCreateFlags;

    void** pStreamingBuffer;

    uint32_t* streamingBufferSize;

    void(*LoadCdDirectory2)(const char*, int);

    CDirectory* clothesDirectory;

    modloader_re3_t* modloader_re3{};
    modloader_reVC_t* modloader_reVC{};

    // Returns the file handle to be used for iModelBeingLoaded (called from Assembly)
    HANDLE CallGetAbstractHandle(HANDLE hFile)
    {
        if(iModelBeingLoaded == -1) return hFile;
        return streaming->TryOpenAbstractHandle(iModelBeingLoaded, hFile);
    }

    // Registers the next model to be loaded (called from Assembly)
    void RegisterNextModelRead(uint32_t id)
    {
        iNextModelBeingLoaded = id;
        if(streaming->DoesModelNeedsFallback(id))    // <- make sure the resource hasn't been deleted from disk
        {
            plugin_ptr->Log("Resource id %d has been deleted from disk, falling back to stock model.", id);
            streaming->FallbackResource(id, true);   // forceful but safe since we are before info setup in RequestModelStream
        }
    }

    static HANDLE __stdcall CreateFileForCdStream(
        LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
    {
        LPCSTR lpActualFileName = streaming->GetCdStreamPath(lpFileName);
        plugin_ptr->Log("Opening file for streaming \"%s\"", lpActualFileName);
        return CreateFileA(
            lpActualFileName, dwDesiredAccess, dwShareMode,
            lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    
    // Reproduces the code @6BF741
    void FixBikeSuspPtr(char* m_pColData)
    {
        *(char*)(m_pColData + 6) = 4;
        char* ptr = injector::cstd<char*(size_t)>::call<0x72F420>(128); // CMemoryMgr::Malloc
        *(char**)(m_pColData + 16) = ptr;
        *(uint32_t*)(ptr + 10) = 0x47C34FFFu;   // a float actually
        *(uint32_t*)(ptr + 8)  = 0x47C34FFFu;   // a float actually
    }
};


void __stdcall CdStreamShutdownSync_Stub( CdStream* stream, size_t idx )
{
	streaming->cdStreamSyncFuncs.Shutdown( &stream[idx] );
}

uint32_t CdStreamSync( int32_t streamID )
{
	static bool bInitFields = false;
	static CdStream** ppStreams;
	static BOOL* streamingInitialized;
	static BOOL* overlappedIO;

	if ( !bInitFields )
	{
		if( gvm.IsSA() )
		{
			auto& cdinfo = *memory_pointer(0x8E3FE0).get<CdStreamInfoSA>();
			ppStreams = &cdinfo.pStreams;
			streamingInitialized = &cdinfo.streamingInitialized;
			overlappedIO = &cdinfo.overlappedIO;
		}
		else if( gvm.IsVC() || gvm.IsIII() )
		{
			ppStreams = memory_pointer(xVc(0x6F76FC)).get<CdStream*>();
			streamingInitialized = memory_pointer(xVc(0x6F7718)).get<BOOL>();
			overlappedIO = memory_pointer(xVc(0x6F7714)).get<BOOL>();
		}
	
		bInitFields = true;
	}

	CdStream* stream = &((*ppStreams)[streamID]);
	if ( *streamingInitialized )
	{
		scoped_lock lock( streaming->cdStreamSyncLock );
		streaming->cdStreamSyncFuncs.SleepCS( stream, &streaming->cdStreamSyncLock );
		stream->bInUse = 0;
		return stream->status;
	}

	if ( *overlappedIO && stream->hFile != nullptr )
	{
		DWORD numBytesRead;
		return GetOverlappedResult( stream->hFile, &stream->overlapped, &numBytesRead, TRUE ) != 0 ? 0 : 254;
	}
	return 0;
}

/*
 *  Streaming thread
 *      This thread reads pieces from cd images and in mod loader on disk files
 */
int __stdcall CdStreamThread()
{
    HANDLE* pSemaphore = nullptr;
    Queue* pQueue      = nullptr;
    CdStream** ppStreams = nullptr;

    // Get reference to the addresses we'll use.
    if(gvm.IsSA())
    {
        auto& cdinfo = *memory_pointer(0x8E3FE0).get<CdStreamInfoSA>();
        pSemaphore = &cdinfo.semaphore;
        pQueue = &cdinfo.queue;
        ppStreams = &cdinfo.pStreams;
    }
    else if(gvm.IsVC() || gvm.IsIII() || plugin_ptr->loader->game_id == MODLOADER_GAME_RE3 || plugin_ptr->loader->game_id == MODLOADER_GAME_REVC)
    {
        pSemaphore = memory_pointer(xVc(0x6F76F4)).get<HANDLE>();
        pQueue = memory_pointer(xVc(0x6F7700)).get<Queue>();
        ppStreams = memory_pointer(xVc(0x6F76FC)).get<CdStream*>();
    }

    // Loop in search of things to load in the queue
    while(true)
    {
        bool bIsAbstract = false;
        CAbstractStreaming::AbctFileHandle* sfile = nullptr;
        
        // Wait until there's something to be loaded...
        WaitForSingleObject(*pSemaphore, -1);
        
        // Take the stream index from the queue
        int i = GetFirstInQueue(pQueue);
        if(i == -1) continue;
        
        CdStream* cd = &((*ppStreams)[i]);
        cd->bInUse = true;          // Mark the stream as under work
        if(cd->status == 0)
        {
            // Setup vars
            uint32_t bsize  = cd->nSectorsToRead;
            uint64_t offset = uint64_t(cd->nSectorOffset)  << 11;   // translate 2KiB based offset to actual offset
            uint32_t size   = uint32_t(bsize) << 11;                // translate 2KiB based size to actual size
            HANDLE hFile    = (HANDLE) cd->hFile;
            bool bResult    = false;
            const char* filename = nullptr; int index = -1; // When abstract those fields are valid
            
            // Try to find abstract file from hFile
            if(true)
            {
                scoped_lock xlock(streaming->cs);
                auto it = std::find(streaming->stm_files.begin(), streaming->stm_files.end(), hFile);
                if(it != streaming->stm_files.end())
                {
                    bIsAbstract = true;
                    
                    // Setup vars based on abstract file
                    sfile  = &(*it);
                    offset = 0;
                    size   = (uint32_t) sfile->info.file->size;
                    bsize  = GetSizeInBlocks(size);
                    index  = sfile->index;
                    filename = sfile->info.file->filepath();
                }
            }
            
#if !defined(NDEBUG) && 0
            plugin_ptr->Log("$$$$$$$$$ CdStreamThread streaming (%u) model %u offset %llu size %u", bIsAbstract,   
                                                                                ReadMemory<uint32_t>(raw_ptr(0x8E4A60 + (i * 38 * 4)/*(i * 0xB480)*/)),
                                                                                offset, size);                                                       
#endif

            // Setup overlapped structure
            LARGE_INTEGER offset_li;
            DWORD nBytesReaden;
            offset_li.QuadPart        = offset;
            cd->overlapped.Offset     = offset_li.LowPart;
            cd->overlapped.OffsetHigh = offset_li.HighPart;
            
            // Read the stream
            if(ReadFile(hFile, cd->lpBuffer, size, &nBytesReaden, &cd->overlapped))
            {
                bResult = true;
            }
            else
            {
                if(GetLastError() == ERROR_IO_PENDING)
                {
                    // This happens when the stream was open for async operations, let's wait until everything has been read
                    // As noted on MSDN [http://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx], when
                    // you open a stream with FILE_FLAG_NO_BUFFERING  (R* does that with their cd streams) it gives you maximum
                    // performance if you use overlapped I/O
                    bResult = GetOverlappedResult(hFile, &cd->overlapped, &nBytesReaden, true) != 0;
                }
            }
            
            // There's some real problem if we can't load a abstract model
            if(bIsAbstract && !bResult)
                plugin_ptr->Log("Warning: Failed to load abstract model file %s; error code: 0x%X", filename, GetLastError());
            

            // Set the cdstream status, 0 for "okay" and 254 for "failed to read"
            cd->status = bResult? 0 : 254;
        }
        
        // Remove from the queue what we just readed
        RemoveFirstInQueue(pQueue);
        
        // Cleanup
        if(bIsAbstract) streaming->CloseModel(sfile);

        if(plugin_ptr->loader->game_id == MODLOADER_GAME_RE3 || plugin_ptr->loader->game_id == MODLOADER_GAME_REVC)
        {
            // no CdStream sync fix for RE3
            cd->nSectorsToRead = 0;
            if(cd->bLocked) ReleaseSemaphore(cd->sync.semaphore, 1, 0);
            cd->bInUse = false;
        }
        else
		{
			// This critical section fixes a deadlock with CdStreamThread present in original code
			scoped_lock xlock(streaming->cdStreamSyncLock);

			cd->nSectorsToRead = 0;
			streaming->cdStreamSyncFuncs.Wake(cd);
			cd->bInUse = false;
		}
    }
    return 0;
}


/*
 *  CAbstractStreaming::GetCdStreamPath
 *      Gets the actual file path for the specified filepath
 *      For example it may find a new gta3.img path for "MODELS/GTA3.IMG"
 */
const char* CAbstractStreaming::GetCdStreamPath(const char* filepath_)
{
    if(filepath_)   // If null it's our abstract cd
    {
        std::string fpath; bool bBreak = false;
        auto filepath = NormalizePath(filepath_);
        auto filename = filepath.substr(GetLastPathComponent(filepath));

        // Check twice, the first time check for path equality, the second time for filename equality
        for(int i = 0; i < 2 && !bBreak; ++i)
        {
            for(auto& file : this->imgFiles)
            {
                auto& cdpath    = (i == 0? filepath : filename);                    // Sent filepath/filename
                fpath           = (i == 0? file->filedir() : file->filename());    // Custom filepath/filename

                // Check if the ending of cdpath is same as fpath
                if(fpath.length() >= cdpath.length() && std::equal(cdpath.rbegin(), cdpath.rend(), fpath.rbegin()))
                {
                    // Yeah, let's override!!!
                    filepath_ = file->filepath();
                    bBreak = true;
                    break;
                }
            }
        }

        return filepath_;
    }

    // It's abstract, ignore name but actually have a valid openable name
    return modloader::szNullFile;
}

/*
 *  CAbstractStreaming::GetCdDirectoryPath
 *      "MODELS/GTA3.DIR" -> ...
 *
 *      WARNING: The returned path buffer is very temporary.
*/
const char* CAbstractStreaming::GetCdDirectoryPath(const char* filepath_)
{
    if(gvm.IsSA()) // on SA the directory is on the header of the stream.
    {
        return GetCdStreamPath(filepath_);
    }

    if(filepath_)
    {
        // Very dirty, I cannot afford the time to implement something better.

        static std::string temp_dirpath;
        size_t pos;

        temp_dirpath = filepath_;
        if((pos = temp_dirpath.rfind('.')) != std::string::npos)
            temp_dirpath.replace(pos+1, 3, "img");

        const char* imgpath = GetCdStreamPath(temp_dirpath.c_str());

        temp_dirpath = imgpath;
        if((pos = temp_dirpath.rfind('.')) != std::string::npos)
            temp_dirpath.replace(pos+1, 3, "dir");

        filepath_ = temp_dirpath.c_str();
        return filepath_;
    }

    // It's abstract, ignore name but actually have a valid openable name
    return modloader::szNullFile;
}


/*
 *  CAbstractStreaming::TryOpenAbstractHandle
 *      Returns another file from the abstract streaming or returns the received file 
 */
HANDLE CAbstractStreaming::TryOpenAbstractHandle(int index, HANDLE hFile)
{
    CAbstractStreaming::AbctFileHandle* f = nullptr;
    
    // Try to find the object index in the import list
    auto it = streaming->imports.find(index);
    if(it != streaming->imports.end())
    {
         // Don't use our custom model if we're falling back to the original file because of an error
        if(it->second.isFallingBack == false)
            f = streaming->OpenModel(it->second, it->first);
    }
    
    // Returns the file from the abstract streaming if available
    return (f? f->handle : hFile);
}

/*
 *  CAbstractStreaming::OpenModel
 *      Opens a abstract model file handle 
 */
auto CAbstractStreaming::OpenModel(ModelInfo& file, int index) -> AbctFileHandle*
{
    DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN | (*pStreamCreateFlags & FILE_FLAG_OVERLAPPED);
    
    HANDLE hFile = CreateFileA(file.file->fullpath(fbuffer).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, flags, NULL);
    
    if(hFile == INVALID_HANDLE_VALUE)
    {
        plugin_ptr->Log("Warning: Failed to open file \"%s\" for abstract streaming; error code: 0x%X",
                       file.file->filepath(), GetLastError());
        return nullptr;
    }
    else
    {
        scoped_lock xlock(this->cs);
        return &(*this->stm_files.emplace(stm_files.end(), hFile, file, index));
    }
}

/*
 *  CAbstractStreaming::CloseModel
 *      Closes a abstract model file handle 
 */
void CAbstractStreaming::CloseModel(AbctFileHandle* file)
{
    scoped_lock xlock(this->cs);
    
    // Close the file handle
    CloseHandle(file->handle);
    
    // Remove this file from the open files list
    this->stm_files.remove(*file);
}

/*
 *  CAbstractStreaming::DoesModelNeedsFallback
 *      If the specified model id is under our control, make entirely sure it's present on disk.
 *      If it isn't, return false for fallback.
 */
bool CAbstractStreaming::DoesModelNeedsFallback(id_t index)
{
    auto it = imports.find(index);
    if(it != imports.end())
    {
        auto& m = it->second;
        if(m.isFallingBack == false)    // If already falling back, don't check for file existence again
        {
            // If file isn't on disk we should fall back to the stock model
            if(!IsPath(m.file->fullpath(fbuffer).c_str()))
                return true;
        }
    }
    return false;
}


/*
 *  CAbstractStreaming::BuildPrevOnCdMap
 *      Builds map to find out nextOnCd defaults field for InfoForMOdel
 */
void CAbstractStreaming::BuildPrevOnCdMap()
{
    CStreamingInfo* res;
    prev_on_cd.clear();
    for(id_t i = 0; res = InfoForModel(i); ++i)
    {
        id_t nextOnCd = res->GetNextOnCd();
        if(nextOnCd != -1) prev_on_cd.emplace(nextOnCd, i);
    }
}

// TODO comment and/or move to another file
void CAbstractStreaming::LoadCdDirectory(std::function<void()> OriginalLoadCdDirectory)
{
    plugin_ptr->Log("Initializing the streaming...");

    // III/VC reinitialize streaming on reloading
    if(!gvm.IsSA())
        this->bHasInitializedStreaming = false;

    // Those two pointers are acquired here because a limit adjuster mightn't have produced
    // its patches during our Patch()
    ms_aInfoForModel = ReadMemory<CStreamingInfo*>(0x40D014, true);
    clothesDirectory = gvm.IsSA()? ReadMemory<CDirectory*>(lazy_ptr<0x5A419B>(), true) : nullptr;

    // TODO should this be on Patch();
    if(plugin_ptr->loader->game_id == MODLOADER_GAME_RE3)
        LoadCdDirectory2 = modloader_re3->re3_addr_table->CStreaming__LoadCdDirectory2;
    else if (plugin_ptr->loader->game_id == MODLOADER_GAME_REVC)
        LoadCdDirectory2 = modloader_reVC->reVC_addr_table->CStreaming__LoadCdDirectory2;
    else
        LoadCdDirectory2 = ReadRelativeOffset(0x5B8310 + 1).get<void(const char*, int)>();


    //
    this->InitialiseStructAbstraction();

    // Remove non-streamed resources that are still puliting the raw_models list.
    this->RemoveNonStreamedFromRawModels();

    // Load standard cd directories.....
    TempCdDir_t tmp_cd_dir;
    this->fetch_to_cd_dir = &tmp_cd_dir;
    this->FetchCdDirectories(tmp_cd_dir, OriginalLoadCdDirectory);
    this->LoadCdDirectories(tmp_cd_dir);
    this->BuildPrevOnCdMap();
    tmp_cd_dir.clear();
    this->fetch_to_cd_dir = nullptr;

    // Clear imports in case this is not the first launch
    this->imports.clear();

    // Do custom setup
    this->BuildClothesMap();                                // Find out clothing hashes and remove clothes from raw_models
    this->LoadAbstractCdDirectory(refs_mapped(raw_models)); // Load abstract directory, our custom files

    // Mark streaming as initialized
    this->bIsFirstLaunched = true;
    this->bHasInitializedStreaming = true;

    // We can discard it in SA but we still need it in III and VC
    if(gvm.IsSA())
        this->raw_models.clear();
}

void CAbstractStreaming::OnRequestSpecialModel(id_t model_id, const char* model_name, uint32_t pos, uint32_t size)
{
    // Before this being called the game built the InfoForModel entry for this index based on the stock entry pRQSpecialEntry
    // Register stock entry, so it can be restored during gameplay if necessary
    {
        DirectoryInfo entry;
        uint32_t img_id;

        strncpy(entry.m_szFileName, model_name, sizeof(entry.m_szFileName));
        entry.m_szFileName[sizeof(entry.m_szFileName) - 1] = 0;
        entry.m_usCompressedSize__ = 0;
        entry.m_usSize = size; // FIXME for RE3 this is only compatible in little-endian
        entry.m_dwFileOffset = pos & 0xFFFFFF;
        img_id = pos >> 24;

        // FIXME m_szFileName can overflow with the following strcat!
        this->RegisterStockEntry(strcat(entry.m_szFileName, ".dff"), entry, model_id, img_id);
    }

    // Try to find abstract special entry related to this request....
    // If it's possible, quickly import our entry into this special index
    // otherwise quickly remove our previous entry at this special index if any there
    auto it = this->special.find(mhash(model_name));
    if(it != this->special.end())
        this->QuickImport(model_id, it->second, true);
    else
        this->QuickUnimport(model_id);
}

/*
 *  CAbstractStreaming::Patch
 *      Patches the default game streaming pipeline to have our awesome hooks.
 */
void CAbstractStreaming::Patch()
{
    using namespace std::placeholders;

    const auto game_id = plugin_ptr->loader->game_id;

    if(game_id == MODLOADER_GAME_RE3)
    {
        const auto* modloader_re3_shdata = plugin_ptr->loader->FindSharedData("MODLOADER_RE3");
        assert(modloader_re3_shdata != nullptr);
        assert(modloader_re3_shdata->type == MODLOADER_SHDATA_POINTER);
        modloader_re3 = (modloader_re3_t*) modloader_re3_shdata->p;
        assert(modloader_re3 != nullptr);
    }
    else if(game_id == MODLOADER_GAME_REVC)
    {
        const auto* modloader_reVC_shdata = plugin_ptr->loader->FindSharedData("MODLOADER_REVC");
        assert(modloader_reVC_shdata != nullptr);
        assert(modloader_reVC_shdata->type == MODLOADER_SHDATA_POINTER);
        modloader_reVC = (modloader_reVC_t*)modloader_reVC_shdata->p;
        assert(modloader_reVC != nullptr);
    }

    // Pointers that we should have before streaming initialization.
    pStreamCreateFlags  = memory_pointer(0x8E3FE0).get();
    pStreamingBuffer    = memory_pointer(0x8E4CAC).get<void*>();
    streamingBufferSize = memory_pointer(0x8E4CA8).get<uint32_t>();

    // See data.cpp
    this->DataPatch();

    // Hook at the initialization of the streaming
    if(game_id == MODLOADER_GAME_RE3)
    {
        modloader_re3->callback_table->LoadCdDirectory0 = +[] {
            streaming->LoadCdDirectory(modloader_re3->re3_addr_table->CStreaming__LoadCdDirectory0);
        };

        modloader_re3->callback_table->FetchCdDirectory = +[](const char* filename, int id) {
            assert(streaming->fetch_to_cd_dir != nullptr);
            return streaming->FetchCdDirectory(*streaming->fetch_to_cd_dir, filename, id);
        };
    }
    else if(game_id == MODLOADER_GAME_REVC)
    {
        modloader_reVC->callback_table->LoadCdDirectory0 = +[] {
            streaming->LoadCdDirectory(modloader_reVC->reVC_addr_table->CStreaming__LoadCdDirectory0);
        };

        modloader_reVC->callback_table->FetchCdDirectory = +[](const char* filename, int id) {
            assert(streaming->fetch_to_cd_dir != nullptr);
            return streaming->FetchCdDirectory(*streaming->fetch_to_cd_dir, filename, id);
        };
    }
    else
    {
        using sinit_hook = function_hooker<0x5B8E1B, void()>;
        make_static_hook<sinit_hook>(std::bind(&CAbstractStreaming::LoadCdDirectory, this, _1));
    }

    // Standard models
    if(true)
    {
        // Making our our code for the stream thread would make things so much better
        if(game_id == MODLOADER_GAME_RE3)
            modloader_re3->callback_table->CdStreamThread = +[] { CdStreamThread(); };
        else if(game_id == MODLOADER_GAME_REVC)
            modloader_reVC->callback_table->CdStreamThread = +[] { CdStreamThread(); };
        else
            MakeJMP(0x406560, raw_ptr(CdStreamThread));

        // We need to know the next model to be read before the CdStreamRead call happens
        if(gvm.IsSA())
        {
#ifndef _WIN64
            MakeCALL(0x40CCA6, raw_ptr(HOOK_RegisterNextModelRead));
#endif
            MakeNOP(0x40CCA6 + 5, 2);
        }
        else if(gvm.IsVC())
        {
#ifndef _WIN64
            MakeCALL(0x40CCA6, raw_ptr(HOOK_RegisterNextModelRead_VC));
            MakeCALL(xVc(0x40B738), raw_ptr(HOOK_RegisterNextModelRead_VC));
#endif
            MakeNOP(0x40CCA6 + 5, 1);
            MakeNOP(xVc(0x40B738) + 5, 1);
        }
        else if(gvm.IsIII())
        {
            using xcd_hook1 = function_hooker_thiscall<xIII(0x40A128), char(CStreamingInfo*, int*, int*)>;
            using xcd_hook2 = function_hooker_thiscall<xIII(0x40A4F3), char(CStreamingInfo*, int*, int*)>;

            auto fn_register = [this](xcd_hook1::func_type GetCdPosnAndSize, CStreamingInfo*& model, int*& pOffset, int*& pSize)
            {
                RegisterNextModelRead(this->InfoForModelIndex(*model));
                return GetCdPosnAndSize(model, pOffset, pSize);
            };

            make_static_hook<xcd_hook1>(fn_register);
            make_static_hook<xcd_hook2>(fn_register);
        }
        else if(game_id == MODLOADER_GAME_RE3)
        {
            modloader_re3->callback_table->RegisterNextModelRead = RegisterNextModelRead;
        }
        else if(game_id == MODLOADER_GAME_REVC)
        {
            modloader_reVC->callback_table->RegisterNextModelRead = RegisterNextModelRead;
        }

        // We need to return a new hFile if the file is on disk
        if(gvm.IsSA())
        {
#ifndef _WIN64
            MakeCALL(0x406A5B, raw_ptr(HOOK_NewFile));
#endif
            MakeNOP(0x406A5B + 5, 1);
        }
        else if(gvm.IsVC() || gvm.IsIII())
        {
#ifndef _WIN64
            MakeCALL(xVc(0x408521), raw_ptr(HOOK_NewFile_3VC));
#endif
        }
        else if(game_id == MODLOADER_GAME_RE3)
        {
            modloader_re3->callback_table->AcquireNextModelFileHandle = +[] {
                return CallGetAbstractHandle(INVALID_HANDLE_VALUE);
            };
        }
        else if(game_id == MODLOADER_GAME_REVC)
        {
            modloader_reVC->callback_table->AcquireNextModelFileHandle = +[] {
                return CallGetAbstractHandle(INVALID_HANDLE_VALUE);
            };
        }

        if(gvm.IsIII() || gvm.IsVC() || gvm.IsSA())
        {
            using cdread_hook = function_hooker<0x40CF34, int(int, void*, int, int)>;
            using cdread_hook2 = function_hooker<xVc(0x40B76A), int(int, void*, int, int)>;
            using cdread_hook3 = function_hooker<xVc(0x40B780), int(int, void*, int, int)>;

            // TODO III VC see unhandled call to CdStreamRead at gta-vc:0x627D50

            // We need to know the model index that will pass throught CallGetAbstractHandle
            auto f = [](cdread_hook::func_type CdStreamRead, int& streamNum, void*& buf, int& sectorOffset, int& sectorCount)
            {
                iModelBeingLoaded = iNextModelBeingLoaded;
                auto result = CdStreamRead(streamNum, buf, sectorOffset, sectorCount);
                iModelBeingLoaded = iNextModelBeingLoaded = -1;
                return result;
            };

            make_static_hook<cdread_hook>(f);

            if(gvm.IsVC() || gvm.IsIII())
            {
                make_static_hook<cdread_hook2>(f);
                make_static_hook<cdread_hook3>(f);
            }
        }
        else if(game_id == MODLOADER_GAME_RE3)
        {
            // TODO this hook can be simplified in RE3. Perhaps we just need a callback after CdStreamRead.
            modloader_re3->callback_table->CdStreamRead = +[](int32_t channel, void* buf, uint32_t sectorOffset, uint32_t sectorCount) {
                iModelBeingLoaded = iNextModelBeingLoaded;
                auto result = modloader_re3->re3_addr_table->CdStreamRead(channel, buf, sectorOffset, sectorCount);
                iModelBeingLoaded = iNextModelBeingLoaded = -1;
                return result;
            };
        }
        else if(game_id == MODLOADER_GAME_REVC)
        {
            // TODO this hook can be simplified in REVC. Perhaps we just need a callback after CdStreamRead.
            modloader_reVC->callback_table->CdStreamRead = +[](int32_t channel, void* buf, uint32_t sectorOffset, uint32_t sectorCount) {
                iModelBeingLoaded = iNextModelBeingLoaded;
                auto result = modloader_reVC->reVC_addr_table->CdStreamRead(channel, buf, sectorOffset, sectorCount);
                iModelBeingLoaded = iNextModelBeingLoaded = -1;
                return result;
            };
        }
    }

    // Special models
    if(gvm.IsSA() || gvm.IsVC() || gvm.IsIII())
    {
        static DirectoryInfo* pRQSpecialEntry;    // Stores the special entry we're working with
        using findspecial_hook  = function_hooker_thiscall<0x409F76, char(void*, const char*, unsigned int&, unsigned int&)>;
        using rqspecial_hook    = function_hooker<0x409FD9, char(int, int)>;
        
        // Hook call to CDirectory::FindItem (1) to find out which directory entry we're dealing with...
        make_static_hook<findspecial_hook>([this](findspecial_hook::func_type FindItem, void*& dir, const char*& name, unsigned int& offset, unsigned int& blocks)
        {
            pRQSpecialEntry = injector::thiscall<DirectoryInfo*(void*, const char*)>::call<0x532450>(dir, name);  // CDirectory::FindItem
            return FindItem(dir, name, offset, blocks); // (above call and this call are different)
        });

        // Hook call to CStreaming::RequestModel and do special processing if the entry is related to the abstract streaming
        make_static_hook<rqspecial_hook>([this](rqspecial_hook::func_type RequestModel, int& index, int& flags)
        {
            OnRequestSpecialModel(index, pRQSpecialEntry->m_szFileName, pRQSpecialEntry->m_dwFileOffset, pRQSpecialEntry->m_usSize);
            return RequestModel(index, flags);
        });
    }
    else if(game_id == MODLOADER_GAME_RE3)
    {
        modloader_re3->callback_table->OnRequestSpecialModel = +[](id_t model_id, const char* model_name, uint32_t pos, uint32_t size)
        {
            return streaming->OnRequestSpecialModel(model_id, model_name, pos, size);
        };
    }
    else if(game_id == MODLOADER_GAME_REVC)
    {
        modloader_reVC->callback_table->OnRequestSpecialModel = +[](id_t model_id, const char* model_name, uint32_t pos, uint32_t size)
        {
            return streaming->OnRequestSpecialModel(model_id, model_name, pos, size);
        };
    }

    // Clothes
    if(gvm.IsSA())
    {
        using log_hook = function_hooker<0x532341, int(const char*)>;
        using ldclothd_hook = function_hooker_thiscall<0x5A6A01, void(CDirectory*, const char*)>;
        using rqcloth_hook  = function_hooker<0x40A106, char(int, int)>;

        // When reading clothes directory again (it happens everytime player clump is rebuilt, i.e. when muscle stats change or clothes change)
        // make sure the entries are fine with our entries
        make_static_hook<ldclothd_hook>([this](ldclothd_hook::func_type ReadDir, CDirectory*& dir, const char*& filepath)
        {
            ReadDir(dir, filepath);
            this->FixClothesDirectory();
        });

        // Right, this hook produces the actual overriding of the clothing files with ours
        make_static_hook<rqcloth_hook>([this](rqcloth_hook::func_type RequestModel, int& index, int& flags)
        {
            // Check if we have a overrider for this clothing item (based on directory offset)
            DirectoryInfo* entry;
            auto it = this->clothes.find(InfoForModel(index)->GetOffset());
            if(it != clothes.end() && (entry = this->FindClothEntry(it->second->hash)))
            {
                // Yep, we have a overrider, save stock entry and quickly import our abstract model
                this->RegisterStockEntry(it->second->filename(), *entry, index, InfoForModel(index)->GetImgId());
                this->QuickImport(index, it->second, false, true);
            }
            else
            {
                // Hmmm.. nope, quickly unimport anything we had imported on this index before
                this->QuickUnimport(index);
            }

            return RequestModel(index, flags);
        });

        // Make sure to log about "Too many objects without model info structure" into our logging stream
        make_static_hook<log_hook>([](log_hook::func_type printf, const char* msg)
        {
            plugin_ptr->Log("Warning: %s", msg);
            return printf(msg);
        });
    }

    // Fixes for stores that do not return the previous registered entry, always registers a new one
    if(true)
    {
        using addcol_hook = function_hooker<0x5B630B, id_t(const char*)>;
        using addr3_hook  = function_hooker<0x5B63E8, id_t(const char*)>;
        using addscm_hook = function_hooker_thiscall<0x5B6419, id_t(void*, const char*)>;

        // Although streamed COLs exist in Vice too, they are checked, so we don't need to check ourselves
        if(gvm.IsSA())
        {
            TraitsSA traits; // see comment above

            // CColStore finding method is dummie, so we need to avoid duplicate cols by ourselves
            make_static_hook<addcol_hook>([this, traits](addcol_hook::func_type AddColSlot, const char*& name)
            {
                return this->FindOrRegisterResource(name, "col", traits.col_start, AddColSlot, name);
            });
        }

        // The following files are in SA only
        if(gvm.IsSA())
        {
            TraitsSA traits;

            // CVehicleRecording do not care about duplicates, but we should
            make_static_hook<addr3_hook>([this, traits](addr3_hook::func_type RegisterRecordingFile, const char*& name)
            {
                return this->FindOrRegisterResource(name, "rrr", traits.rrr_start, RegisterRecordingFile, name);
            });
            
            // CStreamedScripts do not care about duplicates but we should
            make_static_hook<addscm_hook>([this, traits](addscm_hook::func_type RegisterScript, void*& self, const char*& name)
            {
                return this->FindOrRegisterResource(name, "scm", traits.scm_start, RegisterScript, self, name);
            });
        }
    }

    // CdStream path overiding
    if(gvm.IsSA() || gvm.IsVC() || gvm.IsIII())
    {
        static void*(*OpenFile)(const char*, const char*);
        static void*(*RwStreamOpen)(int, int, const char*);

        auto OpenFileCdStreamHook = [](const char* filename, const char* mode)
        {
            return OpenFile(streaming->GetCdStreamPath(filename), mode);
        };

        auto OpenFileCdDirectoryHook = [](const char* filename, const char* mode)
        {
            return OpenFile(streaming->GetCdDirectoryPath(filename), mode);
        };

        auto RwStreamOpenCdStreamHook = [](int a, int b, const char* filename)
        {
            return RwStreamOpen(a, b, streaming->GetCdStreamPath(filename));
        };

        auto RwStreamOpenCdDirectoryHook = [](int a, int b, const char* filename)
        {
            return RwStreamOpen(a, b, streaming->GetCdDirectoryPath(filename));
        };

        // Resolve the cd stream filenames by ourselves on the following calls
        {
            auto pOpenFileCdStream     = raw_ptr((decltype(OpenFile))(OpenFileCdStreamHook));
            auto pOpenFileCdDirectory  = raw_ptr((decltype(OpenFile))(OpenFileCdDirectoryHook));

            OpenFile = MakeCALL(0x5B6183, pOpenFileCdDirectory).get();
            MakeCALL(0x532361, pOpenFileCdDirectory);
            MakeCALL(0x5AFC9D, pOpenFileCdStream);
            if(gvm.IsIII() || gvm.IsVC()) MakeCALL(xVc(0x627D79), pOpenFileCdDirectory);

            auto pRwStreamOpenCdStream = raw_ptr((decltype(RwStreamOpen))(RwStreamOpenCdStreamHook));
            if(!gvm.IsIII()) RwStreamOpen = MakeCALL(0x5AFBEF, pRwStreamOpenCdStream).get();
            if(gvm.IsSA()) MakeCALL(0x5B07E9, pRwStreamOpenCdStream);
            if(gvm.IsIII()) RwStreamOpen = MakeCALL(xIII(0x4BA6F6), pRwStreamOpenCdStream).get();
        }

        // Pointers to archieve the ds:[CreateFileA] overriding, we also have to deal with SecuROM obfuscation there!
        static void* pCreateFileForCdStream = (void*) &CreateFileForCdStream;

        if(gvm.IsSA() && gvm.IsHoodlum())
        {
            static uintptr_t SRXorCreateFileForCdStream = 0x214D4C48 ^ (uintptr_t)(&pCreateFileForCdStream);
            memory_pointer_raw p;

            if(gvm.IsUS())
                p = raw_ptr(0x01564B56 + 1);
            else if(gvm.IsEU())
                p = raw_ptr(0x01564AED + 1);
            else
                plugin_ptr->Error("SRXorCreateFileForCdStream patch failed");

            injector::WriteMemory(p, &SRXorCreateFileForCdStream, true);
        }
        else
        {
            injector::WriteMemory(0x40685E + 2, &pCreateFileForCdStream, true);
        }
    }
    else if(game_id == MODLOADER_GAME_RE3)
    {
        modloader_re3->callback_table->GetCdDirectoryPath_Unsafe = +[](const char* filepath) {
            return streaming->GetCdDirectoryPath(filepath);
        };

        modloader_re3->callback_table->GetCdStreamPath_Unsafe = +[](const char* filepath) {
            return streaming->GetCdStreamPath(filepath);
        };
    }
    else if(game_id == MODLOADER_GAME_REVC)
    {
        modloader_reVC->callback_table->GetCdDirectoryPath_Unsafe = +[](const char* filepath) {
            return streaming->GetCdDirectoryPath(filepath);
        };

        modloader_reVC->callback_table->GetCdStreamPath_Unsafe = +[](const char* filepath) {
            return streaming->GetCdStreamPath(filepath);
        };
    }

    // Some fixes to allow the refreshing process to happen
    if(gvm.IsSA())
    {
        // Fix issue with CBike having some additional fields on SetupSuspensionLines that gets deallocated when
        // we destroy it's model or something. Do just like CQuad and other does, checks if the pointer is null and then allocate it.
        MakeNOP(0x6B89CE, 6);
#ifndef _WIN64
        MakeCALL(0x6B89CE, HOOK_FixBikeSuspLines);
#endif
    }

    // Disable txd.img / txd.dir
    if(gvm.IsIII() || gvm.IsVC())
    {
        // Make the fopen that checks if txd.img exists return NULL (does not exist).
        MakeCALL(xVc(0x410814), raw_ptr(return_value<void*, nullptr>));
        // Do not let __CreateCacheTxdImage do anything, and return that cache wasn't created (false).
        MakeCALL(xVc(0x41083A), raw_ptr(return_value<bool, false>));

        // Do not even let the code above run!
        // Important because when __CreateCacheTxd fails on VC, bad things seem to happen.
        if(gvm.IsVC())
            MakeCALL(xVc(0x410801), raw_ptr(return_value<bool, true>));
    }

    // CdStream race condition fix
    if(gvm.IsSA() || gvm.IsVC() || gvm.IsIII())
    {
        // These are required so we can fix CdStream race condition
        MakeJMP(0x406460, raw_ptr(CdStreamSync));
        if(gvm.IsSA())
        {
            const uint8_t mem[] = { 0xFF, 0x15 };
            WriteMemoryRaw(0x406910, mem, sizeof(mem), true);
            WriteMemory(0x406910 + 2, &streaming->cdStreamSyncFuncs.Initialize, true);
            MakeNOP(0x406910 + 6, 4);
            MakeNOP(0x406910 + 0x16, 2);
        }
        else if(gvm.IsVC() || gvm.IsIII())
        {
            MakeNOP(xVc(0x4088F7), 8);
            WriteMemory(xVc(0x4088F7) + 10, &streaming->cdStreamSyncFuncs.Initialize, true);
            WriteMemory(xVc(0x408919), uint8_t(0xEB), true);
        }

        if(gvm.IsSA())
        {
            const uint8_t mem[] = { 0x56, 0x50 };
            WriteMemoryRaw(0x4063B5, mem, sizeof(mem), true);
            MakeCALL(0x4063B5 + 2, raw_ptr(CdStreamShutdownSync_Stub), true);
        }
        else if(gvm.IsVC() || gvm.IsIII())
        {
            const uint8_t mem[] = { 0x8D, 0x04, 0x29, 0x90 };
            WriteMemoryRaw(xVc(0x4086B6), mem, sizeof(mem), true);
            WriteMemory(xVc(0x4086B6) + 5 + 2, &streaming->cdStreamSyncFuncs.Shutdown, true);
        }
    }
}

/*
*  Export for SilentPatch so it knows that this ML version patches the race condition
*/
extern "C" __declspec(dllexport)
uint32_t CdStreamRaceConditionAware()
{
	return 1;
}

