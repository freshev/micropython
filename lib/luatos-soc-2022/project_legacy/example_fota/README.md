Fota CSDK sample usage instructions
1. Differential packet generation
The differential package is generated using the FotaToolkit provided by Yixin. For specific instructions, see the FotaToolkit User Guide.
2. This example tests fota upgrade by requesting the upgrade package via URL. In addition to the correct return of the upgrade interface, the upgrade success indicator can also be used to help determine the success of the upgrade through the different printed content before and after the upgrade.
The old version down start prints the front part as AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA, the new version down start prints the front part as BBBBBBBBBBBBBBBBBBBBBB
BBBBBBBBBBBB. Users can directly run build.bat fota to download the compiled firmware into the Modules. At the same time, open the EPAT monitoring output log and find that the user downloaded the upgrade package for the first time.
The verification is ok. After the Modules upgrade is successful, restart and print the log. Before down start, AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA has been replaced by BBBBBBBBBBBBBBBBBBBBBBBBBBB.
BBB. If you request the upgrade package again for the second time, it will prompt an upgrade package verification error due to mismatch, and the upgrade will not be performed.
3. Users can also generate their own differential packages and put them into the test server. They only need to modify the url in the fota_test_task function to be their own differential package address.
