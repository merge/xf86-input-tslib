/*
 * (c) 2006 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 *
 * 20070416 Support for rotated displays by
 *          Clement Chauplannaz, Thales e-Transactions <chauplac@gmail.com>
 *
 * derived from the xf86-input-void driver
 * Copyright 1999 by Frederic Lepied, France. <Lepied@XFree86.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is  hereby granted without fee, provided that
 * the  above copyright   notice appear  in   all  copies and  that both  that
 * copyright  notice   and   this  permission   notice  appear  in  supporting
 * documentation, and that   the  name of  Frederic   Lepied not  be  used  in
 * advertising or publicity pertaining to distribution of the software without
 * specific,  written      prior  permission.     Frederic  Lepied   makes  no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * FREDERIC  LEPIED DISCLAIMS ALL   WARRANTIES WITH REGARD  TO  THIS SOFTWARE,
 * INCLUDING ALL IMPLIED   WARRANTIES OF MERCHANTABILITY  AND   FITNESS, IN NO
 * EVENT  SHALL FREDERIC  LEPIED BE   LIABLE   FOR ANY  SPECIAL, INDIRECT   OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA  OR PROFITS, WHETHER  IN  AN ACTION OF  CONTRACT,  NEGLIGENCE OR OTHER
 * TORTIOUS  ACTION, ARISING    OUT OF OR   IN  CONNECTION  WITH THE USE    OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* tslib input driver */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <misc.h>
#include <xf86.h>
#if !defined(DGUX)
#include <xisb.h>
#endif
#include <xf86_OSproc.h>
#include <xf86Xinput.h>
#include <exevents.h>		/* Needed for InitValuator/Proximity stuff */
#include <X11/keysym.h>
#include <mipointer.h>
#include <randrstr.h>
#include <xserver-properties.h>

#include <sys/time.h>
#include <time.h>

#include <tslib.h>


#define MAXBUTTONS 3
#define TIME23RDBUTTON 0.5
#define MOVEMENT23RDBUTTON 4

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
#define COLLECT_INPUT_OPTIONS(pInfo, options) xf86CollectInputOptions((pInfo), (options), NULL)
#else
#define COLLECT_INPUT_OPTIONS(pInfo, options) xf86CollectInputOptions((pInfo), (options))
#endif

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) > 13
static void
xf86XInputSetScreen(InputInfoPtr	pInfo,
		    int			screen_number,
		    int			x,
		    int			y)
{
	if (miPointerGetScreen(pInfo->dev) !=
	    screenInfo.screens[screen_number]) {
		miPointerSetScreen(pInfo->dev, screen_number, x, y);
	}
}
#endif

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 23
#define HAVE_THREADED_INPUT	1
#endif

enum { TSLIB_ROTATE_NONE = 0, TSLIB_ROTATE_CW = 270, TSLIB_ROTATE_UD = 180, TSLIB_ROTATE_CCW = 90 };

enum button_state { BUTTON_NOT_PRESSED = 0, BUTTON_1_PRESSED = 1, BUTTON_3_CLICK = 3, BUTTON_3_CLICKED = 4, BUTTON_EMULATION_OFF = -1 };

struct ts_priv {
	XISBuffer *buffer;
	struct tsdev *ts;
	int lastx, lasty, lastp;
	int screen_num;
	int rotate;
	int height;
	int width;
	enum button_state state;
	struct timeval button_down_start;
	int button_down_x, button_down_y;
};

static void
BellProc(int percent, DeviceIntPtr pDev, pointer ctrl, int unused)
{
#ifdef DEBUG
	ErrorF("%s\n", __FUNCTION__);
#endif
	return;
}

static void
KeyControlProc(DeviceIntPtr pDev, KeybdCtrl *ctrl)
{
#ifdef DEBUG
	ErrorF("%s\n", __FUNCTION__);
#endif
	return;
}

static void
PointerControlProc(DeviceIntPtr dev, PtrCtrl *ctrl)
{
}

