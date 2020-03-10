# blocks

This is a Pd external for Roli Blocks, that is built on top of the [ROLI Blocks](https://github.com/WeAreROLI/BLOCKS-SDK) Standalone SDK. It provides the following functionality to use Roli Blocks devices, such as the Lightpad and Seaboard, with Pure Data. 

- Discovering the connected blocks and get the topology information
- Receiving information about the block like battery level, charging, rotation etc.
- Receiving the control button state.
- Assigning names to blocks, if you have multiple blocks of the same type
- Lightpad Blocks
  - Setting blocks in different modes like drum pads, faders, mixer, touch surface and drawing.
  - Setting the grid size and colors of the controls.
  - Getting the touch information, fader and button values from the different modes.
  - Setting fader and button values on the block.
  - Drawing leds, rectangles, triangles and circles
- Seaboard Blocks
  - Controlling all settings of the block like MIDI channel, modes, sensitivity etc.

As the blocks communicate over MIDI with the host software, the host software has no way to detect, if the information sent from the host (colors, fader values etc.) were properly received by the block. This Pd external checks it the block has received all information sent from Pure Data, to make sure, it represents the correct state. If not, the packets are resent. This is especially important when drawing on the blocks. In this case, the right order of the drawing commands is also verified.
With this mechanism the blocks can be also be used very reliable with MIDI over Bluetooth.

![blocks-help.pd](https://github.com/UrbanLienert/blocks/blob/master/blocks-help.png?raw=true)

## Usage

Send and receive messages to and from blocks by name. The default name for a Lightpad Block is `pad` and for a Seaboard Block `seaboard`. If you have multiple blocks of the same type e.g. two Lightpad Blocks, then you have two give at least one block another name to distinguish them with the message: `setname [serialnumber] [newname]`.
You can find the serial numbers of the connected blocks in the Pd console.

Here are some examples for sending messages:
- Show a mixer with 4 channels on the block: `[blockname] mode mixer 4`
- Set the block in drawing mode: `[blockname] mode paint`
- Draw a red square rectangle on the block: `[blockname] 2 2 5 5 0xff0000`

An example for a received message when in mixer mode:
- Receiving button 2 value (on): `[blockname] button 2 1`

**Important: Only use one block object in Pd at the same time for all connected blocks.**

## Building / Installation

To build the external from source make sure to check out the repository with the following command:

`git clone --recursive https://github.com/UrbanLienert/blocks.git`

### macOS
Make sure you have the Apple Developer Tools installed.

Run the following command in the terminal:

`make`

Copy the two files **blocks.pd_darwin** and **blocks-help.pd** from `build/MacOS` to `~/Documents/Pd/externals/`

### Linux
Make sure you have a C++11 compatible compiler installed.

Install the following dependencies
- libxinerama-dev
- libxext-dev
- libx11-dev
- libcurl4-openssl-dev

Run the following command in the terminal:

`make`

Copy the two files **blocks.pd_linux** and **blocks-help.pd** from `build/Linux` to `/usr/local/lib/pd-externals`

## Reporting Bugs

Please submit bug reports and pull requests through the source code repository, or send me an email.

Urban Lienert <mail@urbanlienert.com>
