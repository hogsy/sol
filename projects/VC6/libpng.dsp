# Microsoft Developer Studio Project File - Name="libpng" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libpng - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libpng.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libpng.mak" CFG="libpng - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libpng - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libpng - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libpng - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libpng\Release"
# PROP Intermediate_Dir "libpng\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G5 /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libpng - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libpng\Debug"
# PROP Intermediate_Dir "libpng\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libpng - Win32 Release"
# Name "libpng - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\libraries\png\png.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngerror.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngget.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngmem.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngpread.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngread.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngrio.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngrtran.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngrutil.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngset.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngtrans.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngwio.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngwrite.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngwtran.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngwutil.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\libraries\png\png.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngconf.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngdebug.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pnginfo.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pnglibconf.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngpriv.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\png\pngstruct.h
# End Source File
# End Group
# End Target
# End Project
