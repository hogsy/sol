# Microsoft Developer Studio Project File - Name="liboggvorbis" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=liboggvorbis - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "liboggvorbis.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "liboggvorbis.mak" CFG="liboggvorbis - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "liboggvorbis - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "liboggvorbis - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "liboggvorbis - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "liboggvorbis\Release"
# PROP Intermediate_Dir "liboggvorbis\Release"
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

!ELSEIF  "$(CFG)" == "liboggvorbis - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "liboggvorbis\Debug"
# PROP Intermediate_Dir "liboggvorbis\Debug"
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

# Name "liboggvorbis - Win32 Release"
# Name "liboggvorbis - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "ogg_src"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\ogg_src\bitwise.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\ogg_src\framing.c
# End Source File
# End Group
# Begin Group "vorbis_src"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\analysis.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\bitrate.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\block.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\codebook.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\envelope.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\floor0.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\floor1.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\info.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\lookup.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\lpc.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\lsp.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\mapping0.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\mdct.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\psy.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\registry.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\res0.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\sharedbook.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\smallft.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\synthesis.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\tone.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\vorbisenc.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\vorbisfile.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_src\window.c
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "ogg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\ogg_inc\ogg.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\ogg_inc\os_types.h
# End Source File
# End Group
# Begin Group "vorbis"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_inc\codec.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_inc\vorbisenc.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\oggvorbis\vorbis_inc\vorbisfile.h
# End Source File
# End Group
# End Group
# End Target
# End Project
