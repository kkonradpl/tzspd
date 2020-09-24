# tzspd
TZSP (TaZmen Sniffer Protocol) repeater.

Copyright (C) 2020  Konrad Kosmatka

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

# Introduction
This software receives TZSP packets and can send them to several applications at the same time using other UDP ports. It is also possible to route packets to different ports based on their sensor MAC field.

It was originally written to run several instances of [MTscan](https://github.com/kkonradpl/mtscan/) at the same time in the sniffer mode.

GNU/Linux and Windows (MINGW) operating systems are supported.

# Usage
You can specify as many output ports as you want:
```sh
$ tzspd 7000 7200 7400
```

You can specify a port range:
```sh
$ tzspd 7000-7004
```

For each output you can filter it by sensor MAC value:
```sh
$ tzspd 7000,00:11:22:33:44:55 7001-7003,11:22:33:44:55:66
```

You can also change the output IP address (default 127.0.0.1):
```sh
$ tzspd 10.0.0.1:7000 
```

For more information check:
```sh
$ tzspd -h
```

# Build
In order to build tzspd you will need:
- CMake
- C compiler

Scripts are available in the `build` directory.

# Installation
After a successful build, just use:
```sh
$ sudo make install
```
in the `build` directory. This will install the executable file `tzspd`.
