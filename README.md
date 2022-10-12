# opendeco

[![builds.sr.ht status](https://builds.sr.ht/~tsegers/opendeco/commits/main.svg)](https://builds.sr.ht/~tsegers/opendeco/commits/main?)

```
Usage: opendeco [OPTION...] 
Implementation of Buhlmann ZH-L16 with Gradient Factors:

 Dive options:
  -d, --depth=NUMBER         Set the depth of the dive in meters
  -t, --time=NUMBER          Set the time of the dive in minutes

  -g, --gas=STRING           Set the bottom gas used during the dive, defaults
                             to Air

  -p, --pressure=NUMBER      Set the surface air pressure, defaults to
                             1.01325bar or 1atm

 Deco options:
  -l, --gflow=NUMBER         Set the gradient factor at the first stop,
                             defaults to 30

  -h, --gfhigh=NUMBER        Set the gradient factor at the surface, defaults
                             to 75

  -G, --decogasses=LIST      Set the gasses available for deco

  -s                         Only switch gas at deco stops

  -6                         Perform last deco stop at 6m

 Informational options:

  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Examples:

  ./opendeco -d 18 -t 60 -g Air
  ./opendeco -d 30 -t 60 -g EAN32
  ./opendeco -d 40 -t 120 -g 21/35 -l 20 -h 80 --decogasses Oxygen,EAN50

Report bugs to <~tsegers/opendeco@lists.sr.ht> or
https://todo.sr.ht/~tsegers/opendeco.
```

## Contributing

All contributors are required to "sign-off" their commits (using `git commit
-s`) to indicate that they have agreed to the [Developer Certificate of
Origin][dco], reproduced below. Your commit authorship must reflect the name
you use in meatspace, anonymous or pseudonymous contributions are not permitted.

[dco]: https://developercertificate.org/

```
Developer Certificate of Origin
Version 1.1

Copyright (C) 2004, 2006 The Linux Foundation and its contributors.
1 Letterman Drive
Suite D4700
San Francisco, CA, 94129

Everyone is permitted to copy and distribute verbatim copies of this
license document, but changing it is not allowed.


Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the best
    of my knowledge, is covered under an appropriate open source
    license and I have the right under that license to submit that
    work with modifications, whether created in whole or in part
    by me, under the same open source license (unless I am
    permitted to submit under a different license), as indicated
    in the file; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including all
    personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with
    this project or the open source license(s) involved.
```

Please [send patches](https://git-send-email.io) to the [opendeco][opendeco]
mailing list to send your changes upstream.

[opendeco]: https://lists.sr.ht/~tsegers/opendeco