static Bool
ConvertProc(InputInfoPtr local,
			 int first,
			 int num,
			 int v0,
			 int v1,
			 int v2,
			 int v3,
			 int v4,
			 int v5,
			 int *x,
			 int *y)
{
	*x = v0;
	*y = v1;
	return TRUE;
}

static struct timeval TimevalDiff(struct timeval a, struct timeval b)
{
	struct timeval t;
	t.tv_sec = a.tv_sec-b.tv_sec;
	t.tv_usec = a.tv_usec - b.tv_usec;
	if (t.tv_usec < 0) {
		t.tv_sec--;
		t.tv_usec += 1000000;
	}
    return t;
}

static void ReadInputLegacy(InputInfoPtr local)
{
	struct ts_priv *priv = (struct ts_priv *) (local->private);
	struct ts_sample samp;
	int ret;
	int x, y;
	ScrnInfoPtr pScrn = xf86Screens[priv->screen_num];
	Rotation rotation = rrGetScrPriv (pScrn->pScreen) ? RRGetRotation(pScrn->pScreen) : RR_Rotate_0;
	struct timeval now;

	while ((ret = ts_read(priv->ts, &samp, 1)) == 1) {
		gettimeofday(&now, NULL);
		struct timeval pressureTime = TimevalDiff(now, priv->button_down_start);

		if (samp.pressure) {
			int tmp_x = samp.x;

			switch (priv->rotate) {
			case TSLIB_ROTATE_CW:	samp.x = samp.y;
						samp.y = priv->width - tmp_x;
						break;
			case TSLIB_ROTATE_UD:	samp.x = priv->width - samp.x;
						samp.y = priv->height - samp.y;
						break;
			case TSLIB_ROTATE_CCW:	samp.x = priv->height - samp.y;
						samp.y = tmp_x;
						break;
			default:		break;
			}

			tmp_x = samp.x;

			switch (rotation) {
			case RR_Rotate_90:
				samp.x = (priv->height - samp.y - 1) * priv->width / priv->height;
				samp.y = tmp_x * priv->height / priv->width;
				break;
			case RR_Rotate_180:
				samp.x = priv->width - samp.x - 1;
				samp.y = priv->height - samp.y - 1;
				break;
			case RR_Rotate_270:
				samp.x = samp.y * priv->width / priv->height;
				samp.y = (priv->width - tmp_x - 1) * priv->height / priv->width;
				break;
			}

			priv->lastx = samp.x;
			priv->lasty = samp.y;
			x = samp.x;
			y = samp.y;

			/* TODO remove this? */
			xf86XInputSetScreen(local, priv->screen_num,
					samp.x,
					samp.y);

			xf86PostMotionEvent (local->dev, TRUE, 0, 2,
					x, y);

		}

		/* button pressed state machine
		 * if pressed than press button 1, start timer and remember the tab position
		 * if pressed longer than TIME23RDBUTTON and it is not moved more than MOVEMENT23RDBUTTON release button 1 and click button 3
		 * if still pressed do nothing until the pressure is released
		 */
		switch (priv->state) {
			 case BUTTON_EMULATION_OFF:
				if (priv->lastp != samp.pressure) {
					 priv->lastp = samp.pressure;
					 xf86PostButtonEvent(local->dev, TRUE,
						 1, !!samp.pressure, 0, 2,
						 priv->lastx,
						 priv->lasty);
				}
				break;
			case BUTTON_NOT_PRESSED:
				if (samp.pressure) {
					priv->button_down_start = now;
					priv->button_down_y = samp.y;
					priv->button_down_x = samp.x;
					priv->state = BUTTON_1_PRESSED;
					//ErrorF("b1 down");
					xf86PostButtonEvent(local->dev, TRUE,
						priv->state, TRUE, 0, 2,
						priv->lastx,
						priv->lasty);
				}
				break;
			case BUTTON_1_PRESSED:
				if (samp.pressure) {
					//ErrorF("%d %d ",pressureTime.tv_sec,pressureTime.tv_usec);
					if ((((double)pressureTime.tv_sec)+(((double)pressureTime.tv_usec)*1e-6) > TIME23RDBUTTON) &&
					   (abs(priv->lastx-priv->button_down_x) < MOVEMENT23RDBUTTON &&
					    abs(priv->lasty-priv->button_down_y) < MOVEMENT23RDBUTTON)) {
						//ErrorF("b1 up");
						xf86PostButtonEvent(local->dev, TRUE,
							priv->state, FALSE, 0, 2,
							priv->lastx,
							priv->lasty);
						priv->state = BUTTON_3_CLICK;
						//ErrorF("b3 down");
						xf86PostButtonEvent(local->dev, TRUE,
							priv->state, TRUE, 0, 2,
							priv->lastx,
							priv->lasty);
					}
					if (abs(priv->lastx-priv->button_down_x) > MOVEMENT23RDBUTTON ||
					    abs(priv->lasty-priv->button_down_y) > MOVEMENT23RDBUTTON) {
						priv->button_down_start = now;
						priv->button_down_y = samp.y;
						priv->button_down_x = samp.x;
						//ErrorF("b1 state reset");
					}
				} else {
					//ErrorF("b1 up");
					xf86PostButtonEvent(local->dev, TRUE,
						priv->state, FALSE, 0, 2,
						priv->lastx,
						priv->lasty);
					priv->state = BUTTON_NOT_PRESSED;
				}
				break;
			case BUTTON_3_CLICK:
				//ErrorF("b3 up");
				xf86PostButtonEvent(local->dev, TRUE,
					priv->state, FALSE, 0, 2,
					priv->lastx,
					priv->lasty);
				priv->state = BUTTON_3_CLICKED;
				break;
			case BUTTON_3_CLICKED:
				if (!samp.pressure) {
					//ErrorF("b3 free");
					priv->state = BUTTON_NOT_PRESSED;
				}
				break;
		}
	}

	if (ret < 0) {
		ErrorF("ts_read failed\n");
		return;
	}
}

