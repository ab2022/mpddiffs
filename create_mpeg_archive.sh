#!/bin/bash

# Exclude any files containing notes to specific implementation, as well as previously created zip
# files and executables
zip -r nginx_diffstub.zip . -x '.DS_Store' 'Notes.txt' 'mpd_samples/stream_session/*' \
'run_notes.txt' 'create_mpeg_archive.sh' '.git/*' '.gitignore' '*.dSYM/*' 'clean_start.sh' \
'test_ngx_diffstub_internal' 'diffstub' 'nginx_diffstub/*' 'nginx_diffstub.zip' 'format_test_cases.sh'
