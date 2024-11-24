* # nr_micro_shell

## Introduction

When debugging and maintaining, it is often necessary to interact with the microcontroller to obtain and set certain parameters or perform certain operations. **nr_micro_shell** is a basic command line written for MCUs with fewer resources to meet this need. tool. Although the RT_Thread component has provided a powerful **finsh** command line interactive tool, **finsh** is still slightly bulky for microcontrollers with less ROM and RAM resources. On these platforms, if you still want to keep For basic command line interaction functions, **nr_micro_shell** is a good choice.

**nr_micro_shell** has the following advantages

1. It takes up less resources, is simple to use, flexible and convenient. The usage process only involves two functions, shell_init() and shell(). This tool can be easily applied whether using RTOS or bare metal without additional coding work.

2. Good interactive experience. Completely similar to the Linux shell command line, when the serial port terminal supports ANSI (such as Hypertrm terminal), it not only supports basic command line interaction, but also provides Tab key command completion, query history commands, and arrow keys to move the cursor and modify it.

3. Good scalability. **nr_micro_shell** provides users with standard function prototypes for custom commands. You only need to write the command function according to the command and register the command function to use the command.

## Transplantation:

Reference AIR101

## Configuration:

All configuration work can be done in **_nr_micro_shell_config.h_**. See the comments in the file for details.

## Usage:

- Make sure all files have been added to the project.

- Ensure that the macro functions "shell_printf(), ansi_show_char()" in **_nr_micro_shell_config.h_** can be used normally in the project.

- Usage examples are as follows

  ```c
  #include "nr_micro_shell.h"
  
  int main(void)
  {
/* initialization */
      shell_init();
  
      while(1)
      {
if(USART GET A CHAR 'c')
          {
/* nr_micro_shell receives characters */
              shell(c);
          }
      }
  }
  ```

Before using hardware input directly, it is recommended to use the following code (to ensure that information can be printed normally) to verify whether nr_micro_shell can run normally.

  ```c
  #include "nr_micro_shell.h"
  
  int main(void)
  {
      unsigned int i = 0;
//Match the end character configuration NR_SHELL_END_OF_LINE 0
      char test_line[] = "test 1 2 3\n"
/* initialization */
      shell_init();
      
/* Preliminary test code */
for(i = 0; i < sizeof(test_line)-1; i++)
      {
          shell(test_line[i]);
      }
  
/* Official working code */
      while(1)
      {
if(USART GET A CHAR 'c')
          {
/* nr_micro_shell receives characters */
              shell(c);
          }
      }
  }
  ```

## Add your own commands

  **STEP1**:

You need to implement a command function in **nr_micro_shell_commands.c***. The prototype of the command function is as follows

  ```c
  void your_command_funtion(char argc, char *argv)
  {
      .....
  }
  ```

**argc** is the number of arguments. **argv** stores the starting address and content of each parameter. If the input string is

  ```c
test -a 1
  ```

Then **argc** is 3, and the content of **argv** is

  ```c
  -------------------------------------------------------------
  0x03|0x08|0x0b|'t'|'e'|'s'|'t'|'\0'|'-'|'a'|'\0'|'1'|'\0'|
  -------------------------------------------------------------
  ```

If you want to know the contents of the first or second parameter, you should use

  ```c
/* "-a" */
  printf(argv[argv[1]])
/* "1" */
  printf(argv[argv[2]])
  ```

  **STEP2**:
You need to register the command before using it. There are two ways to register the command.

1. When NR_SHELL_USING_EXPORT_CMD is not defined in the configuration file, write it in the **static_cmd[]** table

  ```c
  const static_cmd_st static_cmd[] =
  {
     .....
     {"your_command_name",your_command_funtion},
     .....
     {"\0",NULL}
  };
  ```

**_Note: Do not delete {"\0", NULL}! _**

2. When NR_SHELL_USING_EXPORT_CMD is defined in the configuration file and NR_SHELL_CMD_EXPORT() supports the compiler used, you can use the following method to register the command

  ```c
  NR_SHELL_CMD_EXPORT(your_command_name,your_command_funtion);
  ```

## Notes

Use NR_SHELL_USING_EXPORT_CMD to select the command registration method according to your usage habits.

When using the registry registration command, make sure the registry exists in your project

  ```c
  const static_cmd_st static_cmd[] ={   .....   {"\0",NULL}};
  ```

When using NR_SHELL_CMD_EXPORT(), make sure that the compiler used is supported by NR_SHELL_CMD_EXPORT(), otherwise an error will be reported.

nr_micro_shell does not support control keys (control characters) such as the ESC key.

## Original address connection

- Home page: <https://gitee.com/nrush/nr_micro_shell>
