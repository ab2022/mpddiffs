#!/bin/bash
# Should be run as on the server

# Compile
cd /home/ubuntu/nginx-1.18.0 &&\
./configure --with-ld-opt="-lstdc++" --with-compat --add-dynamic-module=../diffstub --with-cc-opt=-Wno-error &&\
make &&\
sudo make install &&\

# Move to modules directory
sudo cp /usr/local/nginx/modules/ngx_http_diffstub_module.so /usr/share/nginx/modules/ngx_http_diffstub_module.so &&\

sudo service nginx restart &&\
echo "Successfully completed run_on_server"