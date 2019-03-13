## odfgrep - grep utility for searching ODF documents

Currently only text documents are supported. The documents
must follow the ISO/OASIS Open Document Format standard.

The usual grep command line options are supported, as much
as make sense for searching documents instead of text files.

The actual pattern matching is handled by Boost.Regex. 

Ray Lischner
2-Aug-2006

## Building odfgrep

Debian/Ubuntu/Mint build dependencies:

* libboost-dev
* libboost-regex1.65-dev
* libxml2-dev
* libzip-dev
* automake
* autotools-dev
* libtool

You can install the dependencies by running this command:

```bash
sudo apt install libboost-dev libboost-regex1.65-dev libxml2-dev libzip-dev automake autotools-dev libtool
```
Once the dependencies have been installed you can configure and compile the code by running:

```bash
autoreconf -vi
./configure
make
```

The odfgrep binary will be in the src directory.  To install it under
/usr/local/bin run:

```bash
sudo make install
```

Once it is installed you can read the manual page for odfgrep:

```bash
man odfgrep
```
