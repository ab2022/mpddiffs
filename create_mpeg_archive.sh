#!/bin/bash

zip -r nginx_diffstub.zip . -x '.DS_Store' 'Notes.txt' 'mpd_samples/stream_session/*' 'run_notes.txt' 'create_mpeg_archive.sh' '.git/*' '.gitignore' 'diffstub.dSYM/*' 'test_ngx_diffstub_internal.dSYM/*' 'diffstub' 'tags' 'nginx_diffstub/*'
