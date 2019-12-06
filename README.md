# Introduction
> Implement a https/http server, using libevent and http-parser

# Depended library
> libevent, openssl, http-parser

# Tasks
> - Basic
>   - GET
>   - POST
>   - Download
>       > support small files & large files download
>       > the speed of downloading one file is limited
>       > using io thread to downlaod large files
>   - Upload
>       > support small files upload
> - Connection
>     - Keep-alive
>       > using libevent tools to set
>     - Chunk 
>         > there is a bug, couldn't run correctly
>         > similiar to download large file
>         > evbuffer_add_printf breakout,
>     - Pipeline
>         > not implement
> - Openssl
>   > using libevent tools - bufferevent_openssl_socket_new
> - Libevent
>   > using listener, bufferevent, evbuffer
> - other framework
>   - http_parser 2.9.1, source install
>       > parser http (not very useful)

# Docker
> - Create image 
>   - docker build -t server .
> - Create a container
>   - docker run -it -P --name=server1 server /bin/bash
> - Exec a container command
>   - docker exec -it server1 /bin/bash
> - list containers
>   - docker ps -a

# Test
> open one brower
> - run server locally, visit https://127.0.0.1:443 or http://127.0.0.1:8080
> - run server in docker, visit https://docker_ip:docker_443_port or http://docker_ip:docker_8080_port

# Flow