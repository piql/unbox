set -e
#sudo apt install libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev
cc -o raw_viewer -O2\
 -D_GLFW_X11\
 -DSUPPORT_TRACELOG_DEBUG\
 -DPLATFORM_DESKTOP\
 -Idep/raylib/src/external/glfw/include\
 dev/raw_viewer.c\
 dep/raylib/src/rcore.c\
 dep/raylib/src/rtext.c\
 dep/raylib/src/rtextures.c\
 dep/raylib/src/rshapes.c\
 dep/raylib/src/utils.c\
 dep/raylib/src/external/glfw/src/monitor.c\
 dep/raylib/src/external/glfw/src/window.c\
 dep/raylib/src/external/glfw/src/context.c\
 dep/raylib/src/external/glfw/src/glx_context.c\
 dep/raylib/src/external/glfw/src/egl_context.c\
 dep/raylib/src/external/glfw/src/osmesa_context.c\
 dep/raylib/src/external/glfw/src/input.c\
 dep/raylib/src/external/glfw/src/init.c\
 dep/raylib/src/external/glfw/src/x11_window.c\
 dep/raylib/src/external/glfw/src/x11_init.c\
 dep/raylib/src/external/glfw/src/x11_monitor.c\
 dep/raylib/src/external/glfw/src/xkb_unicode.c\
 dep/raylib/src/external/glfw/src/platform.c\
 dep/raylib/src/external/glfw/src/posix_thread.c\
 dep/raylib/src/external/glfw/src/posix_time.c\
 dep/raylib/src/external/glfw/src/linux_joystick.c\
 dep/raylib/src/external/glfw/src/posix_poll.c\
 dep/raylib/src/external/glfw/src/posix_module.c\
 dep/raylib/src/external/glfw/src/vulkan.c\
 -lm
./raw_viewer ../../../../Documents/reel.raw
