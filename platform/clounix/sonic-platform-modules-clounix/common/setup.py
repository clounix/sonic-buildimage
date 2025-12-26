#!/usr/bin/env python

from setuptools import setup, find_packages

setup(
    name="sonic_platform",
    version="1.0",
    description="Module to initialize Clounix CLX8000-48C8D platforms",
    packages=find_packages(where="common"),
    package_dir={"": "common"},
)

