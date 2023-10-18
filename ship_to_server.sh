#!/bin/bash
# Set the source and destination paths
source_path="../diffstub"
destination_user="$ORIGIN_USER"
destination_host="$ORIGIN_HOST"
destination_port="$ORIGIN_SSH_PORT"
destination_path="/home/$destination_user/diffstub"

if [ "$(uname)" == "Darwin" ]; then
    # If it's macOS, set COPYFILE_DISABLE, this prevents ._* metadata files from being added to the tar archive
    export COPYFILE_DISABLE=true
fi

# Exclude git-related files using the --exclude flag with tar
tar --exclude=".git" --exclude=".gitignore" --exclude="mpd_samples" --exclude="doctest.h" \
    --exclude="create_mpeg_archive.sh" --exclude="*.txt" --exclude="diffstub" \
    --exclude="format_test_cases.sh" --exclude="ship_to_server.sh" --exclude="*.dSYM"  \
    --exclude=".vscode" --exclude=".DS_Store" --exclude="test_ngx_diffstub_internal" \
    --exclude="cxxopts.hpp" --exclude="main.cpp" --exclude="test_ngx_diffstub_internal.cpp" \
    -czf /tmp/archive.tar.gz -C $source_path .
echo 'tar created'

# Take Diffstub backup
ssh -p $destination_port $destination_user@$destination_host "mv $destination_path ${destination_path}_$(date +%s)_backup"
echo 'diffstub backed up'

# Create new empty diffstub directory
ssh -p $destination_port $destination_user@$destination_host "mkdir $destination_path"
echo 'diffstub created'

# Transfer the archived files to the VM using scp
scp -P $destination_port /tmp/archive.tar.gz $destination_user@$destination_host:$destination_path
echo 'tar transferred'

# Extract the files on the VM using tar
ssh -p $destination_port $destination_user@$destination_host "tar -xzf $destination_path/archive.tar.gz -C $destination_path 2>/dev/null"
echo 'tar extracted'

# Remove the temporary archive file on the VM
ssh -p $destination_port $destination_user@$destination_host "rm $destination_path/archive.tar.gz"
echo 'tar removed'