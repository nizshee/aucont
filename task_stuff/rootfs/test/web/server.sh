#!/bin/sh

PWD=/test/web
cd $PWD

PORT=$1

./mongoose-lua-sqlite-ssl-static-x86_64-5.2 \
    -document_root . \
    -listening_port 0.0.0.0:$PORT > ./mongoose.log
