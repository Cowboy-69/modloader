/* 
 * Standard Streamer Plugin for Mod Loader
 * Copyright (C) 2014  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under the MIT License, see LICENSE at top level directory.
 * 
 *      Low-level stuff for Microsoft Visual Studio
 *
 */
#include <stdinc.hpp>
#include "streaming.hpp"

#ifndef _WIN64
extern "C" {

/* Hooks and sub calls */
extern void* CallGetAbstractHandle(void*);

/* vars */
//extern void* ms_aInfoForModel;
extern void* (*ColModelPool_new)(int);

/* funcs */
extern char* AllocBufferForString(const char*);
extern void RegisterNextModelRead(int id);
extern void FixBikeSuspPtr(char* m_pColData);

/*
    void* _nakedcall _cdecl HOOK_LoadColFileFix(arg0 = size)
        Fixes the CFileLoader::LoadCollisionFile method to work properly
*/
void __declspec(naked) HOOK_LoadColFileFix(int size)
{
    _asm
    {
        /* Perform the original operation (new ColModel) */
        push [esp+4]
        mov eax, ColModelPool_new
        call eax
        add esp, 4

        /* Now, the fix is here, edi should contain the ColModel pointer, but it doesn't! 
           Let's fix it     */
        mov edi, eax
        ret
    }
}



/*
    void _nakedcall HOOK_RegisterNextModelRead(esi = objectIndex, eax = someOffsetAt_aInfoForModel)
        Used to tell us which model will get loaded ahead, so we know if it's a imported one
*/
void __declspec(naked) HOOK_RegisterNextModelRead()
{
    _asm
    {
        pushad
        push esi
        call RegisterNextModelRead
        add esp, 4
        popad

        /* Run replaced code: */
        mov edx, dword ptr[ms_aInfoForModel]
        mov edx, [edx + 0xC + eax * 4]    /* edx = ms_aInfoForModel[iLoadingModelIndex].blocks */
        ret
    }
}

/*
    void _nakedcall HOOK_RegisterNextModelRead(ebx = objectIndex, ecx = someOffsetAt_aInfoForModel)
    Used to tell us which model will get loaded ahead, so we know if it's a imported one
*/
void __declspec(naked) HOOK_RegisterNextModelRead_VC()
{
    _asm
    {
        pushad
        xor edx, edx
        mov eax, ecx
        mov ecx, 0x14  // sizeof CStreamingInfo on VC
        div ecx
        push eax // result of div
        call RegisterNextModelRead
        add esp, 4
        popad

        /* Run replaced code: */
        mov edx, dword ptr[ms_aInfoForModel]
        mov edx, [edx+ecx+0x10] // field for VC
        //mov edx, [edx + 0xC + eax * 4]    /* edx = ms_aInfoForModel[iLoadingModelIndex].blocks */
        ret
    }
}

/*
    HANDLE _nakedcall HOOK_NewFile(eax = hOriginalFile, esi = blockOffset)
        Returns the file handle to the file that will get read to get an object data
        Normally it will return it's original handle, but if a custom file (put in a modloader folder) it will return a new unique handle
*/
void __declspec(naked) HOOK_NewFile()
{
    _asm
    {
        and esi, 0x00FFFFFF     /* Original code */
        push eax
        call CallGetAbstractHandle
        add esp, 4
        ret
    }
}

/*
    
*/
void __declspec(naked) HOOK_NewFile_3VC()
{
    _asm
    {
        push ebp
        call CallGetAbstractHandle
        add esp, 4
        mov ebp, eax

        add esi, edi        /* Original code */
        mov [esi+0x18], ebp /* Original code */
        ret
    }
}


/*
    void _nakedcall HOOK_FixBikeSuspLines(edi = v2->clump.base.m_pColModel->m_pColData, ...)
        Fixes broken pointer on CBike::SetupSuspensionLine after a refresh
*/
void __declspec(naked) HOOK_FixBikeSuspLines()
{
    _asm
    {
    _BikeSuspTry:
        mov eax, [edi+0x10]  /* Original Code */
        test eax, eax
        jz _BikeSuspFix
        mov edx, [eax+0x28]  /* Original Code */
        ret

    _BikeSuspFix:
        pushad
        push edi
        call FixBikeSuspPtr
        add esp, 4
        popad
        jmp _BikeSuspTry
    }
}


}   // extern "C"
#endif