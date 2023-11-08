# TS-7100-Z UI Demo Using LVGL

A small demo utilizing [Light and Versatile Graphics Library (LVGL)](https://lvgl.io/) to create a simply and tailored HMI for the TS-7100-Z to quickly demonstrate and interact with its I/O capabilities.

The demo consists of four tabs:
- Pinout: The same image used in the splash-screen that shows the connections on the terminal block.
- Relays: Two buttons to turn on and off either relay.
- HV IO: Control of the 3 low-side switches' output, a reflection of their input, as well as control the the high-side switch output.
- ADC: A meter showing the current voltage on the 4, 0-12 V ADC inputs.


## Notable Features

The demo uses fbdev which is emulated by the kernel's DRM layer. It also uses libinput to handle touchscreen input events. The touchscreen calibration is maintained by a udev file that sets a default calibration on startup.

The GPIO are controlled via gpiod and implement a lazy initialization. If the demo boots up and remains on the first screen then the GPIO pins are no`t claimed. This allows for users to log in to the system and manipulate the GPIO themselves. Once the demo is moved to any other tab for the first time, all necessary GPIO pins are claimed by the demo and no other application is able to claim them until the application is closed.

The ADC inputs are monitored via the kernel's IIO system.

The Pinout screen with the splash-screen image, will move the tabs out of the way after a second and a half to allow the user to view the full image. Touching the display will bring the tabs back up and allow the user to navigate.

Despite offering no graphical hardware acceleration, the demo application takes a very small amount of CPU at run time. However, the inputs are regularly polled rather than implementing an interrupt system. While this is a disadvantage to UI frameworks that can run on a more interrupt bases (e.g. Qt), the CPU utilization from this whole demo is much lower overall than heavier UI frameworks during periods of activity.


## Building

LVGL and its main driver libraries, as well as libgpiod, libinput, and libiio are required to build this application.

In this repository are also two files, `lv_conf.h` and `lv_drv_conf.h`. These files are needed when building LVGL and its drivers in order to configure the main features of the libraries.

The below instructions assume libgpio, libinput, and libiio development headers and libraries are already available, and that the application is being built on the unit itself. Cross compilation is possible and is done with our [Buildroot-ts.git project]() and [distro-seed project]() to output a whole filesystem that includes this application.


LVGL and its main driver libraries are both submodules of this project and are otherwise self contained. Aside from needing some form of standard C library, libgpiod and libiio are the only other libraries needed. No other graphical libraries are required.

First, download this repository to access the needed conf files for building LVGL and its drivers:
```
git clone https://github.com/embeddedTS/ts7100z-lvgl-ui-demo
```

Next, clone and build both LVGL and its drivers:
```
git clone https://github.com/lvgl/lvgl
cd lvgl
cp ../ts7100z-lvgl-ui-demo/lv_conf.h .
mkdir build
cd build
cmake .. -DBUILD_SHARED_LIBS=TRUE
make
make install
cd ../..
```

Now, the drivers can be built and installed:
```
git clone https://github.com/lvgl/lv_drivers
cd lv_drivers
cp ../ts7100z-lvgl-ui-demo/lv_drv_conf.h ../
mkdir build
cd build
cmake .. -DBUILD_SHARED_LIBS=TRUE
make
make install
cd ../..
```

Finally, the UI demo itself can be built:
```
cd ts7100z-lvgl-ui-demo
mkdir build
cd build
cmake ..
make
```

The binary `ts7100z-lvgl-ui-demo` can now be run.


## Running Environment

The UI demo relies on the kernel's DRM framebuffer emulation since the application draws directly to a framebuffer. Support for the emulated framebuffer must be enabled in the kernel, and no other application should be drawing anything to the framebuffer or DRI device. Otherwise the various drawn screnes will interfere with each other.
