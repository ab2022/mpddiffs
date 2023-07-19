#!/bin/bash

# Save a backup of the current diffstub with the timestamp and "backup" appended
sudo mv /home/ubuntu/diffstub /home/ubuntu/diffstub_$(date +%s)_backup