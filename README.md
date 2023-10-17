# Diffstub
Started from Alex Balk's (abalk200_comcast) diffstub project.  Code contributions from Erik Ponder (eponde494_comcast) and Jovan Rosario (jriver829_comcast)

## Info
This is a standalone NGINX C++ module for generateting an RF5261 (https://datatracker.ietf.org/doc/html/rfc5261) compliant MPED-DASH patch from 2 MPD files.  This uses the PugiXML library to generate patch files. This software is in a development phase where features are being added and tested, and it may not be stable or feature-complete.

https://github.com/MPEGGroup/DASHSchema/blob/5th-Ed-AMD1/DASH-MPD.xsd

## Requirements
- C++ 11
- NGINX 1.18.0

## Shell Scripts
- ```ship_to_server.sh``` - Runs locally, Ships source code to external server
- ```run_on_server.sh``` - Run on server. Compiles module and starts nginx

## Running Tests
- In VSCode
    - Run Build task (ctl/cmd + shift + B) -> "C/C++: Build Test File"
    - Run Tests with the "Run DIFFSTUB Tests" run configuration

- Command Line:
    - Compile tests with:
    ```g++ -std=c++11 -g test_ngx_diffstub_internal.cpp diffstub_xml_node.cpp ngx_diffstub_internal.cpp -o test_ngx_diffstub_internal```

    - Run Tests:
    ```./test_ngx_diffstub_internal```

# Current Limitations
    - Hardcoded TTL/Patch Headers
    - Issue with Adding nested elements (i.e. adding a new element that contains grandchildren, currently WIP but incomplete)
    - Issue with multiple directives (add/rem/rep) on a single element (Not Implemented)
    - NOTE: Failing test cases have been disabled for now