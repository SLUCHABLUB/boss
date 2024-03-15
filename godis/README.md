# 🎃 Boss || godis 👻
This folder contains a software implementation of the hardware api layer that boss uses to display content. It is meant to be used as a test bed for existing and new programs for boss, without having to ssh into boss and run the programs there. It is designed to take programs for boss without the need for modification.

## How to use
Simply develop your program as if it were to run directly on boss (feel free to take inspiration from existing programs in `/examples-api-use/`). Then, it is as easy as running the following commands from this directory:
```
> make ARGS=<progname>
> ./godis <args-for-your-program>
```
The *`ARGS=<progname>`* will look for a `.cc`-file named `progname.cc` and compile it with the test bed code.

Running this will open a window where an emulated boss (godis) can be seen.

To compile and run a different program, be sure to run `make clean` before following the steps above

### Example usage:
```
> make ARGS=choochoo
> ./godis
```

![](./choochoo.gif)

### Videoviewer
There is also a Make target for the video viewer. Simply run `make video` instead, then run `./video -F <path-to-video>`

## Dependencies
This tool make use of the open multimedia library *[SFML](https://www.sfml-dev.org/index.php)*. It can be installed by running `sudo apt install libsfml-dev`

To run the video-viewer program you will also need the following libraries: `avcodec`, `swscale` and `pkg-config`. They can be installed by running `sudo apt install libavcodec-dev libswscale-dev pkg-config`