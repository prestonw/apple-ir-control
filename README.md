# Apple IR Control Utility

`apple-ir-control` is a command-line utility designed to manage the enabled/disabled state of the infrared (IR) remote control on Mac computers. This utility provides a programmable interface for the equivalent functionality found in the System Preferences path: **Security & Privacy > Advanced > Disable remote control infrared receiver**.

## Modifications and Updates

This version introduces compatibility updates for macOS Ventura 13.5 (22G74) and addresses deprecation warnings related to `kIOMasterPortDefault`. The code has been tested and verified to work on the latest macOS versions up to Ventura 13.5.

## New Authorship

Originally developed by Robert Sesek, this updated version has been modified and maintained by Will Preston (will@preston.ie). The modifications include code adjustments for newer macOS compatibility and general maintenance updates.

## Source Code

The original source code for this project is available at [Robert Sesek's Repository](https://src.bluestatic.org/?p=apple-ir-control.git). The updated version maintained by Will Preston is hosted on GitHub at [Will Preston's GitHub Repository](https://github.com/prestonw/apple-ir-control).

## Usage

To use `apple-ir-control`, compile the source code with the provided Makefile and run the executable with either `on` or `off` as an argument to enable or disable the Mac's IR receiver:

```bash
./apple-ir-control on  # Enables the IR receiver
./apple-ir-control off # Disables the IR receiver


Administrative privileges (sudo) may be required to modify the IR receiver state.

## License

This program is distributed under the terms of the GNU General Public License, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

## Acknowledgments

Special thanks to Robert Sesek for the original development of this utility. This project stands on the foundation of his work and continues to evolve to meet the needs of the latest macOS environments.

## Brief Description

apple-ir-control is a command line utility to get and set the enabled/disabled
state of the IR remote control on Macs.

This is equivalent of System Preferences > Security > Advanced > Disable remote
