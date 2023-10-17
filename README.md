# Diffstub
Started from Alex Balk's (abalk200_comcast) diffstub project.  Code contributions from Erik Ponder (eponde494_comcast) and Jovan Rosario (jriver829_comcast)

## Info
This is a standalone NGINX C++ module for generateting an RF5261 (https://datatracker.ietf.org/doc/html/rfc5261) compliant MPED-DASH patch from 2 MPD files.  This uses the PugiXML library to generate patch files. This software is in a development phase where features are being added and tested, and it may not be stable or feature-complete.

https://github.com/MPEGGroup/DASHSchema/blob/5th-Ed-AMD1/DASH-MPD.xsd

## Requirements
- C++ 11
- NGINX 1.18.0 ~ (Only tested with Ubuntu 20.04)
- DASH.JS (Nightly Build) - (HTTP only) http://reference.dashif.org/dash.js/nightly/samples/dash-if-reference-player/index.html

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

# Setup

 - ORIGIN: Download and extract NGINX 1.18.0 source code to home directory ~
 ```
 cd ~
 wget http://nginx.org/download/nginx-1.18.0.tar.gz
 tar -zxvf nginx-1.18.0.tar.gz
```
- LOCAL: Edit `ship_to_server.sh` with your origin servers user ip and port, and Run:
```
./ship_to_server.sh
```
- ORIGIN: 
    - Copy the `nginx_diffstub.conf` to `/etc/nginx/nginx/conf`, or add the required lines to existing conf.  You can use `nginx_diffstub.conf` for reference
    ```
    sudo cp nginx_diffstub.conf /etc/nginx/nginx.conf
    ```
    -  Edit `run_on_server.sh` with nginx source location, if different than home (~), and Run.  This will compile the diffstub module and start nginx
    ```
    ./run_on_server
    ```
    
    - To Stop: `sudo service nginx stop`

- DASH.JS
    - Open Nightly Build: http://reference.dashif.org/dash.js/nightly/samples/dash-if-reference-player/index.html
    - Enter Origin IP and MPD location and 'load'
    


# Current Limitations
    - Only Support for HTTP streams (no DRM)
    - Hardcoded TTL/Patch Headers
    - Issue with Adding nested elements (i.e. adding a new element that contains grandchildren, currently WIP but incomplete)
    - Issue with multiple directives (add/rem/rep) on a single element (Not Implemented)
    - NOTE: Failing test cases have been disabled for now