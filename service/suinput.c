/*
suinput - Simple C-API to the Linux uinput-system.
Copyright (C) 2009 Tuomas Räsänen <tuos@codegrove.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
//#include <linux/uinput.h>
#include "uinput.h"
#include <stdio.h>

#include "suinput.h"

char* UINPUT_FILEPATHS[] = {
//  "/android/dev/uinput",
  "/dev/uinput",
  "/dev/input/uinput",
  "/dev/misc/uinput",
};
#define UINPUT_FILEPATHS_COUNT (sizeof(UINPUT_FILEPATHS) / sizeof(char*))

static int suinput_write(int uinput_fd,
                         uint16_t type, uint16_t code, int32_t value)
{
  struct input_event event;
  memset(&event, 0, sizeof(event));
  gettimeofday(&event.time, 0); /* This should not be able to fail ever.. */
  event.type = type;
  event.code = code;
  event.value = value;
  if (write(uinput_fd, &event, sizeof(event)) != sizeof(event))
    return -1;
  return 0;
}

int suinput_mouse(int uinput_fd,
			int32_t x, int32_t y, int32_t down)
{
	if (suinput_write(uinput_fd, EV_ABS, ABS_X, x) ||
		suinput_write(uinput_fd, EV_ABS, ABS_Y, y) ||
		suinput_write(uinput_fd, EV_KEY,BTN_TOUCH, down) ||
		suinput_write(uinput_fd, EV_SYN, SYN_REPORT, 0))
		return -1;
	return 0;
}

/*
int suinput_mouse_relative(int uinput_fd,
			int32_t x, int32_t y, int32_t down)
{
	if (suinput_write(uinput_fd, EV_REL, REL_X, x) ||
		suinput_write(uinput_fd, EV_REL, REL_Y, y) ||
		suinput_write(uinput_fd, EV_KEY,BTN_TOUCH, down) ||
		suinput_write(uinput_fd, EV_SYN, SYN_REPORT, 0))
		return -1;
	return 0;
}
*/

static int suinput_write_syn(int uinput_fd,
                             uint16_t type, uint16_t code, int32_t value)
{
  if (suinput_write(uinput_fd, type, code, value))
    return -1;
  return suinput_write(uinput_fd, EV_SYN, SYN_REPORT, 0);
}

int suinput_open(const char* device_name, const struct input_id* id, bool mouse)
{
  int original_errno = 0;
  int uinput_fd = -1;
  struct uinput_user_dev user_dev;
  int i;

  for (i = 0; i < UINPUT_FILEPATHS_COUNT; ++i) {
    uinput_fd = open(UINPUT_FILEPATHS[i], O_WRONLY | O_NONBLOCK);
    if (uinput_fd != -1)
      break;
  }
printf("0\n");

  if (uinput_fd == -1)
    return -1;

  /* Set device to handle following types of events: */

  /* Key and button events */
  if (ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY) == -1)
    goto err;
printf("1\n");

  /* Key and button repetition events */
  if (ioctl(uinput_fd, UI_SET_EVBIT, EV_REP) == -1)
    goto err;
printf("2\n");

  /* Relative pointer motions */

//if (ioctl(uinput_fd, UI_SET_EVBIT, EV_REL) == -1)
//    goto err;
printf("3\n");
    /* Absolute pointer motions */

    if (mouse)
    {
	    if (ioctl(uinput_fd, UI_SET_EVBIT, EV_ABS) == -1)
		goto err;
    }
//	    if (ioctl(uinput_fd, UI_SET_EVBIT, EV_REL) == -1)
//		goto err;

  /* Synchronization events, this is probably set implicitely too. */
  if (ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN) == -1)
    goto err;
printf("4\n");

  /* Configure device to handle relative x and y axis. */
//  if (ioctl(uinput_fd, UI_SET_RELBIT, REL_X) == -1)
//    goto err;
printf("5\n");
//  if (ioctl(uinput_fd, UI_SET_RELBIT, REL_Y) == -1)
//    goto err;

    if (mouse)
    {
	    if (ioctl(uinput_fd, UI_SET_ABSBIT, ABS_X) == -1)
		goto err;
	    if (ioctl(uinput_fd, UI_SET_ABSBIT, ABS_Y) == -1)
		goto err;
    }
