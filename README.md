# Bulkrays #

Bulkrays is a light c++ http server. It is aimed to deliver content through C++ code, providing
the C / C++ performances, maturity stability and long term support to HTML and content rendering.

A minimalist example is provided through the [simplefmap.hcpp](https://github.com/jd-code/bulkrays/blob/master/simplefmap.hcpp) which maps directories to HTML
content.

# Dependencies #
[qiconn](https://github.com/jd-code/qiconn) is imported as a git submodule

# Usage #
there's yet no typical service script for maintaining the daemon operations.
such a command line will launch the server binding on two addresses, dropping root
credentials for a less dangerously empowered user "bulkrays" right after binding ports
for example :
```
./bulkrays --bind=134.214.100.25:80 --bind=134.214.100.245:80 --user=bulkrays 2>&1 >> /var/log/bulkrays/error.log & 
```

