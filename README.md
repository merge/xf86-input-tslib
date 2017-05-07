# xf86-input-tslib
[X.org](https://x.org/) [tslib](http://tslib.org) input driver

### how to use
xf86-input-tslib assumes you have only one touchscreen device available, see
`80-tslib.conf`. If there are multiple in your system, please specify one config
section for each.
xf86-input-tslib aims to make [tslib](http://tslib.org) easy to use and doesn't
offer special configuration options.

* have tslib (libts) [installed](https://github.com/kergoth/tslib/blob/master/README.md#install-tslib).
* [download](https://github.com/merge/xf86-input-tslib/releases) and extract.
* `cd xf86-input-tslib` into the extracted directory
* `./configure --prefix=/usr`
* `make`
* `sudo make install`
    
Done. Configure your [tslib's ts.conf](https://github.com/kergoth/tslib/blob/master/README.md#configure-tslib)
(which is probably why you installed xf86-input-tslib in the first place :) ) and
reboot.

To _uninstall_, again go inside the extracted directory, and do

    sudo make uninstall

### multitouch
if you use tslib 1.10 or higher (libts.so.0.7.0 or higher), you have multitouch.

### contact
* If you have problems or suggestions, feel free to
[open an issue](https://github.com/merge/xf86-input-tslib/issues).
* The tslib developers can be reached by writing emails to the
[tslib mailing list.](http://lists.infradead.org/mailman/listinfo/tslib)
* X.org developers can be reached at the
[xorg-devel mailing list.](https://lists.freedesktop.org/mailman/listinfo/xorg)

### historical notes
In cooperation with [Pengutronix](http://pengutronix.de/index_en.html), this
[Github repository](https://github.com/merge/xf86-input-tslib) builds upon the
former
[source distribution.](http://public.pengutronix.de/software/xf86-input-tslib/)
(Thanks Pengutronix, for hosting and all your awesome development work!)
Development and main source distribution is now done on Github. In case
Pengutronix continues mirroring future tarball releases, and thus provides
backwards compatibility for downstream users: Thanks for your help :)
