# libwmp
WMP stands for Wynand Marais Prototyping, and being a library, it says in full
Wynand Marais Prototyping Library. Of course that is just a mouth full and the
word libwmp is a lot smaller and easier to communicate.

## Overview
This C++ library is designed to allow rapidly prototyping of applications in
Windows and Linux. It seeks to contain the core functionality required by many
applications such as Logging, Serialisation and Networking. The library is kept
as small as possible with as few dependencies as possible so the return on
effort is not too high (i.e. faster to write it from scratch than the use the
library). To ease installation and usage, the library is also build using a
CMake build script.

The current feature set (some still in planning) include:

* Log (planned) – Provide a method of logging data to the console.
* Socket (planned) – Provides network socket communication ability.
* Serialiser / Deserialiser (planned) – Provides serialisation of data in and
out of binary streams.
* vulkan (planned) – Provides a simple interface to create vulkan applications.

## Building
The library is super easy to build, on purpose, otherwise it will not be worth
using. To achieve this, nothing but OS libraries and C++ language features and
libraries are used where possible.

For this project, the "git flow" model conceived by Vincent Driessen and 
described at https://nvie.com/posts/a-successful-git-branching-model/

The short of it is the repository consist of the following branches:

* feature-\* : This is where particular features are being actively developed. 
For example the log is developed in the ```release-log``` branch, etc.
* develop : This is where the code ends up when it is ready for release and
needs to be integrated with the other features in the code base.
* release-\* : This is where the code lives after integration. This is where
it goes through final review and prepared for publication to ```master```.
* master : This is where the delivered code lives. Always use the master branch
for your pojects.
* hotfix-\* : If anything release in master requires an urgent fix, it is made
in the hotfix branch. 

As a user you only want to use the contents of ```master```, and every other 
branch is a little bit dangerouse, but can be used if you understand the
consequences.

### Dependencies
The minimum required dependencies are:

* CMake 3.16+
* C++ 17 capable compiler such as GCC or Clang.

The optional dependencies are:

* Vulkan SDK – If not installed, the vulkan interface will not be built.

# Usage

## Log

## Socket

## Serialiser / Deserialiser

## Vulkan
