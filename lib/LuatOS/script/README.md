# LuatOS script library

## Directory description

|Directory name|Directory content|Key points|Typical examples|
|--------|-------|----|---------|
|[libs](libs)|External library collection|Peripheral driver or control|Make peripheral driver into a standardized library|[ADS1115](libs/peripheral/ADS1115)|
|[turnkey](turnkey)|Quasi project level solution|Demonstration of real project|[Scanner gun turnkey](turnkey/scanner_air105/)|
|[corelib](corelib)|Core library|System core library|Already built into the firmware|Basic firmware, no modifications|

## demo brief description

For the Lua library embedded in LuatOS (basically implemented in C), demonstrate the usage of function calls. In principle, when adding a built-in library, the corresponding apidemo must be submitted.

## turnkey brief description

Extract the core logic of real projects into a turnkey solution and provide

1. Detailed source code comments
2. Development documentation
3. Hardware requirements
4. Effect demonstration, deployment demonstration

## libs brief description

Currently it mainly stores `peripheral drivers` to quickly control peripherals or read peripheral data.
