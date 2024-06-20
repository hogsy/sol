#!/bin/bash
f=`dirname $0`
cd $f
cd ../game
../tools_bin/extractfuncs_x64 *.c -o g_func_list.h g_func_decs.h
../tools_bin/extractfuncs_x64 *.c -t mmove_t -o g_mmove_list.h g_mmove_decs.h
