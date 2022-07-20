#!/bin/sh
if [[ -z $1 ]]; then
    echo "Usage: ./make.sh project_name (Directory within src/ with build.conf) makefile_option (Number. Check/modify make.sh for options.)"
    exit 0
fi
project_dirs=$(ls -d src/*/ | sed 's#/##g' | sed 's#src##') #Make space separated string list of src subdirectories
for pn in $project_dirs; do
    if [[ "$pn" = "$1" ]]; then #Add name if within src directory.
        project_name=$1
        break
    fi
done
if [[ -z $project_name ]]; then
    >&2 echo "Unable to build! Project name directory \"$1\" is nonexistent."
    exit 1
fi
if test -f "src/$project_name/build.conf"; then
    source src/$project_name/build.conf
else
    >&2 echo "Unable to build! Project name directory \"$1\" does not have a build.conf. Copy build.conf from project root directory."
    exit 1
fi
all_shared_src_d=$(ls -d shared_src/*/ 2>/dev/null | sed 's#/##g' | sed 's#shared_src##')
use_shared_src_d=""
for us in $use_shared; do
    for ss in $all_shared_src_d; do
        if [[ "$us" = "$ss" ]]; then #Add any shared_src projects from build.conf.
            use_shared_src_d+="shared_src/$us "
        fi
    done
done
echo -e "\033[1;33m--------------------STARTING MAKEFILE BUILD FOR src/$project_name--------------------\033[0m"
export project_name compiler prefix compiler_flags inc_dirs link_libs linking_flags file_exe_postfix use_shared_src_d binary_arguments used_make_sh
c=-j$(($(nproc) + 1)) #To make build process faster
makefilename="-f Makefile_build"
mc="\033[1;35mUsing makefile command #$2 IN make.sh:\033[0m"
if [[ "$2" = "1" ]]; then #Build
    echo -e "$mc make $c $makefilename"
    make $c $makefilename
    exit $? #Cascade any errors if makefile has errors.
elif [[ "$2" = "2" ]]; then #Clean
    echo -e "$mc make clean $makefilename"
    make clean $makefilename
    exit $?
elif [[ "$2" = "3" ]]; then #Build and run
    echo -e "$mc make $c $makefilename && make run $makefilename"
    make $c $makefilename && make run $makefilename
    exit $?
elif [[ "$2" = "4" ]]; then #Clean and build
    echo -e "$mc make clean $makefilename && make $c $makefilename"
    make clean $makefilename && make $c $makefilename
    exit $?
elif [[ "$2" = "5" ]]; then #Clean and build (without -j)
    echo -e "$mc make clean $makefilename && make $makefilename"
    make clean $makefilename && make $makefilename
    exit $?
elif [[ "$2" = "6" ]]; then #Clean, build, and run
    echo -e "$mc make clean $makefilename && make $c $makefilename && make run $makefilename"
    make clean $makefilename && make $c $makefilename && make run $makefilename
    exit $?
fi
>&2 echo "Unable to build! Second argument must be a number within make.sh"
exit 1