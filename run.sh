#!/bin/bash

set -xe

gcc -ggdb -Wall -Wextra -o breadch main.c
./breadch
