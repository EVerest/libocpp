#!/bin/sh

exit 0
ninja -C "$EXT_MOUNT/build" install
retVal=$?

if [ $retVal -ne 0 ]; then
    echo "Installation failed with return code $retVal"
    exit $retVal
fi