#ifdef TSLIB_VERSION_MT
static void ReadInputMT(InputInfoPtr local)
{
	/* TODO
	 * buffers
	 * ts_read_mt()
	 * xf86PostTouchEvent() per slot / sample
	 *
	 * only if ENOSYS -> ReadInputLegacy()
	 */

	ReadInputLegacy(local);
}
#endif /* TSLIB_VERSION_MT */

static void ReadInput(InputInfoPtr local)
{
#ifdef TSLIB_VERSION_MT
	ReadInputMT(local);
#else
	ReadInputLegacy(local);
#endif
}

static void xf86TslibInitButtonLabels(Atom *labels, int nlabels)
{
	memset(labels, 0, nlabels * sizeof(Atom));
	switch (nlabels) {
		default:
		case 7:
			labels[6] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_RIGHT);
		case 6:
			labels[5] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_LEFT);
		case 5:
			labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);
		case 4:
			labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
		case 3:
			labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
		case 2:
			labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
		case 1:
			labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
			break;
	}
}

/*
 * xf86TslibControlProc --
 *
 * called to change the state of a device.
 */
static int
xf86TslibControlProc(DeviceIntPtr device, int what)
{
	InputInfoPtr pInfo;
	unsigned char map[MAXBUTTONS + 1];
	Atom labels[MAXBUTTONS];
	int i, axiswidth, axisheight;
	struct ts_priv *priv;

#ifdef DEBUG
	ErrorF("%s\n", __FUNCTION__);
#endif
	pInfo = device->public.devicePrivate;
	priv = pInfo->private;

	switch (what) {
	case DEVICE_INIT:
		device->public.on = FALSE;

		for (i = 0; i < MAXBUTTONS; i++) {
			map[i + 1] = i + 1;
		}
		xf86TslibInitButtonLabels(labels, MAXBUTTONS);

		if (InitButtonClassDeviceStruct(device, MAXBUTTONS,
						labels,
						map) == FALSE) {
			ErrorF("unable to allocate Button class device\n");
			return !Success;
		}

		if (InitValuatorClassDeviceStruct(device,
						  2,
						  labels,
						  0, Absolute) == FALSE) {
			ErrorF("unable to allocate Valuator class device\n");
			return !Success;
		}

		switch (priv->rotate) {
		case TSLIB_ROTATE_CW:
		case TSLIB_ROTATE_CCW:
			axiswidth = priv->height;
			axisheight = priv->width;
			break;
		default:
			axiswidth = priv->width;
			axisheight = priv->height;
			break;
		}

		InitValuatorAxisStruct(device, 0,
					       XIGetKnownProperty(AXIS_LABEL_PROP_ABS_X),
					       0,		/* min val */
					       axiswidth - 1,	/* max val */
					       axiswidth,	/* resolution */
					       0,		/* min_res */
					       axiswidth	/* max_res */
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 12
					       , Absolute
#endif
					       );

		InitValuatorAxisStruct(device, 1,
					       XIGetKnownProperty(AXIS_LABEL_PROP_ABS_Y),
					       0,		/* min val */
					       axisheight - 1,	/* max val */
					       axisheight,	/* resolution */
					       0,		/* min_res */
					       axisheight	/* max_res */
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 12
					       , Absolute
#endif
					       );

		if (InitProximityClassDeviceStruct(device) == FALSE) {
			ErrorF("Unable to allocate EVTouch touchscreen ProximityClassDeviceStruct\n");
			return !Success;
		}

		if (!InitPtrFeedbackClassDeviceStruct(device, PointerControlProc))
			return !Success;
		break;

	case DEVICE_ON:
#if HAVE_THREADED_INPUT
		xf86AddEnabledDevice(pInfo);
#else
		AddEnabledDevice(pInfo->fd);
#endif
		device->public.on = TRUE;
		break;

	case DEVICE_OFF:
	case DEVICE_CLOSE:
#if HAVE_THREADED_INPUT
		if (pInfo->fd != -1)
			xf86RemoveEnabledDevice(pInfo);
#endif
		device->public.on = FALSE;
		break;
	}
	return Success;
}

