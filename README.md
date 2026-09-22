# Pico 1-Wire Temperature sensor

This project uses the [RP2040](https://www.raspberrypi.com/products/raspberry-pi-pico/), [DS18B20](https://www.analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf) and MQTT to create a multi-point temperature probe.

It uses MQTT to send temperature data and features a C++ implementation of the 1-Wire algorithm based on stefanalt's [RP2040-PIO-1-Wire-Master](https://github.com/stefanalt/RP2040-PIO-1-Wire-Master).

## Dependencies

- [Pico SDK](https://github.com/raspberrypi/pico-sdk) 2.3.1 or newer (tested with 2.3.1), with its submodules initialized
- CMake 3.21 or newer, Ninja, Python 3, Git, and an Arm GNU toolchain (`arm-none-eabi-gcc` and `arm-none-eabi-g++` with C++20 support)
- A native C/C++ compiler to build the SDK's `pioasm` and `picotool` tools

## Building

Clone this repository:

```bash
git clone  https://github.com/andy-held/RP2040-PIO-1-Wire-Master.git
cd RP2040-PIO-1-Wire-Master
```

Point CMake at the local Pico SDK 2.3.1 checkout and initialize its dependencies:

```bash
export PICO_SDK_PATH=<path to sdk>
git clone https://github.com/raspberrypi/pico-sdk.git --recurse-submodules ${PICO_SDK_PATH}
```

Configure and build the Pico W firmware:

```bash
cmake --preset unixlike-release -DPICO_SDK_PATH="$PICO_SDK_PATH"
cmake --build --preset unixlike-release --parallel
```

The firmware is written to `out/build/unixlike-release/src/picomultipointtemp.uf2`.
Use `unixlike-debug` in both commands for a debug build. When upgrading an existing
build from an older SDK, add `--fresh` to the configure command (CMake 3.24 or newer),
or remove that preset's build directory before configuring.

The first configure downloads `project_options` and, unless a compatible installation
is available, the SDK's matching `picotool`. Internet access is needed for these
downloads. `picotool` generates UF2 files in SDK 2.x; CMake builds it automatically.
The explicit `-DPICO_SDK_PATH` setting takes precedence over the environment variable.
