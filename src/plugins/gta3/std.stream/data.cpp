/*
 * Standard Streamer Plugin for Mod Loader
 * Copyright (C) 2014  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 *
 */
#include <stdinc.hpp>
#include "streaming.hpp"
using namespace std::placeholders;

extern "C"
{
    void* (*ColModelPool_new)(int)  = nullptr;
#ifndef _WIN64
    extern void HOOK_LoadColFileFix(int);
#endif
};

static void FixColFile();
static void* LoadNonStreamedRes(std::function<void*(const char*)> load, const char* filepath, NonStreamedType type);
static const char* RegisterAndGetNonStreamedResPath(const char* filepath, NonStreamedType type);

/*
 *  gta.dat / default.dat related stuff
 */
void CAbstractStreaming::DataPatch()
{
    // Fix broken COLFILE in GTA SA
    FixColFile();

    // Detouring of non streamed resources loaded by gta.dat/default.dat
    if(gvm.IsIII() || gvm.IsVC() || gvm.IsSA())
    {
        if(true)
        {
            using hcolfile  = function_hooker<0x5B9188, void*(const char*, int)>;
            using hcolfile2 = function_hooker<xIII(0x476589), void*(const char*, int)>;
            using hclump    = function_hooker<0x5B91DB, void*(const char*)>;

            auto fn_hcolfile = [](hcolfile::func_type load, const char* filepath, int id)
            {
                // NOTE: on III `id` is not a parameter, but since this is __cdecl, we don't have a problem.
                return load(RegisterAndGetNonStreamedResPath(filepath, NonStreamedType::ColFile), id);
            };

            make_static_hook<hcolfile>(fn_hcolfile);
            if(gvm.IsIII()) make_static_hook<hcolfile2>(fn_hcolfile);

            make_static_hook<hclump>(std::bind(LoadNonStreamedRes, _1, _2, NonStreamedType::ClumpFile));
        }

        if(gvm.IsIII() || gvm.IsSA())
        {
            using hatomic = function_hooker<0x5B91B0, void*(const char*)>;
            using htexdict = function_hooker<0x5B910A, void*(const char*)>;
            make_static_hook<hatomic>(std::bind(LoadNonStreamedRes, _1, _2, NonStreamedType::AtomicFile));
            make_static_hook<htexdict>(std::bind(LoadNonStreamedRes, _1, _2, NonStreamedType::TexDictionary));
        }
        else if(gvm.IsVC())
        {
            using hatomic = function_hooker<xVc(0x48DB7B), void*(int, int, const char*)>;
            using htexdict = function_hooker<xVc(0x48DA46), void*(int, int, const char*)>;

            make_static_hook<hatomic>([](hatomic::func_type load, int a, int b, const char* filepath)
            {
                return load(a, b, RegisterAndGetNonStreamedResPath(filepath, NonStreamedType::AtomicFile));
            });

            make_static_hook<htexdict>([](htexdict::func_type load, int a, int b, const char* filepath)
            {
                return load(a, b, RegisterAndGetNonStreamedResPath(filepath, NonStreamedType::TexDictionary));
            });
        }
    }
    else if(plugin_ptr->loader->game_id == MODLOADER_GAME_RE3)
    {
        modloader_re3->callback_table->RegisterAndGetAtomicFile_Unsafe = +[](const char* filepath) {
            return RegisterAndGetNonStreamedResPath(filepath, NonStreamedType::AtomicFile);
        };

        modloader_re3->callback_table->RegisterAndGetClumpFile_Unsafe = +[](const char* filepath) {
            return RegisterAndGetNonStreamedResPath(filepath, NonStreamedType::ClumpFile);
        };

        modloader_re3->callback_table->RegisterAndGetTexDiction_Unsafe = +[](const char* filepath) {
            return RegisterAndGetNonStreamedResPath(filepath, NonStreamedType::TexDictionary);
        };

        modloader_re3->callback_table->RegisterAndGetColFile_Unsafe = +[](const char* filepath) {
            return RegisterAndGetNonStreamedResPath(filepath, NonStreamedType::ColFile);
        };
    }
    else if(plugin_ptr->loader->game_id == MODLOADER_GAME_REVC)
    {
        modloader_reVC->callback_table->RegisterAndGetAtomicFile_Unsafe = +[](const char* filepath) {
            return RegisterAndGetNonStreamedResPath(filepath, NonStreamedType::AtomicFile);
        };

        modloader_reVC->callback_table->RegisterAndGetClumpFile_Unsafe = +[](const char* filepath) {
            return RegisterAndGetNonStreamedResPath(filepath, NonStreamedType::ClumpFile);
        };

        modloader_reVC->callback_table->RegisterAndGetTexDiction_Unsafe = +[](const char* filepath) {
            return RegisterAndGetNonStreamedResPath(filepath, NonStreamedType::TexDictionary);
        };

        modloader_reVC->callback_table->RegisterAndGetColFile_Unsafe = +[](const char* filepath) {
            return RegisterAndGetNonStreamedResPath(filepath, NonStreamedType::ColFile);
        };
    }
}