/*
 * xf86TslibUninit --
 *
 * called when the driver is unloaded.
 */
static void
xf86TslibUninit(InputDriverPtr drv, InputInfoPtr pInfo, int flags)
{
	struct ts_priv *priv = (struct ts_priv *)(pInfo->private);
#ifdef DEBUG
	ErrorF("%s\n", __FUNCTION__);
#endif

	xf86TslibControlProc(pInfo->dev, DEVICE_OFF);
	ts_close(priv->ts);
	free(pInfo->private);
	pInfo->private = NULL;
	xf86DeleteInput(pInfo, 0);
}

/*
 * xf86TslibInit --
 *
 * called when the module subsection is found in XF86Config
 */
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 12
static int
xf86TslibInit(InputDriverPtr drv, InputInfoPtr pInfo, int flags)
#else
static InputInfoPtr
xf86TslibInit(InputDriverPtr drv, IDevPtr dev, int flags)
#endif
{
	struct ts_priv *priv;
	char *s;
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
	InputInfoPtr pInfo;
#endif

	priv = calloc(1, sizeof (struct ts_priv));
	if (!priv)
		return BadValue;

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
	if (!(pInfo = xf86AllocateInput(drv, 0))) {
		free(priv);
		return BadValue;
	}

	/* Initialise the InputInfoRec. */
	pInfo->name = dev->identifier;
	pInfo->flags =
	    XI86_KEYBOARD_CAPABLE | XI86_POINTER_CAPABLE |
	    XI86_SEND_DRAG_EVENTS;
	pInfo->conf_idev = dev;
	pInfo->close_proc = NULL;
	pInfo->conversion_proc = ConvertProc;
	pInfo->reverse_conversion_proc = NULL;
	pInfo->private_flags = 0;
	pInfo->always_core_feedback = 0;
#endif

	pInfo->type_name = XI_TOUCHSCREEN;
	pInfo->control_proc = NULL;
	pInfo->read_input = ReadInput;
	pInfo->device_control = xf86TslibControlProc;
	pInfo->switch_mode = NULL;
	pInfo->private = priv;
	pInfo->dev = NULL;

	/* Collect the options, and process the common options. */
	COLLECT_INPUT_OPTIONS(pInfo, NULL);
	xf86ProcessCommonOptions(pInfo, pInfo->options);

	priv->screen_num = xf86SetIntOption(pInfo->options, "ScreenNumber", 0);

	priv->width = xf86SetIntOption(pInfo->options, "Width", 0);
	if (priv->width <= 0)
		priv->width = screenInfo.screens[0]->width;

	priv->height = xf86SetIntOption(pInfo->options, "Height", 0);
	if (priv->height <= 0)
		priv->height = screenInfo.screens[0]->height;

	s = xf86SetStrOption(pInfo->options, "Rotate", NULL);
	if (s) {
		if (strcmp(s, "CW") == 0) {
			priv->rotate = TSLIB_ROTATE_CW;
		} else if (strcmp(s, "UD") == 0) {
			priv->rotate = TSLIB_ROTATE_UD;
		} else if (strcmp(s, "CCW") == 0) {
			priv->rotate = TSLIB_ROTATE_CCW;
		} else {
			priv->rotate = TSLIB_ROTATE_NONE;
		}
	} else {
		priv->rotate = TSLIB_ROTATE_NONE;
	}

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
	s = xf86CheckStrOption(dev->commonOptions, "path", NULL);
#else
	s = xf86CheckStrOption(pInfo->options, "path", NULL);
#endif
	if (!s)
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
		s = xf86CheckStrOption(dev->commonOptions, "Device", NULL);
#else
		s = xf86CheckStrOption(pInfo->options, "Device", NULL);
#endif

	priv->ts = ts_open(s, 1);
	free(s);

	if (!priv->ts) {
		ErrorF("ts_open failed (device=%s)\n", s);
		xf86DeleteInput(pInfo, 0);
		return BadValue;
	}

	if (ts_config(priv->ts)) {
		ErrorF("ts_config failed\n");
		xf86DeleteInput(pInfo, 0);
		return BadValue;
	}

	pInfo->fd = ts_fd(priv->ts);

	priv->state = BUTTON_NOT_PRESSED;
	if (xf86SetIntOption(pInfo->options, "EmulateRightButton", 0) == 0)
		priv->state = BUTTON_EMULATION_OFF;

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
	/* Mark the device configured */
	pInfo->flags |= XI86_CONFIGURED;
#endif

	/* Return the configured device */
	return Success;
}

