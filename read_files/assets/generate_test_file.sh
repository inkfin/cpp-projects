#!/bin/bash

# 生成一个1GB的文件，包含随机数据
dd if=/dev/urandom of=test_file.bin bs=1M count=1024 status=progress

