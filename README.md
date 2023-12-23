# Transcoding JPEG images

A research project on the topic "Transcoding JPEG images using prediction of DCT coefficients based on a neural network". Repository structure:
- `src/gpegdec.cpp` - the source code of the JPEG decoder;
- `Makefile` - makefile containing declarations for building the project;
- `models/model.ipynb` - the Jupiter notebook in which the DCT coefficients are processed.

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

To run the decoder on the list of images and output the DCT coefficients to the out/coefficients.csv file for further processing, use:
```
make process folder=<images/>
```
