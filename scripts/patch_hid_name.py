Import("env")
import os

# Patch the Arduino framework's USBHID.cpp to use our custom device name
# instead of the hardcoded "TinyUSB HID" string.
# This string appears in Windows joy.cpl as the controller name.
# The USB product string (USB_PRODUCT) only shows in Device Manager,
# while joy.cpl uses the HID interface string descriptor.
#
# IMPORTANT: This patch runs IMMEDIATELY at script load time (pre: phase)
# so it executes BEFORE any library compilation starts.

framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
usbhid_path = os.path.join(framework_dir, "libraries", "USB", "src", "USBHID.cpp")

if not os.path.exists(usbhid_path):
    print("WARNING: USBHID.cpp not found at", usbhid_path)
else:
    # Get USB_PRODUCT from build flags
    product_name = None
    for flag in env.get("BUILD_FLAGS", []):
        if "USB_PRODUCT" in str(flag):
            name = str(flag).split("=", 1)[1] if "=" in str(flag) else None
            if name:
                product_name = name.replace('\\"', '').replace('"', '').strip()
                break

    if not product_name:
        product_name = "ESP-ButtonBox-WHEEL"

    with open(usbhid_path, "r") as f:
        content = f.read()

    old_str = 'tinyusb_add_string_descriptor("TinyUSB HID")'
    new_str = 'tinyusb_add_string_descriptor("{}")'.format(product_name)

    if old_str in content:
        content = content.replace(old_str, new_str)
        with open(usbhid_path, "w") as f:
            f.write(content)
        print("PATCHED: USBHID.cpp HID interface name -> '{}'".format(product_name))
        # Force recompile of USBHID by touching the file
        os.utime(usbhid_path, None)
    elif new_str in content:
        print("OK: USBHID.cpp already patched with '{}'".format(product_name))
    else:
        print("WARNING: Could not find expected string in USBHID.cpp")
