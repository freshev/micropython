[**English**](./README.md) | **中文**

![github license](https://img.shields.io/github/license/Dozingfiretruck/nes)![linux](https://github.com/Dozingfiretruck/nes/actions/workflows/action.yml/badge.svg?branch=master)



# nes

#### Introduction
nes simulator implemented in c language, requiring c11

**Notice:**

**This warehouse is only for the nes simulator and does not provide the game itself! ! ! **

Support status:

- [x] CUP

- [x] PPU

- [ ] APU

mapper support: 0, 2

#### Software architecture
The example is based on SDL2 for image and sound output. There are no special dependencies. You can transplant it to any hardware by yourself.


#### Compilation tutorial

Clone this repository, install [xmake](https://github.com/xmake-io/xmake), and directly execute xmake to compile

#### Instructions for use

Under Linux, enter `./nes xxx.nes` to load the game you want to run.
Under windows, enter `.\nes.exe xxx.nes` to load the game you want to run.



Button mapping:

Go up A B

Left Down Right Select Start

P1:

W                                                     J            K

A	    S	    D		      V             B

P2:

↑                                                      5            6

←	  ↓	    →		    1             2

#### Literature Reference

https://www.nesdev.org/



