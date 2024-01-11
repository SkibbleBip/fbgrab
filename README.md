# fbgrab

**fbgrab** is a simple command-line utility for capturing the current frame in the framebuffer and saving it as a .bmp file. It includes an option for optional lossy compression suitable for thumbnailing.

## Usage

```bash
./fbgrab --input/-i <input_file> --output/-o <output_file> --resolution/-r <resolution>
```

### Options

- **--input/-i**: Specifies the input file (framebuffer device). This is an optional parameter.
- **--output/-o**: Specifies the output file where the captured frame will be saved as a .bmp file. This is an optional parameter.
- **--resolution/-r**: Specifies the fraction of the original captured frame. This is an optional parameter.

## Example

```bash
./fbgrab -i /dev/fb0 -o output.bmp -r 4
```

This example captures the current frame from the framebuffer device `/dev/fb0`, saves it as `output.bmp`, and saves it as 1/4th of the original frame resolution.

## Build

To build the fbgrab executable, you can use the provided Makefile:

```bash
make
```

## Clean

To clean the build directory:

```bash
make clean
```

## License

This project is licensed under the [GNU General Public License v2.0](LICENSE).

Feel free to contribute and report issues.
