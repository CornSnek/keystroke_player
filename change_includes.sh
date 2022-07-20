#!/bin/sh
if [[ -z $1 ]] || [[ -z $2 ]]; then
    echo "Usage: project_name (Directory within src/ with build.conf), . This changes .vscode/c_cpp_properties.json .includePath option"
    exit 0
fi
if [[ "$2" != "Linux" ]] && [[ "$2" != "Mac" ]] && [[ "$2" != "Win32" ]]; then
    echo "Second argument needs to be either Linux, Mac, or Win32"
    exit 0
fi
project_dirs=$(ls -d src/*/ | sed 's#/##g' | sed 's#src##') #Make space separated string list of src subdirectories
for pn in $project_dirs; do
    if [[ "$pn" = "$1" ]]; then #Add name if within src directory.
        project_name="src/$1"
        include_paths="include/$1" #Headers in include if any.
        mkdir -p include/$1 #Add if nonexistent
        break
    fi
done
if [[ -z $project_name ]]; then
    >&2 echo "Project name directory \"$1\" is nonexistent."
    exit 1
fi
if test -f "$project_name/build.conf"; then
    source $project_name/build.conf
else
    >&2 echo "Project name directory \"$1\" does not have a build.conf."
    exit 1
fi
all_shared_include_d=$(ls -d shared_include/*/ 2>/dev/null | sed 's#/##g' | sed 's#shared_include##')
for us in $use_shared; do
    for ss in $all_shared_include_d; do
        if [[ "$us" = "$ss" ]]; then #Add any shared_include projects from build.conf.
            include_paths+=" shared_include/$us"
        fi
    done
done
for i in $inc_dirs; do #Add custom -I directories.
    include_paths+=" ${i:2:${#i}}" #Exclude "-I"
done
workspace_d='${workspaceFolder}/' #Append to every include_paths
for path in $include_paths; do
    if [[ "${path::1}" != "/" ]]; then #If relative, don't append $workspace_d
        include_paths_final+="\"${workspace_d}${path}\","
    else
        include_paths_final+="\"$path\","
    fi
done
include_paths_final=[${include_paths_final::-1}]
#^Do format of include_paths_final=["include_path1","include_path2",...,"include_path_last"] for jq -r --argjson. jq is picky.
for flag in $compiler_flags; do
    if [[ ${flag::2} = -D ]]; then #Separate defines (-D)
        defines+="\"${flag:2:${#flag}}\","
    else
        compiler_flags_final+="\"$flag\","
    fi
done
#Do the same formats as $include_paths_final
defines=[${defines::-1}]
compiler_flags_final=[${compiler_flags_final::-1}]
cp .vscode/c_cpp_properties.json .vscode/c_cpp_properties.json.old
#tmp_f=$mktemp
#Command to ONLY edit a platform name (configuration[].name using .configurations|=map if  then  else . end).
jq -r --argjson IP $include_paths_final --argjson CF $compiler_flags_final --argjson DF $defines --arg NAME $2 \
'.configurations|=map(if .name==$NAME then .includePath=$IP|.compilerArgs=$CF|.defines=$DF else . end)' .vscode/c_cpp_properties.json \
> temp.json && mv temp.json .vscode/c_cpp_properties.json
echo -e Done configuring includePath, compilerArgs, and defines at .vscode/c_cpp_properties.json. \
Note: Some options still need to be manually configured. A backup .json copy is at .vscode/c_cpp_properties.json.old