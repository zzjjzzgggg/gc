# H-Group Closeness Centrality Calculation

## Requirements

* The code is tested on a 64 bit Ubuntu Server 14.04.3 LTS.
* Tested on GCC 4.8.4 and LLVM 3.6.0
* Require the C++11 `pthread` support, `-pthread -std=c++11`.
* Require the SNAP library (will be automatically downloaded).
  * The used SNAP library is slightly different from the official version.
    If the script does not clone the code. Please clone the specified
    library from here: https://github.com/zzjjzzgggg/netsnap.git

## Usage

### A minimum running example

The folder `exam` contains a minimum running example.

```shell
cd exam
chmod u+x run_all.sh
./run_all.sh
```

The script will automatically download the required library, compile the
program, and run all the programs.



