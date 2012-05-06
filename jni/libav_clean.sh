#!/bin/bash

(
	cd libav
	git clean -fx
	git checkout .
	git status
)

