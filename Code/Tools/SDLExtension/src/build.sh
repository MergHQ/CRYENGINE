#!/bin/bash

# Create a symbolic link to Code/SDKs/SDL2. (symbolic link created during bootstraping)

ndk-build NDK_MODULE_PATH=.:../../../SDKs/ APP_PLATFORM=android-19 APP_ABI=armeabi-v7a APP_OPTIM=release

cp libs/armeabi-v7a/libSDL2Ext.so ../lib/android-armeabi-v7a/libSDL2Ext.so
