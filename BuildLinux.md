# Introduction #
The best way to stay up-to-date with the project - it to build it yourself.
So here is a simple guide to build UDPT.

## Required Tools ##
Before building UDPT you may require some tools:

  * gcc
  * g++
  * Make
  * libsqlite3-dev
  * git

You can install all the above tools with this command:
```
sudo apt-get install gcc g++ make libsqlite3-dev git
```

# Getting the source #
Simply type:
```
git clone http://code.google.com/p/udpt/
```
This will download the repository to a new directory "udpt".

# Building #
Building UDPT is like building almost anything else on Linux based systems. Open a terminal (you can use the previous one...) and enter the following commands:
```
$ cd udpt
$ make all
```
After entering these commands, you should have the latest version of UDPT.
The executable file will be called udpt in the root directory of the repository.
If for some reason the compilation fails, please submit an issue about it.
Now you can simply type `./udpt` and your tracker will be running!