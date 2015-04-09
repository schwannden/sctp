# Practice SCPT

## Pre-requisite

#### First we need sctp

* Fedora
``sudo yum install lksctp\*``

* Debian/Ubuntu
``sudo apt-get install libsctp-dev lksctp-tools``

#### Second we need libsnp
git clone https://github.com/schwannden/libsnp
cd libsnp
make
sudo make install

For those system whose ``LD_LIBRARY_PATH does not contain /usr/lib
and libsctp or libsnp is installed in /usr/lib

``source environment-setup``
