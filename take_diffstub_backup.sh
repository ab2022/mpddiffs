#!/bin/bash

# Save a backup of the current diffstub with the timestamp and "backup" appended
sudo mv $HOME/diffstub $HOME/diffstub_$(date +%s)_backup