_X_EXPORT InputDriverRec TSLIB = {
	1,			/* driver version */
	"tslib",		/* driver name */
	NULL,			/* identify */
	xf86TslibInit,		/* pre-init */
	xf86TslibUninit,	/* un-init */
	NULL,			/* module */
	NULL,			/* (ref count) new: default options */
#ifdef XI86_DRV_CAP_SERVER_FD
	0			/* TODO add this capability */
#endif
};

/*
 ***************************************************************************
 *
 * Dynamic loading functions
 *
 ***************************************************************************
 */

/*
 * xf86TslibUnplug --
 *
 * called when the module subsection is found in XF86Config
 */
static void xf86TslibUnplug(pointer p)
{
}

/*
 * xf86TslibPlug --
 *
 * called when the module subsection is found in XF86Config
 */
static pointer xf86TslibPlug(pointer module, pointer options, int *errmaj,
			     int *errmin)
{
	static Bool Initialised = FALSE;

	xf86AddInputDriver(&TSLIB, module, 0);

	return module;
}

static XF86ModuleVersionInfo xf86TslibVersionRec = {
	"tslib",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	PACKAGE_VERSION_MAJOR, PACKAGE_VERSION_MINOR, PACKAGE_VERSION_PATCHLEVEL,
	ABI_CLASS_XINPUT,
	ABI_XINPUT_VERSION,
	MOD_CLASS_XINPUT,
	{0, 0, 0, 0}		/* signature, to be patched into the file by */
	/* a tool */
};

_X_EXPORT XF86ModuleData tslibModuleData = {
	&xf86TslibVersionRec,
	xf86TslibPlug,
	xf86TslibUnplug
};
