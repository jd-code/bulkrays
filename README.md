# Bulkrays #

Bulkrays is a light c++ http server. It is aimed to deliver content through C++ code, providing
the C / C++ performances, maturity stability and long term support to HTML and content rendering.

A minimalist example is provided through the [simplefmap.hcpp](https://github.com/jd-code/bulkrays/blob/master/simplefmap.hcpp) which maps directories to HTML content.

Modules are simple to build, particularly by inlining HTML right into your CPP code.
A [vim syntax extension](https://github.com/jd-code/bulkrays/blob/master/hcpp.vim) is provided in order to hilight correctly those inclusions.

# Usage #
there's yet no typical service script for maintaining the daemon operations.
such a command line will launch the server binding on two addresses, *dropping root
credentials* for a less dangerously empowered user "bulkrays" right after binding ports
for example :
```
./bulkrays --bind=134.214.100.25:80 --bind=134.214.100.245:80 --user=bulkrays 2>&1 \
    >> /var/log/bulkrays/error.log & 
```

# Dependencies #
* [qiconn/libqiconn](https://github.com/jd-code/qiconn) a pool of "socket-enriched" derivative of iostreams.

### Typical building dependencies ###
in order to compile on a debian buster :
* build-essential
* autotools-dev
* libtool
* expect (provides unbuffer used in the "vimtest" target)
* libmhash-dev	(computes md5 here and there ...)


# Building #
the following will bring a default build :
```
[ retrieve and install libqiconn ]
autoall && ./configure
make all
```
A real interesting build should contain additionnal cpp modules of yours, inspired by
`simplefmap.hcpp`, and duly referenced in `bootstrap.cpp`.
