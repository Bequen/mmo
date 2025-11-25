# mmo

Game engine demo with built-in networking for multiplayer.

## Building

Dependencies:
- SDL2
- Vulkan SDK

Both need to be downloaded before building. Do not forget to set PATH on Windows.


This command will initialize the build directory: `build/`

```bash
$ cmake -S . -B ./build/
```

This will build the entire project.

```bash
$ cmake --build ./build
```

There are now two executables:

Server is at `build/modules/client/mmo_server`

Client is at `build/modules/client/mmo_client`

First run the server, so that client can connect. The server will try to use port 8080, but if it is already occupied, it will try use the next free higher one.

