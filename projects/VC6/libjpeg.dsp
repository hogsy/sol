# Microsoft Developer Studio Project File - Name="libjpeg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libjpeg - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libjpeg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libjpeg.mak" CFG="libjpeg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libjpeg - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libjpeg - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libjpeg\Release"
# PROP Intermediate_Dir "libjpeg\Release"
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

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libjpeg\Debug"
# PROP Intermediate_Dir "libjpeg\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ  /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_LIB" /YX /FD /GZ  /c
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

# Name "libjpeg - Win32 Release"
# Name "libjpeg - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\libraries\jpeg\jaricom.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jcapimin.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jcapistd.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jcarith.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jccoefct.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jccolor.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jcdctmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jchuff.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jcinit.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jcmainct.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jcmarker.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jcmaster.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jcomapi.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jcparam.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jcprepct.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jcsample.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jctrans.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdapimin.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdapistd.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdarith.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdatadst.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdatasrc.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdcoefct.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdcolor.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jddctmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdhuff.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdinput.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdmainct.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdmarker.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdmaster.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdmerge.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdpostct.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdsample.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdtrans.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jerror.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jfdctflt.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jfdctfst.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jfdctint.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jidctflt.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jidctfst.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jidctint.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jmemmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jmemnobs.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jquant1.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jquant2.c
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jutils.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\libraries\jpeg\jconfig.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jdct.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jerror.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jinclude.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jmemsys.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jpegint.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jpeglib.h
# End Source File
# Begin Source File

SOURCE=..\..\libraries\jpeg\jversion.h
# End Source File
# End Group
# End Target
# End Project
