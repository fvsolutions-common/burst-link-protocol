# BURST interface 
Binary Utility for Reliable Stream Transfer (BURST) is a library for encoding and decoding binary data streams, a packet format.
It combines a 16 bit checksum and cobs encoding to convert packets into a format that can be sent over a stream.

This projects is written so it can be used both in python, c and c++ based project

# Installation instuctions

## As an user

Simple installation
```sh
pip install -e .
```

## As a developer

Fast build
```sh
pip install --no-build-isolation -ve .
```

Auto rebuild on run
```sh
pip install --no-build-isolation -Ceditable.rebuild=true -ve .
```


### Python Stub files generation

They are generated automatically buy can also be generated 

```
python -m nanobind.stubgen -m nanobind_example_ext
```

# Publishing instructions

```

```

# Test

```sh
pytest
```

# BURST protocol
TODO
* STAGE 1
    * Convert cpp to c files [OK]
    * Formalise naming [OK]
    * Add c encode functions [OK]
    * Test c encode functions [OK]
    * Update README 
    * Improve poetry.toml   [OK]


* STAGE 2
    * Add CI/CD on github to compile x86
       * Fix dependencies once compilation succeeds
    * Publish on pypi
* STAGE 3
    * Add a way to get C test coverage