/*
 *  CAbstractStreaming::TryLoadNonStreamedResource
 *      Checks if the file 'filepath' is under our ownership, and if it is register as non-streamed.
 */
std::string CAbstractStreaming::TryLoadNonStreamedResource(std::string filepath, NonStreamedType type)
{
    auto filename = NormalizePath(filepath.substr(GetLastPathComponent(filepath)));

    if(!this->bHasInitializedStreaming)
    {
        auto it = this->raw_models.find(filename);
        if(it != this->raw_models.end())
        {
            // Log about non streamed resource and make sure it's unique
            plugin_ptr->Log("Using non-streamed resource \"%s\"", it->second->filepath());
            if(this->non_stream.count(it->second->hash))
                throw std::runtime_error("std.stream: TryLoadNonStreamedResource: Repeated resource!");

            // Register into non_stream and unregister from raw_models
            this->non_stream.emplace(it->second->hash, std::make_pair(it->second, type));
            std::string fullpath = it->second->fullpath();
            this->raw_models.erase(it);
            return fullpath;
        }
    }

    if(true)
    {
        size_t filename_hash = modloader::hash(filename);
        auto it = this->non_stream.find(filename_hash);
        if(it != this->non_stream.end())
        {
            return it->second.first->fullpath();
        }
    }

    return std::string();
}

void CAbstractStreaming::RemoveNonStreamedFromRawModels()
{
    // This is only needed for III because of non-streamed .col/.ifp
    if(!gvm.IsIII() && plugin_ptr->loader->game_id != MODLOADER_GAME_RE3)
        return;

    for(auto it = this->raw_models.begin(); it != this->raw_models.end(); )
    {
        if(IsNonStreamed(it->second))
            it = this->raw_models.erase(it);
        else
            ++it;
    }
}

const char* RegisterAndGetNonStreamedResPath(const char* filepath, NonStreamedType type)
{
    static std::string static_result;
    static_result = streaming->TryLoadNonStreamedResource(filepath, type);
    return static_result.empty()? filepath : static_result.c_str();
}

void* LoadNonStreamedRes(std::function<void* (const char*)> load, const char* filepath, NonStreamedType type)
{
    return load(RegisterAndGetNonStreamedResPath(filepath, type));
}


/*
 *  FixColFile
 *      Fixes the COLFILE from gta.dat not working properly, crashing the game (GTA SA only).
 */
void FixColFile()
{
    if(!gvm.IsSA())
        return;

    static bool using_colbuf;           // Is using colbuf or original buffer?
    static bool empty_colmodel;         // Is this a empty colmodel?
    static std::vector<char> colbuf;    // Buffer for reading col file into

    // Prototype for patches
    using rcolinfo_f  = int(void*, uint32_t*, size_t);
    using rcolmodel_f = int(void*, char*, size_t);
    using lcol_f      = int(char*, int, void*, char*);
    using rel_f       = int(void*);

    // Fixes the crash caused by using COLFILE for a building etc
#ifndef _WIN64
    ColModelPool_new = MakeCALL(0x5B4F2E, HOOK_LoadColFileFix).get();
#endif

    // Reads collision info and check if we need to use our own collision buffer
    auto ReadColInfo = [](std::function<rcolinfo_f> Read, void*& f, uint32_t*& buffer, size_t& size)
    {
        auto r    = Read(f, buffer, size);
        empty_colmodel = (buffer[1] <= 0x18);
        if(using_colbuf = !empty_colmodel)
            colbuf.resize(buffer[1]);
        return r;
    };

    // Replace buffer if necessary
    auto ReadColModel = [](std::function<rcolmodel_f> Read, void*& f, char*& buffer, size_t& size)
    {
        if(!empty_colmodel)
            return Read(f, using_colbuf? colbuf.data() : buffer, size);
        return 0;
    };

    // Replace buffer if necessary
    auto LoadCollisionModel = [](std::function<lcol_f> LoadColModel, char*& buf, int& size, void*& colmodel, char*& modelname)
    {
        return LoadColModel(using_colbuf? colbuf.data() : buf, size, colmodel, modelname);
    };

    // Frees our buffer
    auto ReleaseBuffer = [](std::function<rel_f> fclose, void*& f)
    {
        colbuf.clear(); colbuf.shrink_to_fit();
        return fclose(f);
    };

    // Patches
    make_static_hook<function_hooker<0x5B4EF4, rcolmodel_f>>(ReadColModel);
    make_static_hook<function_hooker<0x5B4E92, rcolinfo_f>>(ReadColInfo);
    make_static_hook<function_hooker<0x5B4FCC, rcolinfo_f>>(ReadColInfo);
    make_static_hook<function_hooker<0x5B4FA0, lcol_f>>(LoadCollisionModel);
    make_static_hook<function_hooker<0x5B4FB5, lcol_f>>(LoadCollisionModel);
    make_static_hook<function_hooker<0x5B4F83, lcol_f>>(LoadCollisionModel);
    make_static_hook<function_hooker<0x5B92F9, rel_f>>(ReleaseBuffer);
}
