.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = python3-clx-utils_0.1-1_all.deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	pushd ./clounix-utils

	# Build source and Debian packages
	rm -rf deb_dist/*
	python3 setup.py --command-packages=stdeb.command bdist_deb
	popd

	# Move the newly-built .deb package to the destination directory
	mv ./clounix-utils/deb_dist/$* $(DEST)/
