# Notes on whole package upgrade
1. The size of the generated ap.bin cannot exceed 1984KB, and the generated upgrade package cannot exceed 1472KB.
2. Luatools requires version 2.2.0 and above. Go to Options and Tools->SOC Differential/Full Package Upgrade Package Creation Tool. For the new version of firmware, select the binpkg to be upgraded. You can fill in the version number or not for the user ID. It depends on your own. Does the code control the upgrade version? If you use Hezhou IOT upgrade service, you must fill in the version number! ! ! Select the output path, click Generate, and generate the corresponding xxx.sota file in the output directory as an upgrade package. Download it on the server to upgrade.
3. When upgrading, the traffic is between 1MB and 1.5MB. Pay attention to traffic consumption.
4. The original version of csdk must be V0002 and above. Upgrading from V0001 to V0002 will not work because it involves bootloader modification.

