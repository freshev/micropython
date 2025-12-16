

# Preface

Embedded developers usually start with C language, but learning C language is difficult and the entry threshold is relatively high. Many people find it difficult to make complex products due to their own C language bottlenecks. Some people just want to make some gadgets because of their interests and hobbies. It is obviously not cost-effective to spend a lot of time learning C language just because of personal interests. I have seen many people who are not engaged in embedded development related work, but they are very successful in playing with Arduino and are no worse than some so-called commercial products on the market. They cannot erase their interests and hobbies just because others do not engage in related work. We at Hezhou Communications are well aware of this pain point, so we spent a lot of time developing LUAT development, which brought users a simpler development model at the expense of part of the performance. This is not only for individual enthusiasts, but also more than enough for corporate customers. The functions our LUAT development can provide, and it can greatly reduce development time.

As the saying goes, we cannot have both fish and bear's paw. Since our LUAT development runs a LUAT interpreter on an embedded platform, the execution efficiency is definitely not as good as directly developing applications in C language. There are always people in the community asking if our CAT1 has a CSDK development method. After learning about this situation, we worked overtime to open source the development package of the first version of CSDK before mid-May. Provided for geeks. We can satisfy everything customers want!

Then now we have two development methods. To develop simply and efficiently, we provide the LUAT model. In the community, we also provide a large number of document tutorials and video tutorials, so that users can master the development of our Moduless in a very short time, greatly reducing users' learning costs. So for geeks who want to squeeze out CPU performance, we also provide CSDK development methods so that you can also be faster than others in some places.

Normally, our LUAT development and CSDK development can meet most of the usage needs. Do you think that's all there is? No! Of course it's not just that. Since we, Hezhou Communication, chose to be open source, we must carry out open source to the end, and there is nothing left to the end. Do what others dare not do, do what others will not do, be a pioneer in the industry!

"Faced with the step-by-step pressing of friendly companies, our friends worked overtime to hold several meetings and studied the way of playing," and finally decided to open source the elua parser code. Here we are not only open source code, but also equipped with detailed open source documents for the elua parser, so that users can choose which functional Moduless to cut and which functional Moduless to add. Let the user decide which Moduless to execute at the bottom layer and which ones to execute at the top layer.



# Table of contents

[TOC]

# Chapter 1, Lua parser software architecture

## 1.1. What is lua

