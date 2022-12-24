#!/bin/bash

strace dbus-send --address='unix:path=/tmp/dbus-test-socket' \
	  --dest=org.freedesktop.ExampleName \
	  /org/freedesktop/sample/object/name \
	  org.freedesktop.ExampleInterface.ExampleMethod
echo $?
