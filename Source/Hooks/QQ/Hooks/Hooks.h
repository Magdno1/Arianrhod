#ifndef _HOOKS_H_10758d45_3f47_4c54_9096_1d2ee621661a_
#define _HOOKS_H_10758d45_3f47_4c54_9096_1d2ee621661a_

#include "../QQ2011.h"

NTSTATUS HookAppUtil(PVOID BaseAddress);
NTSTATUS HookCommon(PVOID BaseAddress);
NTSTATUS HookKernelUtil(PVOID BaseAddress);
NTSTATUS HookGroupApp(PVOID BaseAddress);
NTSTATUS HookAppMisc(PVOID BaseAddress);
NTSTATUS HookMainFrame(PVOID BaseAddress);
NTSTATUS HookNtdll(PVOID BaseAddress);
NTSTATUS HookPsapi(PVOID BaseAddress);
NTSTATUS HookUser32(PVOID BaseAddress);

NTSTATUS CDECL CheckPluginList();

#endif // _HOOKS_H_10758d45_3f47_4c54_9096_1d2ee621661a_
