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

### calibrate (if not calibrated) before X starts the display
(untested :) If you use the `linear` module in `ts.conf` you want to calibrate
your screen, using `ts_calibrate`. To do this once in case not already done, you'd
add something like the following snippet to your display manager's init script
* `/etc/gdm3/Init/Default` for Gnome3's gdm
* `/etc/X11/xdm/Xsetup` for X's display manager
* `/etc/kde4/kdm/Xsetup` for KDE's kdm
and so on:

		TSCALIB_BIN=/usr/bin/ts_calibrate

		# calibrate touchscreen, if no calibfile exists
		if ! [ -f "${TSLIB_CALIBFILE}" ] ; then
			if ! ${TSCALIB_BIN} ; then
				echo "calibration failed!"
			else
				echo "calibration done."
			fi
		fi

To trigger re-calibration, simply delete the `TSLIB_CALIBFILE` file and reboot.

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
