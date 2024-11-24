Currently, the aes and des tests in the demo need to go to the root directory of the warehouse and add the following macro control code into xmake.lua

add_defines("MBEDTLS_CIPHER_MODE_CBC",{public = true})
add_defines("MBEDTLS_CIPHER_MODE_CFB",{public = true})
add_defines("MBEDTLS_CIPHER_MODE_CTR",{public = true})
add_defines("MBEDTLS_CIPHER_MODE_OFB",{public = true})
add_defines("MBEDTLS_CIPHER_MODE_XTS",{public = true}) 

