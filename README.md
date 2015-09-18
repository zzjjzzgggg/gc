# I/O-Efficient Group Closeness Centrality Calculation

## Requirements

* The code is only tested on Ubuntu Server 14.04, x86_64.
* Requires the C++11 support.
* SNAP library.
  * The used SNAP library is slightly different from the official version.
    You can check out the personalized version from here:
    https://github.com/zzjjzzgggg/netsnap.git

## Usage

### Generate bit-strings

>usage: genbits
>    -job     [n]      0: split graph
>		      1: gen bits
>		      2: validate (default: -1)
>    -g       [str]    graph file name (default: )
>    -bs      [n]      graph block size (default: 10000)
>    -ps      [n]      page size (default: 1000)
>    -H       [n]      max hops (default: 5)
>    -N       [n]      n approx (default: 8)
>    -m       [n]      bucket bits (default: 6)
>    -r       [n]      more bits (default: 6)
>


### Run I/O-Efficient Greedy Algorithm

>usage: iogreedy
>    -job     [n]      0: evaluate estimates
>		      1: node reward gains
>		      2: io greedy
>		      3: multi pass (default: -1)
>    -g       [str]    graph (default: )
>    -B       [n]      budget (default: 500)
>    -L       [flt]    lambda (default: 0.50)
>    -H       [n]      max hops (default: 5)
>    -S       [n]      group size (default: 10)
>    -R       [n]      num repeat (default: 100)
