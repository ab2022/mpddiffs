#!/bin/bash

echo 'Stopping NGINX...'
sudo service nginx stop
echo 'NGINX stopped.'

echo 'Cleaning logs...'
sudo rm /var/log/nginx/error.log
sudo rm /var/log/nginx/error.log.*
sudo rm /var/log/error.log
sudo rm /var/log/access.log

echo 'Refreshing live-stream directory...'
rm -rf /dev/shm/dash/Service2/live-stream
mkdir -p /dev/shm/dash/Service2/live-stream

echo 'Refreshing client_temp directory...'
rm -rf /dev/shm/nginx/client_temp
mkdir -p /dev/shm/nginx/client_temp

echo 'Removing manifest...'
rm /dev/shm/dash/Service2/manifest.mpd

echo 'Starting NGINX...'
bash /home/ubuntu/diffstub/run_on_server.sh