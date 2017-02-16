#!/bin/sh
set -ex

CDF_VER_MAJOR=3
CDF_VER_MINOR=6
CDF_VER_PATCH=3

CDF_VER_TWEAK=1

CDF_VERSION="${CDF_VER_MAJOR}${CDF_VER_MINOR}_${CDF_VER_PATCH}"
CDF_FULL_VER="${CDF_VERSION}${CDF_VER_TWEAK:+_${CDF_VER_TWEAK}}"

wget https://spdf.sci.gsfc.nasa.gov/pub/software/cdf/dist/cdf${CDF_FULL_VER}/linux/cdf${CDF_FULL_VER}-dist-all.tar.gz
tar -xzvf cdf${CDF_FULL_VER}-dist-all.tar.gz
cd cdf${CDF_VERSION}-dist

make OS=linux ENV=gnu SHARED=yes all
sudo make INSTALLDIR=/usr/ install
