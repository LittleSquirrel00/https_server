# use dockerfile to build image
> docker build -t image_name dockerfile_path

# list docker containers
> docker ps -a

# create a docker container
> docker run -it -P --name=container_name image_name /bin/bash

# exe container software
> docker exec -it container_name /bin/bash