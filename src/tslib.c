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
/* test old legacy interface with tslib 1.10+
#undef TSLIB_VERSION_MT
 */

#ifdef TSLIB_VERSION_MT
#include <errno.h>
#endif

#define TOUCH_MAX_SLOTS 15
#define TOUCH_SAMPLES_READ 1
#define TOUCHPAD_NUM_AXES 4 /* x, y, hscroll, vscroll */

#define TIME23RDBUTTON 0.5
#define MOVEMENT23RDBUTTON 4

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

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 23
#define HAVE_THREADED_INPUT	1
#endif

enum { TSLIB_ROTATE_NONE = 0, TSLIB_ROTATE_CW = 270, TSLIB_ROTATE_UD = 180, TSLIB_ROTATE_CCW = 90 };

enum button_state { BUTTON_NOT_PRESSED = 0, BUTTON_1_PRESSED = 1, BUTTON_3_CLICK = 3, BUTTON_3_CLICKED = 4, BUTTON_EMULATION_OFF = -1 };

struct ts_priv {
	struct tsdev *ts;
	int lastx, lasty, lastp;
	int screen_num;
	int rotate;
	int height;
	int width;
	enum button_state state;
	struct timeval button_down_start;
	int button_down_x, button_down_y;

#ifdef TSLIB_VERSION_MT
	struct ts_sample_mt **samp_mt;
	struct ts_sample_mt *last_mt;
	ValuatorMask *valuators;
#endif
};

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
				#ifdef DEBUG
					xf86IDrvMsg(local, X_ERROR, "button 1 down\n");
				#endif
					xf86PostButtonEvent(local->dev, TRUE,
						priv->state, TRUE, 0, 2,
						priv->lastx,
						priv->lasty);
				}
				break;
			case BUTTON_1_PRESSED:
				if (samp.pressure) {
				#ifdef DEBUG
					xf86IDrvMsg(local, X_ERROR,
						    "%d %d ",pressureTime.tv_sec,pressureTime.tv_usec);
				#endif
					if ((((double)pressureTime.tv_sec)+(((double)pressureTime.tv_usec)*1e-6) > TIME23RDBUTTON) &&
					   (abs(priv->lastx-priv->button_down_x) < MOVEMENT23RDBUTTON &&
					    abs(priv->lasty-priv->button_down_y) < MOVEMENT23RDBUTTON)) {
					#ifdef DEBUG
						xf86IDrvMsg(local, X_ERROR, "button 1 up\n");
					#endif
						xf86PostButtonEvent(local->dev, TRUE,
							priv->state, FALSE, 0, 2,
							priv->lastx,
							priv->lasty);
						priv->state = BUTTON_3_CLICK;
					#ifdef DEBUG
						xf86IDrvMsg(local, X_ERROR, "button 3 down\n");
					#endif
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
					#ifdef DEBUG
						xf86IDrvMsg(local, X_ERROR, "button 1 state reset\n");
					#endif
					}
				} else {
				#ifdef DEBUG
					xf86IDrvMsg(local, X_ERROR, "button 1 up\n");
				#endif
					xf86PostButtonEvent(local->dev, TRUE,
						priv->state, FALSE, 0, 2,
						priv->lastx,
						priv->lasty);
					priv->state = BUTTON_NOT_PRESSED;
				}
				break;
			case BUTTON_3_CLICK:
			#ifdef DEBUG
				xf86IDrvMsg(local, X_ERROR, "button 3 up\n");
			#endif
				xf86PostButtonEvent(local->dev, TRUE,
					priv->state, FALSE, 0, 2,
					priv->lastx,
					priv->lasty);
				priv->state = BUTTON_3_CLICKED;
				break;
			case BUTTON_3_CLICKED:
				if (!samp.pressure) {
				#ifdef DEBUG
					xf86IDrvMsg(local, X_ERROR, "button 3 free\n");
				#endif
					priv->state = BUTTON_NOT_PRESSED;
				}
				break;
		}
	}

	if (ret < 0) {
		xf86IDrvMsg(local, X_ERROR, "ts_read failed\n");
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
	unsigned char map[MAX_BUTTONS + 1];
	Atom labels[MAX_BUTTONS];
	int i, axiswidth, axisheight;
	struct ts_priv *priv;

#ifdef DEBUG
	xf86IDrvMsg(pInfo, X_ERROR, "%s\n", __FUNCTION__);
#endif
	pInfo = device->public.devicePrivate;
	priv = pInfo->private;

	switch (what) {
	case DEVICE_INIT:
		device->public.on = FALSE;

		for (i = 0; i < MAX_BUTTONS; i++) {
			map[i + 1] = i + 1;
		}
		xf86TslibInitButtonLabels(labels, MAX_BUTTONS);

		if (InitButtonClassDeviceStruct(device, MAX_BUTTONS,
						labels,
						map) == FALSE) {
			xf86IDrvMsg(pInfo, X_ERROR,
				    "unable to allocate Button class device\n");
			return !Success;
		}

		if (InitValuatorClassDeviceStruct(device,
						  2,
						  labels,
						  0, Absolute) == FALSE) {
			xf86IDrvMsg(pInfo, X_ERROR,
				    "unable to allocate Valuator class device\n");
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
				       axiswidth,	/* max_res */
				       Absolute);

		InitValuatorAxisStruct(device, 1,
				       XIGetKnownProperty(AXIS_LABEL_PROP_ABS_Y),
				       0,		/* min val */
				       axisheight - 1,	/* max val */
				       axisheight,	/* resolution */
				       0,		/* min_res */
				       axisheight,	/* max_res */
				       Absolute);

		if (InitProximityClassDeviceStruct(device) == FALSE) {
			xf86IDrvMsg(pInfo, X_ERROR,
				    "Unable to allocate EVTouch touchscreen ProximityClassDeviceStruct\n");
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
	xf86IDrvMsg(pInfo, X_ERROR, "%s\n", __FUNCTION__);
#endif

#ifdef TSLIB_VERSION_MT
	int i;

	for (i = 0; i < TOUCH_SAMPLES_READ; i++)
		free(priv->samp_mt[i]);

	free(priv->samp_mt);
	free(priv->last_mt);
	valuator_mask_free(&priv->valuators);
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
static int
xf86TslibInit(InputDriverPtr drv, InputInfoPtr pInfo, int flags)
{
	struct ts_priv *priv;
	char *s;
	int i;

	priv = calloc(1, sizeof (struct ts_priv));
	if (!priv)
		return BadValue;

	pInfo->type_name = XI_TOUCHSCREEN;
	pInfo->control_proc = NULL;
	pInfo->read_input = ReadInput;
	pInfo->device_control = xf86TslibControlProc;
	pInfo->switch_mode = NULL;
	pInfo->private = priv;
	pInfo->dev = NULL;

/* TODO use libevdev and get them */
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

	s = xf86CheckStrOption(pInfo->options, "path", NULL);
	if (!s)
		s = xf86CheckStrOption(pInfo->options, "Device", NULL);

	priv->ts = ts_open(s, 1);
	free(s);

	if (!priv->ts) {
		xf86IDrvMsg(pInfo, X_ERROR, "ts_open failed (device=%s)\n", s);
		xf86DeleteInput(pInfo, 0);
		return BadValue;
	}

	if (ts_config(priv->ts)) {
		xf86IDrvMsg(pInfo, X_ERROR, "ts_config failed\n");
		xf86DeleteInput(pInfo, 0);
		return BadValue;
	}

	pInfo->fd = ts_fd(priv->ts);

	priv->state = BUTTON_NOT_PRESSED;
	if (xf86SetIntOption(pInfo->options, "EmulateRightButton", 0) == 0)
		priv->state = BUTTON_EMULATION_OFF;

#ifdef TSLIB_VERSION_MT
	priv->samp_mt = malloc(TOUCH_SAMPLES_READ * sizeof(struct ts_sample_mt *));
	if (!priv->samp_mt)
		return BadValue;

	for (i = 0; i < TOUCH_SAMPLES_READ; i++) {
		priv->samp_mt[i] = calloc(TOUCH_MAX_SLOTS, sizeof(struct ts_sample_mt));
		if (!priv->samp_mt[i])
			return BadValue;
	}

	priv->last_mt = calloc(TOUCH_MAX_SLOTS, sizeof(struct ts_sample_mt));
	if (!priv->last_mt)
		return BadValue;

	priv->valuators = valuator_mask_new(6);
	if (!priv->valuators)
		return BadValue;
#endif /* TSLIB_VERSION_MT */

	/* Return the configured device */
	return Success;
}

_X_EXPORT InputDriverRec TSLIB = {
	.driverVersion	= 1,
	.driverName	= "tslib",
	.PreInit	= xf86TslibInit,
	.UnInit		= xf86TslibUninit,
	.module		= NULL,
	.default_options= NULL,
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
 * xf86TslibPlug --
 *
 * called when the module subsection is found in XF86Config
 */
static pointer xf86TslibPlug(pointer module, pointer options, int *errmaj,
			     int *errmin)
{
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
	.vers = &xf86TslibVersionRec,
	.setup = xf86TslibPlug,
	.teardown = NULL
};