//	    if (ioctl(uinput_fd, UI_SET_RELBIT, REL_X) == -1)
//		goto err;
//	    if (ioctl(uinput_fd, UI_SET_RELBIT, REL_Y) == -1)
//		goto err;
printf("6\n");

  /* Configure device to handle all keys, see linux/input.h. */
  for (i = 0; i < KEY_MAX; i++) {
    if (ioctl(uinput_fd, UI_SET_KEYBIT, i) == -1)
      goto err;
  }
printf("7\n");

  /* Set device-specific information. */
  memset(&user_dev, 0, sizeof(user_dev));
  strncpy(user_dev.name, device_name, UINPUT_MAX_NAME_SIZE);
  user_dev.id.bustype = id->bustype;
  user_dev.id.vendor = id->vendor;
  user_dev.id.product = id->product;
  user_dev.id.version = id->version;

// from dorid VNC server
    //minor tweak to support ABSolute events

    if (mouse)
    {
	    user_dev.absmin[ABS_X] = -2047;
	    user_dev.absmax[ABS_X] = 2048;
	    user_dev.absfuzz[ABS_X] = 0;
	    user_dev.absflat[ABS_X] = 0;

	    user_dev.absmin[ABS_Y] = -2047;
	    user_dev.absmax[ABS_Y] = 2048;
	    user_dev.absfuzz[ABS_Y] = 0;
	    user_dev.absflat[ABS_Y] = 0;
    }
//	    user_dev.absmin[REL_X] = -2047;
//	    user_dev.absmax[REL_X] = 2048;
//	    user_dev.absfuzz[REL_X] = 0;
//	    user_dev.absflat[REL_X] = 0;
//
//	    user_dev.absmin[REL_Y] = -2047;
//	    user_dev.absmax[REL_Y] = 2048;
//	    user_dev.absfuzz[REL_Y] = 0;
//	    user_dev.absflat[REL_Y] = 0;

  if (write(uinput_fd, &user_dev, sizeof(user_dev)) != sizeof(user_dev))
    goto err;
printf("8\n");

  if (ioctl(uinput_fd, UI_DEV_CREATE) == -1)
    goto err;
printf("9\n");


  /*
  The reason for generating a small delay is that creating succesfully
  an uinput device does not guarantee that the device is ready to process
  input events. It's probably due the asynchronous nature of the udev.
  However, my experiments show that the device is not ready to process input
  events even after a device creation event is received from udev.
  */
//  sleep(3);

printf("10\n");
  return uinput_fd;

 err:

  /*
    At this point, errno is set for some reason. However, cleanup-actions
    can also fail and reset errno, therefore we store the original one
    and reset it before returning.
  */
  original_errno = errno;

  /* Cleanup. */
  close(uinput_fd); /* Might fail, but we don't care anymore at this point. */

  errno = original_errno;
  return -1;
}

int suinput_close(int uinput_fd)
{
  /*
    Sleep before destroying the device because there still can be some
    unprocessed events. This is not the right way, but I am still
    looking for better ways. The question is: how to know whether there
    are any unprocessed uinput events?
   */
  sleep(2);

  if (ioctl(uinput_fd, UI_DEV_DESTROY) == -1) {
    close(uinput_fd);
    return -1;
  }

  if (close(uinput_fd) == -1)
    return -1;

  return 0;
}

int suinput_move_pointer(int uinput_fd, int32_t x, int32_t y)
{
  if (suinput_write(uinput_fd, EV_REL, REL_X, x))
    return -1;
  return suinput_write_syn(uinput_fd, EV_REL, REL_Y, y);
}

int suinput_press(int uinput_fd, uint16_t code)
{
  return suinput_write_syn(uinput_fd, EV_KEY, code, 1);
}

int suinput_release(int uinput_fd, uint16_t code)
{
  return suinput_write_syn(uinput_fd, EV_KEY, code, 0);
}

int suinput_click(int uinput_fd, uint16_t code, long time)
{
  if (suinput_press(uinput_fd, code))
    return -1;
  if (time)
    usleep(time*1000);
  return suinput_release(uinput_fd, code);
}
