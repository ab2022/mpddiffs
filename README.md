# Diffstub
Started from Alex Balk's (abalk200_comcast) diffstub project

## Info
This is a standalone module for generateting an RF5261 (https://datatracker.ietf.org/doc/html/rfc5261) compliant MPED-DASH patch from 2 MPD files.

https://github.com/MPEGGroup/DASHSchema/blob/5th-Ed-AMD1/DASH-MPD.xsd


## Shell Scripts
- First run "take_backup.sh" within VM (placed in the home directory). 
- Then run "ship_to_server.sh" in local. 
- Lastly, run "run_on_server" in the VM (placed in diffstub).

## Running Tests
- In VSCode
    - Run Build task (ctl/cmd + shift + B) -> "C/C++: Build Test File"
    - Run Tests with the "Run DIFFSTUB Tests" run configuration

- Command Line:
    - Compile tests with:

    ```g++ -std=c++11 -g test_ngx_diffstub_internal.cpp diffstub_xml_node.cpp ngx_diffstub_internal.cpp -o test_ngx_diffstub_internal```
    - Run Tests:

    ```./test_ngx_diffstub_internal```