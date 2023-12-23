# Transcoding JPEG images

A research project on the topic "Transcoding JPEG images using prediction of DCT coefficients based on a neural network". Repository structure:
- `src/gpegdec.cpp` - the source code of the JPEG decoder;
- `Makefile` - makefile containing declarations for building the project.

## Usage

To build the JPEG decoder use:
```
make
```

To clean the build directory use:
```
make clean
```

To run JPEG decoder use:
```
./bin/jpegdec <input.jpg> [<output.ppm>]
```
