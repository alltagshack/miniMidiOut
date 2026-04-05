Import("env")
import shutil, os

src = os.path.join(env['PROJECT_DIR'], "extra", "UsbCore.h")
dst = os.path.join(env['PROJECT_DIR'], ".pio", "libdeps", "uno", "USB-Host-Shield-20", "UsbCore.h")
shutil.copyfile(src, dst)