#!/bin/sh

layer-control /dev/layer0 s enable on
layer-control /dev/layer0 s format B8G8R8
layer-control /dev/layer0 s hstride 3
layer-control /dev/layer0 s vstride 960
layer-control /dev/layer0 s position 0 0 320 240
layer-control /dev/layer0 s dirty
imager /dev/layer0 /test/testimg.rgb
