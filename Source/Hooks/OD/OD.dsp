# Microsoft Developer Studio Project File - Name="OD" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=OD - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "OD.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OD.mak" CFG="OD - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OD - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OD_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Gr /MD /W4 /GR- /O2 /Ob1 /D USE_NT_VER=1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /GS- /GL /MP /arch:SSE /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 undoc_k32.lib MyLib_NT.lib undoc_ntdll.lib ntdll.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"F:\Crack\OllyICE\OD.dll" /ltcg /DELAYLOAD:OllyDbg.exe /DELAYLOAD:USER32.dll /DELAYLOAD:PSAPI.dll /DELAYLOAD:ADVAPI32.dll
# SUBTRACT LINK32 /pdb:none
# Begin Target

# Name "OD - Win32 Release"
# Begin Source File

SOURCE=.\AntiDbgSysCallDbgee.cpp
# End Source File
# Begin Source File

SOURCE=.\AntiDbgSysCallDbgee.h
# End Source File
# Begin Source File

SOURCE=.\AntiDbgSysCallDbger.cpp
# End Source File
# Begin Source File

SOURCE=.\AntiDbgSysCallDbger.h
# End Source File
# Begin Source File

SOURCE=.\OD.CPP
# End Source File
# Begin Source File

SOURCE=.\OD.def
# End Source File
# Begin Source File

SOURCE=.\OD.H
# End Source File
# End Target
# End Project
