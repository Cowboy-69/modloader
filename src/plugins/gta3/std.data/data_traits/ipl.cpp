/*
 * Copyright (C) 2015  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include "../data_traits.hpp"
using namespace modloader;

struct ipl_traits
{
    struct dtraits : modloader::dtraits::OpenFile
    {
        static const char* what() { return "scene"; }
    };
};

using OpenSceneDetour = modloader::OpenFileDetour<0x5B871A, ipl_traits::dtraits>;

const char* RegisterAndGetIplFilePathRE3(const char* filepath)
{
    static std::string static_result;

    std::string filepathString = static_cast<std::string>(filepath);
    auto filename = NormalizePath(filepathString.substr(GetLastPathComponent(filepathString)));

    DataPlugin* dataPlugin = static_cast<DataPlugin*>(plugin_ptr);
    auto it = dataPlugin->iplFiles.find(filename);
    if (it != dataPlugin->iplFiles.end())
    {
        std::string fullpath = it->second->fullpath();
        //dataPlugin->iplFiles.erase(it);
        static_result = fullpath;
    }

    return static_result.empty() ? filepath : static_result.c_str();
}

using namespace std::placeholders;
static auto xinit = initializer([](DataPlugin* plugin_ptr)
{
    if (plugin_ptr->loader->game_id == MODLOADER_GAME_RE3) {
        plugin_ptr->AddBehv(modloader::hash(ipl_merger_name), true);

        plugin_ptr->modloader_re3 = (modloader_re3_t*)plugin_ptr->loader->FindSharedData("MODLOADER_RE3")->p;
        plugin_ptr->modloader_re3->callback_table->RegisterAndGetIplFile_Unsafe = +[](const char* filepath) {
            return RegisterAndGetIplFilePathRE3(filepath);
        };
    } else {
        plugin_ptr->AddIplOverrider<OpenSceneDetour>(ipl_merger_name, false, false, true, no_reinstall);
    }
});
