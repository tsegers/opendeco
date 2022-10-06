# opendeco

[![builds.sr.ht status](https://builds.sr.ht/~tsegers/opendeco/commits/main.svg)](https://builds.sr.ht/~tsegers/opendeco/commits/main?)

```Usage: opendeco [OPTION...]
Implementation of Buhlmann ZH-L16 with Gradient Factors:

  -d, --depth=NUMBER         Set the depth of the dive in meters
  -t, --time=NUMBER          Set the time of the dive in minutes
  -g, --gas=STRING           Set the bottom gas used during the dive, defaults
                             to Air
  -l, --gflow=NUMBER         Set the gradient factor at the first stop,
                             defaults to 30
  -h, --gfhigh=NUMBER        Set the gradient factor at the surface, defaults
                             to 75
  -G, --decogasses=LIST      Set the gasses available for deco
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Examples:

        ./opendeco -d 18 -t 60 -g Air
        ./opendeco -d 30 -t 60 -g EAN32
        ./opendeco -d 40 -t 120 -g 21/35 -l 20 -h 80 --decogasses Oxygen,EAN50

Report bugs to <~tsegers/opendeco@lists.sr.ht>.
```
