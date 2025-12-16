deltagen for Air780
==============
deltagen is a tool to make par (diff) files for FOTA Toolkit 

The original algorithm and implementation was developed by Colin Percival.  The
algorithm is detailed in his paper, [Naïve Differences of Executable Code](http://www.daemonology.net/papers/bsdiff.pdf).  For more information, visit his
website at <http://www.daemonology.net/bsdiff/>.

License
-------
Copyright 2003-2005 Colin Percival  
Copyright 2012 Matthew Endsley
Copyright 2025 freshev

This project is governed by the BSD 2-clause license. For details see the file
titled LICENSE in the project root folder.

Overview
--------
This library generates patches that ARE NOT compatible with the original bsdiff
tool.

Compiling
---------
The libraries should compile warning free in any moderately recent version of
gcc. The project uses `<stdint.h>` which is technically a C99 file and not
available in Microsoft Visual Studio. The easiest solution here is to use the
msinttypes version of stdint.h from <https://code.google.com/p/msinttypes/>.
The direct link for the lazy people is:
<https://msinttypes.googlecode.com/svn/trunk/stdint.h>.

If your compiler does not provide an implementation of `<stdint.h>` you can
remove the header from the bsdiff files and provide your own typedefs
for the following symbols: `uint8_t`, `uint64_t` and `int64_t`.
