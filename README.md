
# Name

dnscrypt-wrapper - A server-side dnscrypt proxy.

[![Build Status](https://travis-ci.org/cofyc/dnscrypt-wrapper.png?branch=master)](https://travis-ci.org/cofyc/dnscrypt-wrapper)

## Table of Contents

* [Description](#description)
* [Installation](#installation)
* [Usage](#usage)
  * [Quick start](#quick-start)
  * [Running unauthenticated DNS and the dnscrypt service on the same port](#running-unauthenticated-dns-and-the-dnscrypt-service-on-the-same-port)
  * [Key rotation](#key-rotation)
* [Chinese](#chinese)
* [See also](#see-also)

## Description

This is dnscrypt wrapper (server-side dnscrypt proxy), which helps to
add dnscrypt support to any name resolver.

This software is modified from
[dnscrypt-proxy](https://github.com/jedisct1/dnscrypt-proxy).

## Installation

Install [libsodium](https://github.com/jedisct1/libsodium) and [libevent](http://libevent.org/) 2.1.1+ first.

On Linux:

    $ ldconfig # if you install libsodium from source
    $ git clone git://github.com/cofyc/dnscrypt-wrapper.git
    $ cd dnscrypt-wrapper
    $ make configure
    $ ./configure
    $ make install

On FreeBSD:

    $ pkg install dnscrypt-wrapper

On OpenBSD:

    $ pkg_add -r gmake autoconf
    $ pkg_add -r libevent
    $ git clone git://github.com/cofyc/dnscrypt-wrapper.git
    $ cd dnscrypt-wrapper
    $ gmake LDFLAGS='-L/usr/local/lib/' CFLAGS=-I/usr/local/include/

On MacOS:
