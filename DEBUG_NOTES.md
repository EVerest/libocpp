what i've learned is that they expect you to have all the everest stuff installed and then try to run the example.
this currently very much does not work as a standalone example or library and is closely coupled to everest and all their other tooling.

i would like to be proven wrong here but that's my current understanding.

tried to set up docker file to run this example and well we got some where but it doesn't seem like the right path. pausing now to re-evaluate a cleaner approach.

current build failure:
```bash
lschubert@LUSN9994YRV74 libocpp % docker build -f Dockerfile.charger -t charger .
[+] Building 8.6s (16/23)                                                                                                                                                                                                                            
 => [internal] load build definition from Dockerfile.charger                                                                                                                                                                                    0.0s
 => => transferring dockerfile: 1.48kB                                                                                                                                                                                                          0.0s
 => [internal] load .dockerignore                                                                                                                                                                                                               0.0s
 => => transferring context: 2B                                                                                                                                                                                                                 0.0s
 => [internal] load metadata for docker.io/library/ubuntu:20.04                                                                                                                                                                                 0.5s
 => [ 1/19] FROM docker.io/library/ubuntu:20.04@sha256:f8f658407c35733471596f25fdb4ed748b80e545ab57e84efbdb1dbbb01bd70e                                                                                                                         0.0s
 => [internal] load build context                                                                                                                                                                                                               0.0s
 => => transferring context: 26.60kB                                                                                                                                                                                                            0.0s
 => CACHED [ 2/19] RUN ln -snf /usr/share/zoneinfo/$CONTAINER_TIMEZONE /etc/localtime && echo $CONTAINER_TIMEZONE > /etc/timezone                                                                                                               0.0s
 => CACHED [ 3/19] RUN apt-get update && apt-get install -y   build-essential   cmake   python3-pip   libboost-all-dev   libsqlite3-dev   libssl-dev   git                                                                                      0.0s
 => [ 4/19] COPY . /usr/src/app/libocpp                                                                                                                                                                                                         0.1s
 => [ 5/19] WORKDIR /usr/src/app                                                                                                                                                                                                                0.0s
 => [ 6/19] RUN git clone https://github.com/EVerest/everest-cmake.git                                                                                                                                                                          0.7s
 => [ 7/19] RUN git clone https://github.com/EVerest/everest-dev-environment.git                                                                                                                                                                0.9s 
 => [ 8/19] WORKDIR /usr/src/app/everest-dev-environment                                                                                                                                                                                        0.0s 
 => [ 9/19] RUN python3 -m pip install --upgrade pip setuptools wheel                                                                                                                                                                           3.1s
 => [10/19] WORKDIR /usr/src/app/everest-dev-environment/dependency_manager                                                                                                                                                                     0.0s 
 => [11/19] RUN python3 -m pip install .                                                                                                                                                                                                        2.8s 
 => ERROR [12/19] RUN edm --register-cmake-module --config ../everest-complete.yaml --workspace ~/checkout/everest-workspace                                                                                                                    0.4s 
------                                                                                                                                                                                                                                               
 > [12/19] RUN edm --register-cmake-module --config ../everest-complete.yaml --workspace ~/checkout/everest-workspace:                                                                                                                               
#0 0.363 usage: edm [-h] [--version] [--workspace WORKSPACE] [--working_dir WORKINGDIR]                                                                                                                                                              
#0 0.363            [--out OUTFILENAME] [--include_deps] [--config CONFIG]                                                                                                                                                                           
#0 0.363            [--create-vscode-workspace] [--cmake] [--verbose] [--nocolor]                                                                                                                                                                    
#0 0.363            [--install-bash-completion] [--create-config CREATECONFIG]
#0 0.363            [--external-in-config]
#0 0.363            [--include-remotes [INTERNAL [INTERNAL ...]]]
#0 0.363            [--create-snapshot [CREATE_SNAPSHOT]] [--git-info] [--git-fetch]
#0 0.363            [--git-pull [GIT_PULL [GIT_PULL ...]]]
#0 0.363            {init,list,rm,git,snapshot,release} ...
#0 0.363 edm: error: unrecognized arguments: --register-cmake-module
------
Dockerfile.charger:26
--------------------
  24 |     WORKDIR /usr/src/app/everest-dev-environment/dependency_manager
  25 |     RUN python3 -m pip install .
  26 | >>> RUN edm --register-cmake-module --config ../everest-complete.yaml --workspace ~/checkout/everest-workspace
  27 |     
  28 |     ENV PATH="/usr/src/app/everest-dev-environment:${PATH}"
--------------------
ERROR: failed to solve: process "/bin/sh -c edm --register-cmake-module --config ../everest-complete.yaml --workspace ~/checkout/everest-workspace" did not complete successfully: exit code: 2
```
