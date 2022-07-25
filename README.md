# neodb


## Docker Image
You could use the `docker/Dockerfile` to compile & run NeoDB's docker image for development or demo run.

```
cd docker
# build the docker image, name it as `neodb_image`
docker build -t neodb_image .

# start a container on privileges mode, so we could access host's block device direclty
# remove `--privileges` if its not an option of your docker version.
sudo docker run --cap-add=SYS_PTRACE --privileges --security-opt seccomp=unconfined --name=neodb -it -v {HOST_NEODB_PATH}:{CONTAINER_NEODB_PATH}  neodb_image`:w

```
