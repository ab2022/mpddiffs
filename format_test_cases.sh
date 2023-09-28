#!/bin/bash

# Set the directory containing your XML files
test_dir="/Users/eponde494/Workspace/diffstub/mpd_samples/test_cases"

# Loop through all MPD files in the directory
for file in "$test_dir"/*.mpd; do
    if [ -f "$file" ]; then
        # Format the XML file with 4-space tab indents and save it with the original name
        xmllint --format --encode UTF-8 "$file" > "$file.tmp" && mv "$file.tmp" "$file"
    fi
done