Lua is a small [script language] (https://baike.baidu.com/item/script language). It was developed in 1993 by a research team composed of Roberto Ierusalimschy, Waldemar Celes and Luiz Henrique de Figueiredo in the Pontifical Catholic University of Rio de Janeiro. It is designed to provide flexible extension and customization functions for applications by flexibly embedding into applications. Lua is written in standard C and can be compiled and run on almost all operating systems and platforms. Lua does not provide a powerful library, which is determined by its positioning. Therefore, Lua is not suitable as a language for developing standalone applications. Lua has a simultaneous JIT project that provides instant compilation capabilities on specific platforms.

## 1.2. Why do we choose lua

Lua scripts can be easily called by [C/C++](https://baike.baidu.com/item/C%2FC%2B%2B) code, or in turn call C/C++ functions, which makes Lua widely used in applications. It can not only be used as an extension script, but also as a normal configuration file, instead of [XML](https://baike.baidu.com/item/XML), ini and other file formats, and it is easier to understand and maintain.  Lua is written from standard C, with concise and beautiful code, and can be [compiled] on almost all operating systems and platforms (https://baike.baidu.com/item/compiled/1258343) and run. A complete Lua interpreter is only 200k, and among all [Script Engines] (https://baike.baidu.com/item/Script Engine/4449478), Lua is the fastest. All of this determines that Lua is the best choice for embedded scripts.

It is because the code of the lua interpreter is small enough and runs the fastest among all script engines that we choose lua as the entry point, and it is definitely not because it is difficult to choose one randomly.


## 1.3. Lua parser architecture

The architecture diagram of Lua language is shown in the figure below. You can take a look at the software architecture. It mainly completes calls to each Modules through a global state machine. There will be no in-depth explanation here. If you are interested, you can read [lua source code] (http://www.lua.org/).

![1](doc/1.jpg)

## 1.4. Lua implementation of embedded platform

Our elua open source project is developed based on Lua5.1. It has cut the corresponding basic library functions and added some libraries for hardware drivers to make it occupy less resources based on the necessary hardware drivers, making it more suitable for running on embedded platforms.

# Chapter 2, elua open source Modules

In the first chapter, we learned what lua is, why we should choose lua, lua parser architecture and design ideas of elua. These are basically some concepts and design concepts, which can be said to be an introduction to this document. Take everyone into the gate of elua.

It is obviously not very useful to only talk about concepts here. So in this chapter, let’s take a look at the code of elua.


Our elua open source Moduless are mainly composed of five parts, namely: newlib, lua, Moduless, lib, and platform. Their corresponding functions are shown in the table below.

| NewLib | Lua | Modules | lib | Platfom |
| ---------- | ---------- | ------------ | ------- | ------------ |
| c library code | parser code | function Modules | function library code | platform adaptation code |

* C library code: mainly some c standard libraries
* parser code: lua kernel
* Function library code: Provide some software algorithms
* Functional Modules: hardware driver Modules, connected to abstract layer
* Platform adaptation code: is the abstract layer of elua, dealing with external code

The functions of the five major Moduless mentioned above are different. They together form our elua open source project, and each large Modules will be subdivided into several small Moduless, one layer after another. There are many connections between each Modules and not independent of each other. The number of files in this open source project reaches 656, and pure code alone occupies 50M of storage space. As the subsequent open source documents progress, we will take out a few subsections to teach you step by step to add other functional Moduless so that the code we write ourselves can also be called by the lua interpreter. By then the number of this file will increase further.



![Number of Files](doc/num.png)



At this stage, the overall directory structure of our elua open source Modules is shown in the figure below. It only lists some important and representative functions and files. From this picture, we can also see that the elua open source project supports those functions by default.

![elua](doc/elua.png)





## 2.1. C library code

For some reasons, our elua open source project cannot use standard C library functions, but instead has a set of dedicated C library code built-in, which is consistent with the naming of the standard C library. However, its internal implementation has undergone great changes.

Some students may encounter a problem when learning STM32, which is serial communication printf redirection. st official does not provide the printf function for us. At this time, if you want to use printf to print the logs to the serial port, you need to redirect printf.

In the standard c library function printf prints the output information to the console. On embedded platforms, there is no console. If you want to use the printf function, there are only two ways.

The first is the method used when using stm32, which is to output and redirect the printf function. Then there is no disadvantage of this method in bare metal programming. When you use operating system programming, such as freertos, you will find a problem. If the task's stack space allocation is relatively small, and printf is used in the task to print data, then stack overflow will occur and the program will crash. This is because when the printf function is used, a large buffer will be defined in the stack space to store data. This buffer is only temporarily defined and will be destructed after use. When using printf, the temporary buffer will be calculated into the task stack size. As long as the task is allocated to a small space, the program stack space overflows will crash. According to my previous experience of trampling, even if you just light a light, as long as you use the printf function, if the stack space of the task is less than 2048, the program will crash. This is not good, right? How can you do so much memory in 2048?

![awkward](doc/awkward.png)



In order to solve this problem, we need to use the second method, which is what we are about to talk about rewriting the c standard library, reducing the space inside it, and changing the implementation method.

This section of C library code corresponds to the contents in the newlib folder in our elua open source project. The work in it is to reimplement the standard C library functions. The main ones are some of the things we often use, such as `#include"string.h"`, `#include"stdio.h"`, `#include"stdlib.h"`, `#include"malloc.h"`, `#include"math.h"`. There are some that are used less, but they will be used in the elua parser, so I won't talk about it here.


The structure of the newlib folder is shown in the figure below, and the number of files involved is also a large number of them. If you are interested, go and see it yourself.

![newlib](doc/newlib.png)

Some functions need to deal with hardware platforms and cannot directly use the standard library. For example, some functions in the stdio library, take the `fopen_ext" function to take a look at it. The implementations in it use some platform-related code.

```c
char *fgets_ext(char *buf, int n, FILE *fp);
FILE *fopen_ext(const char *file, const char *mode);
int fclose_ext(FILE *fp);
int getc_ext(FILE *fp);
int ungetc_ext(int c, FILE *fp);
size_t fread_ext(void *buf, size_t size, size_t count, FILE *fp);
int fseek_ext(FILE *fp, long offset, int whence);
long ftell_ext(FILE *fp);
int feof_ext(FILE *fp);
```

```c
FILE *fopen_ext(const char *file, const char *mode)
{
    FILE *fp = NULL;
E_LUA_SCRIPT_TABLE_SECTION section = LUA_SCRIPT_TABLE_MAX_SECTION;
T_UNCOMPRESS_FILE_TABLE_ITEM *pItem = NULL;
    int fileNameLen = strlen(file);
    
if((!file) || (strlen(file) == 0))
    {
OPENAT_print("[fopen_ext]: para error!\n");
return fp;
    }

If (FindunPompressfeleitem (& Section, & Pitem, File))
    {        
fp = L_CALLOC(1,sizeof(FILE));
        if(fp)
        {
fp->_flags = section;
fp->_cookie = pItem;
        }
fp->_type = LUA_UNCOMPRESS_FILE;

    #ifdef AM_LUA_CRYPTO_SUPPORT
if(strncmp(&file[fileNameLen - 5],".luac", 5) == 0)
        {
fp->_type |= ENC_FILE;
        }
    #endif

//printf("[fopen_ext]: %s %d!\n", file, fp->_type);

    }

return fp;
}
```

Of course, this example cannot explain everything, so let’s look at a function `_malloc_r`.


```c
void* _malloc_r( size_t size )
{
#ifdef MUXUSE_DLMALLOC_MEMORY_AS_LUA_SCRIPT_LOAD
    if(bScriptLoaded)
    {
return dlmalloc(size);
    }
    else
#endif
    {
return CNAME( malloc )( size );
    }
```

 This `_malloc_r` is judged to call `CNAME( malloc )( size )`, and `CNAME( malloc )( size )` eventually points to `platform_malloc`. This is our elua abstraction layer code. In the end, it calls the functions that need to deal with hardware. It is definitely not applicable to directly adopting the standard version of the C library.

```c
void* platform_malloc( size_t size )
{
return OPENAT_malloc( size );
}
```

This is actually an abstract layer, but it abstracts the standard library functions. Enable elua parser to run normally on embedded platforms.

Some people may say, how can I use malloc function and printf function directly when I use other development platforms? There is no problem with using these standard functions directly, so how can I do so many things? Here I would like to remind you that this is because the underlying layer of the development package has processed this part of the code, which may be rewriting or redirecting. For users, you don’t have to worry about anything, just use it. However, this underlying layer is definitely not the standard code of the C library that is used directly.

If you don’t believe it, you can open our ʻiot_sdk_4g_8910` development package to see if there is also a `newlib` folder under the `components` folder.

![iot_sdk_4g_8910-components-newlib](doc/iot_sdk_4g_8910-components-newlib.png)



Then we may have another question. Since the elua open source project is running on ?iot_sdk_4g_8910`, why not use the newlib of `iot_sdk_4g_8910` directly. Let me talk about it here. Since we are doing open source, it means that this elua software package is not only used on ?iot_sdk_4g_8910`. As long as you have the ability, you can run it on any platform. Take stm32 as an example. It did not provide these C standard libraries to users (I don’t know whether it is available now, at least it was not available a few years ago). It doesn’t have it, so wouldn’t it be very complicated to transplant users? It may even be unsuccessful.

## 2.2. Lua parser code

In the previous section, we talked about the reimplementation of the C standard library. As long as we talk about why we need to re-implement the C standard library, since the csdk development package already exists, why do we need to write these two questions again in the elua software package?

So much effort to do something, so who is the C standard library for?

Its usage object is naturally the elua parser. So in this section, we will review the code of the lua parser together.

The lua parser is located in the `elua/lua` folder, and its code structure is shown in the figure below.

![lua parser](doc/luaParser.png)

I divide the lua parser code into three large blocks, namely the core functional part of the virtual machine, the source code parsing and precompiling part, and the embedded library part.

Note: This is my personal division of the source code structure. The purpose is to facilitate understanding. It is not an official definition. I don’t know whether the official division of this structure is.

### 2.2.1. Lua kernel

As the name suggests, the lua kernel is the core code of lua, which is responsible for the operation, scheduling, memory management, input and output device monitoring, and global state machine management functions. These are the most basic functions. The kernel can operate with the kernel behind it. It makes no sense for you to do things that are messy without a kernel.

What are the specific uses in the lua kernel? It is also explained in the following table.

| Source File | Function |
| ---------- | ----------------------------------- |
| lvm.c | Lua Virtual Machine |
| ltm.c | Meta Method |
| ldebug.c | Debug interface |
| lzio.c | General input stream interface |
| lstate.c | Global State Machine Manage Global Information |
| ldo.c | Lua's stack and call structure |
| lfunc.c | Function prototype and closure management |
| lobject.c | Object operation function |
| lgc.c | Garbage recycling mechanism |
| LAPI.C | Lua API |
| lmem.c | Memory Management Interface |
| lopcodes.c | Virtual machine bytecode definition |
| lua.c | lua's executable entry main function |
| lstring.c | String table (retains all strings processed by Lua) |
| ltable.c | Lua table (hash) |

### 2.2.2. Lua parsing and precompiling

Lua language is an interpretive language that cannot be run directly. When running, you need to read out the lua file in the file system, and then you need to translate the lua statement into the corresponding c instruction before it can be run. Then this parsing function needs a separate Modules to be responsible for, which is controlled by the kernel mainly responsible for the lexical analysis, parsing, compilation and other functions of lua scripts.

| Source File | Function |
| --------- | ----------------- |
| lparser.c | Lua's parser |
| luac.c | Lua compiler (save bytecode to file; also list bytecode) |
| ldump.c | Save precompiled Lua blocks |
| lundump.c | Load precompiled Lua blocks |
| llex.c | Lua's lexer |
| lcode.c | Lua's code generator |

### 2.2.3. Some library functions

In addition to allowing the lua virtual machine to run, it also needs to have a built-in write operation library function. Make it easier and simpler to use. We must have the ability to simplify the complex and make it less likely to make mistakes for users to use. This is also the feature of the scripting language.

The following table lists the implementation files of some built-in libraries inside the lua parser, mainly basic libraries, mathematical operations libraries, operating system libraries, table libraries and debugging libraries.


| Source File | Function |
| ---------- | -------------------------------- |
| loadlib.c | Lua's dynamic library loader |
| liolib.c | Standard I/O (and system) library |
| loslib.c | Standard operating system library (this should be coroutines) |
| lauxlib.c | Accessibility features for building Lua libraries |
| linit.c | Library for initializing lua.c |
| ltablib.c | Library for table operations |
| ldblib.c | Interface from Lua to its debug API |
| lmathlib.c | Standard Mathematical Operation Library |
| lbaselib.c | Basic Library |
| lstrlib.c | Standard library for string manipulation and pattern matching |
| print.c | Print bytecode |

## 2.3. Hardware function Modules

In the previous section, we briefly Introductiond the code structure of the lua parser. The lua parser comes with some basic operation libraries and operation libraries by default, which can only implement very simple functions. So far, there should be no problem with the simple operation of the lua script, but we can't see the result of this operation because there is no code written in it that deals with hardware. Even a simple serial port print helloworld it also needs to use the serial port hardware driver.

Not to mention other hardware, they all need to be added artificially, and the lua parser itself does not have these functions. In the `elua/Moduless` folder, we also integrate some commonly used hardware drivers.


![modules](doc/modules.png)

The above picture only lists some commonly used driver libraries, and there are many library files under the actual Moduless folder. Next, take out the adc library and explain it accordingly, how to register the adc driver code in the lua virtual machine. Allows users to perform AdC acquisition control in lua scripts.

### 2.3.1. Register adc

If the user wants the lua kernel to call the relevant libraries of adc, he or she needs to declare the Modules name and the adc Modules initialization function in `auxmods.h`.

```c
#define AUXLIB_ADC      "adc"
LUALIB_API int ( luaopen_adc )( lua_State *L );
```

After the declaration, you need to complete the registration of the Modules name and initialization function in the lua kernel in the `platform_conf.h` file.

```c
_ROM( AUXLIB_ADC, luaopen_adc, adc_map )
```

After registering the adc Modules, it is equivalent to just telling the lua kernel that we provide the relevant driver of adc, but we don’t know what functions are in this driver that can call the lua kernel. At this time, we also need to complete the registration of related call functions of the adc driver in the lua Modules.

In the following code, we have registered three functions that can be called by lua scripts for the adc Modules, namely `open`, `read`, and `close`. The implementation functions in the C language corresponding to these three lua functions are `adc_open()`, `adc_read()`, and `adc_close()`.

```c
const LUA_REG_TYPE adc_map[] =
{ 
{ LSTRKEY( "open" ),  LFUNCVAL( adc_open ) },
{ LSTRKEY( "read" ),  LFUNCVAL( adc_read ) },
{ LSTRKEY( "close" ),  LFUNCVAL( adc_close ) },

{Lnilkey, lnilval}
};

LUALIB_API int luaopen_adc( lua_State *L )
{
luaL_register( L, AUXLIB_ADC, adc_map );

return 1;
}  
```

Then why can I be called by a lua script if I write this way? What is the relationship between them? Next, I will change the writing method of these three scripts. The lua call scripts corresponding to the three functions `open`, `read` and `close` are `adc.open()`, `adc.read()`, and `adc.close()`. Does this look a bit familiar? But for a while, I feel like I can't remember where I have seen it. I will give you five seconds to think about it and think hard! ! !

![emmm](doc/emmm.jpg)

Yes, that’s right! If you have used the adc collection function of our Modules before, I guess you should have thought of this paragraph in the adc routine of the luat script.

```lua
local function ADC0_Task()
local adcValue, voltValue = 0, 0
local result = adc.open(0)-----------1. Is this line very familiar?
while true do
adcValue, voltValue = adc.read(0)-----------2. This line seems to be a bit familiar.
if adcValue ~= 0xffff then
log.info("ADC 0's raw measurement data and voltage value:", adcValue, voltValue)
        end
        sys.wait(1000)
    end
adc.close(0)-------------3. If you see this line, you should have no doubts.
end
```

There seem to be a lot of things in the above Luat script, but in fact, three lines are used for adc, and these three lines are exactly the three statements registered in our adc registration function. As long as we write these three registered statements in the underlying layer, even if there is no implementation in these three functions (the function still needs to be written), it is an empty function. You can also write a lua script for the lua bottom layer to parse, and the lua bottom layer can find the location of the function and execute it. You can write many functions in this function and do very complicated things. This only requires one line for a lua script.

### 2.3.2. Implementation of adc calling function

Although the work we have done now allows the lua kernel to run our custom Modules, of course this Modules is still empty now and has nothing. The lua parser ran here and went back, and did nothing.

```c
// adc.open(id)
static int adc_open(lua_State *L) {
return 1;
}

// adc.read(id)
static int adc_read(lua_State *L) {    
return 2;
}

static int adc_close(lua_State *L) {
return 1;
}
```

We also need to complete the specific implementation methods of calling the function according to specific rules. Come and watch it, I'll change.

![tricks](doc/tricks.jpg)


Cough cough, it's changed.

```c
// adc.open(id)
static int adc_open(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    int ret;

MOD_CHECK_ID(adc, id);

ret = platform_adc_open(id,0);

lua_pushinteger (l, right);

return 1;
}

// adc.read(id)
static int adc_read(lua_State *L) {    
    int id = luaL_checkinteger(L,1);
    int adc, volt;

MOD_CHECK_ID(adc, id);

platform_adc_read(id, &adc, &volt);

lua_pushinteger(L, adc);
lua_pushinteger(L, volt);
   
return 2;
}

static int adc_close(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    int ret;

MOD_CHECK_ID(adc, id);

ret = platform_adc_close(id);

lua_pushinteger (l, right);

return 1;
}
```

### 2.3.3. Several irrelevant functions

In the above three code blocks, we have implemented the opening, query and closing functions of adc respectively. We can find the rules. The implementation structures of the above three functions are very similar and can be represented by the following pseudo-code.

```c
// adc.open(id)
static int adc_open(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
MOD_CHECK_ID(adc, id);
/*User Code*/
    ...........
/*User Code*/
lua_pushinteger(L, ret1);
lua_pushinteger(L, ret2);
lua_pushinteger(L, ret3);
return 1;
}
```

`luaL_checkinteger` function is to check whether the arg parameter of the function is a string and return the string.

`MOD_CHECK_ID` is a macro, the corresponding code is as follows, the purpose is to determine whether the incoming operand is allowed. For example, if our device has only two adcs, then the adc numbers we can operate are 0 and 1. If you give me a 2, the program will terminate and print `adc 2 does not exist`.

```c
// Helper macros
#define MOD_CHECK_ID( mod, id )\
if( !platform_ ## mod ## _exists( id ) )\
return luaL_error( L, #mod" %d does not exist", ( unsigned )id )
```

In lua syntax, functions are allowed to return multiple values   at once. The function of `lua_pushinteger` is to return the running result of lua.

Read the sentence adc in the lua script mentioned above. The two return values   to the left of the equal sign. It is pushed by two sentences in the c code `lua_pushinteger` code.

```lua
adcValue, voltValue = adc.read(0)
```
```c
lua_pushinteger(L, adc);
lua_pushinteger(L, volt);
```
## 2.4. Function library code

After the previous section of study, everyone should have understood how to add some driver libraries. Generally speaking, the driver library provided in Section 2.3 is enough. Sometimes we not only drive the device, but also need to run some software algorithms, such as sha1 and md5 information summary algorithms, as well as some image decoding algorithms. json data processing and so on.

These algorithms are relatively common and are often used. We also built some pure soft algorithms in the elua code package, see the figure below for details.

![lib](doc/lib.png)

Some people may not know what the naming in the picture above are useful, so here is a brief explanation.

| Code Pack | Function |
| --------- | ---------------------------------------------------- |
| crypto | Encryption/digment functions (sha1, sha2, md5, base64, xxtea, aes) |
| iconv | Font encoding conversion (gb2312, ucs2, utf8,) |
| json | Format the *JSON* content or *JSON* file |
| lpng | picture decoding |
| lzma | Compressed file interface |
| pbc | Cryptography library based on bilinear pairs |
| qr_encode | QR code generation |
| zlib | Common compression library |
| zziplib | Lightweight C-language package for reading files from ZIP files |


Just like the previous section, there are too many libraries, and there are a lot of files in these libraries. I was originally going to choose a high-end library to talk about, which is the cryptography library based on bilinear pairs. At this time, I didn't expect that Json was so active and raised his hand to ask for it, so I reluctantly agreed to her. Next, let’s talk about json, it’s the same if you learn from one example and apply it to others.

![json](doc/json.jpg)

### 2.4.1, json registration

Like hardware drivers, even if json is a pure soft computing library, there is nothing special about it. You have to have everything you should have.

If you want json to be called in a lua script, you must first add the Modules name and registration function that declares the json Modules in `auxmods.h`.

```c
#ifndef AM_JSON_NOT_SUPPORT
#define AUXLIB_JSON     "json"
LUALIB_API int ( luaopen_cjson)( lua_State *L );
#endif
```

After the declaration, you need to complete the registration of the Modules name and initialization function in the lua kernel in the `platform_conf.h` file.

```c
_ROM( AUXLIB_JSON, luaopen_cjson, json_map )
```

Then, the Modules function registration is carried out within the Modules initialization function function. The function registration code of this section of json library is much more complicated than the function registration code of the adc library.

```c
/* Return cjson Modules table */
static int lua_cjson_new(lua_State *l)
{
Lual_Reg Reg[] =
{ "encode", json_encode },
{ "decode", json_decode },
{ "encode_sparse_array", json_cfg_encode_sparse_array },
{ "encode_max_depth", json_cfg_encode_max_depth },
{ "decode_max_depth", json_cfg_decode_max_depth },
{ "encode_number_precision", json_cfg_encode_number_precision },
{ "encode_keep_buffer", json_cfg_encode_keep_buffer },
{ "encode_invalid_numbers", json_cfg_encode_invalid_numbers },
{ "decode_invalid_numbers", json_cfg_decode_invalid_numbers },
{ "new", lua_cjson_new },
{ NULL, NULL }
    };

/* Initialise number conversions */
    fpconv_init();

/* cjson Modules table */
    lua_newtable(l);

/* Register functions with config data as upvalue */
    json_create_config(l);
Lual_Setfuncs(l, Reg, 1);

/* Set cjson.null */
lua_pushlightuserdata(l, NULL);
lua_setfield(l, -2, "null");

/* Set Modules name / version fields */
lua_pushliteral(l, CJSON_MODNAME);
lua_setfield(l, -2, "_NAME");
lua_pushliteral(l, CJSON_VERSION);
lua_setfield(l, -2, "_VERSION");

return 1;
}
int luaopen_cjson(lua_State *l)
{
    lua_cjson_new(l);

#ifdef ENABLE_CJSON_GLOBAL
/* Register a global "cjson" table. */
lua_pushvalue(l, -1);
lua_setglobal(l, CJSON_MODNAME);
#endif

/* Return cjson table */
return 1;
}
```

### 2.4.2. Implementation of json calling function

The differences between pure soft Moduless and hardware drivers are now beginning to be reflected. Pure soft Moduless like json do not rely on the external environment, and use some standard libraries to implement them. Basically, it can be used just by taking it, and there is no need to modify it. If the json Modules is only used for elua, it is not considered for external use. Then we can directly call functions in the json library in the implementation of the above lua call. And there is no need to go alone in the abstraction layer.

Look at the implementation of the adc_open function in this adc Modules, remove some of the things that must be written, and in fact, only one line related to adc is related. The code at the beginning of this line platform is the platform adaptation code we will talk about in the next chapter. It can be regarded as a special abstraction layer of the elua open source project, responsible for interacting with the external environment. If you don’t explain it in detail here, just understand it.

```c
// adc.open(id)
static int adc_open(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    int ret;

MOD_CHECK_ID(adc, id);

ret = platform_adc_open(id,0);

lua_pushinteger (l, right);

return 1;
}
```

Let’s take a look at the implementation of the json_decode function in the json Modules. This is different, right? It’s so dizzy to see such a big lump. This is the simplest one I chose, and the others will be longer.

```c
static int json_decode(lua_State *l)
{
json_parse_t json;
json_token_t token;
    size_t json_len;

luaL_argcheck(l, lua_gettop(l) == 1, 1, "expected 1 argument");

json.cfg = json_fetch_config(l);
Json.data = lual_checklstring (L, 1, & Json_len);
json.current_depth = 0;
json.ptr = json.data;

/* Detect Unicode other than UTF-8 (see RFC 4627, Sec 3)
     *
* CJSON can support any simple data type, hence only the first
* character is guaranteed to be ASCII (at worst: '"'). This is
* still enough to detect whether the wrong encoding is in use. */
if (json_len >= 2 && (!json.data[0] || !json.data[1]))
luaL_error(l, "JSON parser does not support UTF-16 or UTF-32");

/* Ensure the temporary buffer can hold the entire string.
* This means we no longer need to do length checks since the decoded
* string must be smaller than the entire json string */
json.tmp = strbuf_new(json_len);

json_next_token(&json, &token);
json_process_value(l, &json, &token);

/* Ensure there is no more input left */
json_next_token(&json, &token);

if (token.type != T_END)
json_throw_parse_error(l, &json, "the end", &token);

    strbuf_free(json.tmp);

return 1;
}
```

Let’s take a look at you in 20 seconds to see if there is any function starting with `platform` in the above code.

![time](doc/time.jpg)

Alas, it seems that there is no one! This shows that this json Modules is specially provided for elua open source projects and does not need to be used by the outside world. There is no need to carry it outside the project and then dock it with elua.



## 2.5. Platform adaptation code

The above two sections talk about hardware-driven registration and software algorithm registration. Can software algorithms be exclusive or shared? The implementation of hardware drivers in the lua function needs to be connected to the outside through an abstract layer. If the software algorithm is exclusive, it can be called directly in the lua function implementation. If it is selected for sharing, it must also be used as the hardware driver to connect with the outside.

So have you ever thought about it↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓

### 2.5.1. Why do you need a platform to adapt code?

A successful software system often needs to run on different system platforms according to needs. In order to solve the risks brought by the transplantation of the system on multiple platforms, the business architecture often designs corresponding platform adaptation layers to isolate the differences between different platforms. How to design an easy-to-scaling platform adaptation layer is a problem that software designers need to consider.

In order to make it more convenient for everyone to port our elua open source Modules, we have written corresponding platform adaptation code, and its directory is located in elua/platform. Due to limited time, we only provide the adaptation code of the opanat platform and the adaptation code of the win32 platform in this directory. Among them, the adaptive code of the openat platform supports many functions and can be directly used in our CSSDK development environment. The functions provided are basically provided. If you have other requirements, you can also add other program Moduless yourself.


### 2.5.2. The platform adaptation code adc is connected to openat

Take the adc Modules in the hardware driver above, and its platform adaptation code is extremely simple. Look at the code block below, where the docking of `platform_adc_open` only requires one line. The docking of `platform_adc_close` is still one line.
The docking of `platform_adc_read` looks like there are several lines. If you look at the reading part carefully, there is actually only one line. However, in order to provide elua with the return value of the two results, a little bit of operation was performed.

```c
int platform_adc_open(unsigned id, unsigned mode)
{
//return PLATFORM_OK;
/*+\BUG\wangyuan\2020.06.30\lua version compiles but*/
return IVTBL(InitADC)(id, mode) ? PLATFORM_OK : PLATFORM_ERR;
/*-\BUG\wangyuan\2020.06.30\lua version compiles but*/
}

int platform_adc_close(unsigned id)
{
return IVTBL(CloseADC)(id) ? PLATFORM_OK : PLATFORM_ERR;
}

int platform_adc_read(unsigned id, int *adc, int *volt)
{
U16 adcval = 0xffff;
u16 voltage = 0xffff;
BOOL ret;

ret = IVTBL(ReadADC)(id, &adcVal, &voltage);

*adc = voltage / 3;
*volt = voltage;
return ret ? PLATFORM_OK : PLATFORM_ERR;
}
```

So far, we have sorted out the kernel part of elua, the registration part of Modules, and the docking part of platform. There are only so many things, and that's the process. It’s just that some Moduless are more complex, and some places have more codes, and it’s only a few lines that are important. Then the basic parts of our elua open source project will be everywhere. What's the use of saying so much? You can even ignore it. It's a mule or a horse that pulls it out.


# Chapter 3, Lua parser code compilation

Chapter 2 We talked about the lua kernel and Modules registration examples, so in Chapter 3, we will take it out and run away.

Some students will say that I don’t understand the second chapter, can I continue to learn in the third chapter? If it doesn’t work, then I will escape and delete the database and run away.



![Stay away](doc/gogogo.jpg)





Don’t be afraid, boy, I guarantee it with my personality. Don’t say that you didn’t understand this second chapter, even if you didn’t read it. Chapter 3 You can run, you can learn. So ,Believe in Yourself!!!



![pa](doc/pa.jpg)



## 3.1. CSDK partition related knowledge

The next content is closely related to the device. Before entering code compilation, we need to know some basic partition information. Let’s take the rda8910 platform as an example here.


### 3.1.1, Default CSDK Partition


| rda8910 (8M) | App reserved space | File system space |
| ------------- | ----------- | ------------ |
| Start address | 0x60160000 | 0x60340000 |
| Length | 0x1E0000 | 0x160000 |
| End address | 0x60340000 | 0x604a0000 |

There are two user-operable spaces at the bottom of the normal csdk. The first is the reserved space of the app, which is 1.875MB in size. Theoretically, the maximum result of the user's app compiled can be 1.875MB.
$$
0x1E0000=1966080=x*1024*1024
$$

$$
x=1.875(MB)
$$

But in fact, a little need to be reserved later. If there are fewer left, the program cannot be run. I didn’t test how much I need to reserve, so......hehehe, you know.



The second is the reserved space of the file system, with a size of 1.375MB. This space seems to be a little small, so don't expect to store any large files. In this space, you can't even save a song of mp3. But it is still good to use it to save some other small text files.
$$
0x160000=1441792=x*1024*1024
$$

$$
x=1.375(MB)
$$

### 3.1.2. CSDK—elua project partition


| rda8910 (8M) | App reserved space | elua script area | file system space |
| ------------- | ----------- | ------------ | ------------ |
| Start address | 0x60160000 | 0x602D8000 | 0x60340000 |
| Length | 0x178000 | 0x68000 | 0x160000 |
| End address | 0x602D8000 | 0x60340000 | 0x604a0000 |

Does it feel a little different? The available space for the app is a little smaller. That means you can write less.
$$
0x178000=1540096=x*1024*1024
$$

$$
x=1.46875(MB)
$$

We cut out part of the app's space and store the lua script. This space is only 416KB.
$$
0x68000=425984=x*1024
$$
$$
x=416(KB)
$$

In this way, I find that the app space seems to be a bit insufficient, and the lua script space seems to be a bit insufficient. There is nothing to do about this, the space is only so big, just a little less, save some money.

## 3.2. Customize elua script space

Some students said this is not OK, I can write it. The 400KB script space is too small and not enough for me to use. So quickly change it to me.

![I don't listen, I don't listen](doc/Don't listen.jpg)

    EMMM, it has been corrected, please check it out, bosses. Open the ?iot_sdk_4g_8910/config.cmake` file and take a look. Have you seen what you want?

```c
# app enabled
set(CONFIG_APP_ssl_SUPPORT OFF)
set(CONFIG_APP_ftp_SUPPORT OFF)
set(CONFIG_APP_http_SUPPORT OFF)
set(CONFIG_APP_littlevgl_SUPPORT OFF)
set(CONFIG_APP_mqtt_SUPPORT OFF)

if(CONFIG_BUILD_LUA)
# It's best not to move if you have nothing to do below
set(CONFIG_LUA_FLASH_OFFSET 0x2D8000)
set(CONFIG_LUA_FLASH_SIZE 0x68000)
endif()

```



The CONFIG_LUA_FLASH_OFFSET parameter sets the starting address of the lua script, and the default start address is 0x2D8000.

![papapa](doc/papapa.jpg)

  Stop calling, stop calling... 0x602D8000 is a physical address. In fact, each address has 60 in front of it. This underlying layer has been processed, so we only need to fill in the logical address.

The CONFIG_LUA_FLASH_SIZE parameter sets the lua script space size, you can modify these two values. However, the starting address plus the space size means that the address at the end of the lua script cannot exceed 0x340000. Be careful not to make mistakes. Although we have opened up the modified interface, I personally strongly recommend not modifying it casually! ! ! !



## 3.2. Start compilation

  The cmd window enters the ʿiot_sdk_4g_8910/project` directory, and then execute `app_elua.bat`. It takes a little time to get into compiled state.

![Compiled](doc/build.png)

  。。。。。。

  alright. Compilation is complete



![Compilation is completed](doc/buildOK.png)

## 3.3. Download firmware and scripts
Next, it’s the same as everyone uses the LUAT version to develop, open the luatools tool. Try to use a version higher than 2.0.68, as the lower version will cause incorrect printing of CSDK log information.

![luatools](doc/luatools.png)

Download firmware and scripts. Pay attention, choose the firmware you compiled here! ! ! ! In this place `~\iot_sdk_4g_8910\hex\Air720U_CSDK_app_elua.pac`, the script is the same as the luat development script, and there is no difference between it.

![Download script](doc/Download script.png)



This is what it looks like when the download ends.

![luatoolsover](doc/luatoolsover.png)

![Sample Program](doc/demo.gif)



Next, I can brag with others, and I can also compile the luat source code. Let it do whatever I want. Make your own Moduless!

# Chapter 4: Adding custom Moduless for eLua

To be continued. . . . .

















