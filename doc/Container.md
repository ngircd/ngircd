# [ngIRCd](https://ngircd.barton.de) - Container How-To

The ngIRCd daemon can be run as a containerized application, for example using
Docker or Podman (the latter being preferred and used in the examples below).
The container definition file, also known as "Docker file", is bundled with this
distribution as `contrib/Dockerfile` and based on the official "stable-slim"
container of the Debian project (see https://hub.docker.com/_/debian).

## Building the container

You can use the following command to build the ngIRCd container image:

```bash
podman build --format=docker -f contrib/Dockerfile .
```

The `Dockerfile` includes a `HEALTHCHECK` directive, which is not supported by
the default OCI 1.0 image format, therefore we use the "docker" format here.

If you are using Git, you can tag the built image like this (use the ID of the
newly built image!):

```bash
tag=$(git describe --tags | sed 's/rel-//g')
podman tag <container_id> "ngircd:${tag}"
```

## Running the container

You can use this command to run the ngIRCd container using Podman, for example:

```bash
podman run --name=ngircd --detach \
  -p 127.0.0.1:6667:6667 \
  ngircd:<tag>
```

This creates and starts a new container named "ngircd" from the image
"ngircd:<tag>" (you habe to substitute _<tag>_ with the real tag name here!) and
maps the host port 6667 on localhost to the port 6667 inside of the container.

### Configuring the container

The ngIRCd inside of the container is installed inside of `/opt/ngircd/` and the
default drop-in directory is `/opt/ngircd/etc/ngircd.conf.d`. Therefore you can
map a host folder to this drop-in directory inside of the container and place
drop-in configuration file(s) in the host path like this:

```bash
mkdir -p /host/path/to/ngircd/conf.d
touch /host/path/to/ngircd/conf.d/my.conf
podman run --name=ngircd --detach \
  -p 127.0.0.1:6667:6667 \
  -v "/host/path/to/ngircd/conf.d:/opt/ngircd/etc/ngircd.conf.d" \
  ngircd:<tag>
```

### Testing the configuration

As with the native daemon, it is a very good idea to validate the configuration
of the daemon after making changes.

With Docker and Podman, you can pass arguments to the `ngircd` binary inside of
the container by simply appending it to the "run" command line like this:

```bash
podman run --rm -it \
  -v "/host/path/to/ngircd/conf.d:/opt/ngircd/etc/ngircd.conf.d" \
  ngircd:<tag> \
  --configtest
```

### Reloading the daemon configuration in a running container

To activate changed configuration of ngIRCd, you can either restart the
container (which will disconnect all currently connected clients) or signal
`ngircd`(8) inside of the running container to reload its configuration file(s).

The latter can be done with this command, for example:

```bash
podman exec -it ngircd /bin/bash -c 'kill -HUP $(/usr/bin/pidof -s ngircd)'
```
