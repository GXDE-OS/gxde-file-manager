The reusable Qt compatibility regression suite now lives in **gxde-bernese**.
It covers text layout, filters, blur, window helpers, and fractional-scale
backdrop coordinates without loading DTK or the file manager.

This directory retains file-manager-specific plugin loading, ABI, thread
dispatch and direct Qt-private-symbol checks:

```sh
cmake -S tests -B /tmp/gxde-file-manager-tests \
  -DCMAKE_PREFIX_PATH=/path/to/bernese/install \
  -DGXDE_LIBRARY_DIR="$PWD/build/gxde-file-manager-lib"
cmake --build /tmp/gxde-file-manager-tests
ctest --test-dir /tmp/gxde-file-manager-tests --output-on-failure
```

Run as a normal user (root plugins are disabled). The tests use the offscreen
Qt platform and do not interact with the running desktop.
