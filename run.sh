#!/bin/bash

set -xe

gcc -ggdb -Wall -Wextra -o foltrek main.c
./foltrek
