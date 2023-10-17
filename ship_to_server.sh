#!/bin/bash
# Set the source and destination paths
source_path="../diffstub"
destination_user="ubuntu"
destination_host="35.88.227.80"
destination_port="20202"
destination_path="/home/ubuntu/diffstub"

# Exclude git-related files using the --exclude flag with tar
tar --exclude=".git" --exclude=".gitignore" --exclude="mpd_samples" -czf /tmp/archive.tar.gz -C $source_path .
echo 'tar created'

# Take Diffstub backup
ssh -p $destination_port $destination_user@$destination_host "mv /home/ubuntu/diffstub /home/ubuntu/diffstub_$(date +%s)_backup"
echo 'diffstub backed up'

# Create new empty diffstub directory
ssh -p $destination_port $destination_user@$destination_host "mkdir /home/ubuntu/diffstub"
echo 'diffstub created'

# Transfer the archived files to the VM using scp
scp -P $destination_port /tmp/archive.tar.gz $destination_user@$destination_host:$destination_path
echo 'tar transferred'

# Extract the files on the VM using tar
ssh -p $destination_port $destination_user@$destination_host "tar -xzf $destination_path/archive.tar.gz -C $destination_path"
echo 'tar extracted'

# Remove the temporary archive file on the VM
ssh -p $destination_port $destination_user@$destination_host "rm $destination_path/archive.tar.gz"
echo 'tar removed'