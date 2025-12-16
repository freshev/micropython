#ifndef AT_PROCESS_H
#define AT_PROCESS_H

#include "string.h"
#include "am_openat_vat.h"


//ATÃüÁîÁ÷³Ì
//1, Ö÷¶¯·¢ËÍATÖ¸Áî
// 2, ¶eè¡atö¸eÎte¹û
//3, ¼ÙÉèATÖ¸ÁîÄ¬ÈÏ¶¼ÊÇÒÔOK»òÕßERROR½eÎ²
//read at result thread
#define AT_SUCCESS       0

#define AT_ERROR_GENERIC -1
#define AT_ERROR_COMMAND_PENDING -2
#define AT_ERROR_CHANNEL_CLOSED -3
#define AT_ERROR_TIMEOUT -4
#define AT_ERROR_INVALID_THREAD -5 /* AT commands may not be issued from
                                       reader thread (or unsolicited response
                                       callback */
#define AT_ERROR_INVALID_RESPONSE -6 /* eg an at_send_command_singleline that
                                        did not get back an intermediate
                                        response */

/** a singly-lined list of intermediate responses */
typedef struct ATLine  {
    struct ATLine *p_next;
    char *line;
} ATLine;

/** Free this with at_response_free() */
typedef struct {
    int success;              /* true if final response indicates
                                    success (eg "OK") */
    char *finalResponse;      /* eg OK, ERROR */
    ATLine  *p_intermediates; /* any intermediate responses */
} ATResponse;

typedef struct RILChannelDataTag
{
	char 				*data;
    int 				len;
}RILChannelData;




typedef enum {
    NO_RESULT,   /* no intermediate response expected */
    NUMERIC,     /* a single intermediate response starting with a 0-9 */
    SINGLELINE,  /* a single intermediate response starting with a prefix */
    MULTILINE    /* multiple line intermediate response
                    starting with a prefix */
} ATCommandType;

#ifndef IVTBL
#define IVTBL(func) (g_s_InterfaceVtbl->func)
#endif
#ifndef ASSERT
#define ASSERT(condition) IVTBL(assert)(condition, (CHAR*)__FUNCTION__, __LINE__)
#endif

typedef void (*ATUnsolHandler)(const char *s, const char *sms_pdu);
typedef void (*ATUnsolSMSHandler)(const int sms_index);


/*********************************************************
  Function:    ril_set_cb
  Description: ×¢²eiot_vat_init»Øµ÷º¯Êý
  Input:
        pAtMessage:iot_vat_init»Øµ÷º¯Êý

*********************************************************/
void ril_set_cb(PAT_MESSAGE pAtMessage);

/*********************************************************
  Function:    at_init
  Description: ³õÊ¼»¯
  Input:

  Output:
  Return:      
  Others:  1£¬³õÊ¼»¯¶ÁÏß³Ì
           2£¬³õÊ¼»¯µÈ´ýÐÅºÅÁ¿
           3£¬³õÊ¼»¯ÏµÍ³±äÁ¿
*********************************************************/

void at_init(void);
/*********************************************************
  Function:    at_regNetStatusCb
  Description: ³õÊ¼»¯
  Input:

  Output:
  Return:  
*********************************************************/

void at_regNetStatusCb(void (*netStatusCb)(int));


/*********************************************************
  Function:    at_send_command_singleline
  Description: ·¢ËÍATÖ¸Áî£¬²¢µÈ´ý½e¹û
  Input:

  Output:
  Return:       AT_ERROR_xx/AT_SUCCESS
  Others:
*********************************************************/

int at_send_command_singleline (const char *command,
                                const char *responsePrefix,
                                 ATResponse **pp_outResponse);
/*********************************************************
  Function:    at_send_command_numeric
  Description: ·¢ËÍATÖ¸Áî£¬²¢µÈ´ý½e¹û
  Input:

  Output:
  Return:       AT_ERROR_xx/AT_SUCCESS
  Others:
*********************************************************/

int at_send_command_numeric (const char *command,
                                 ATResponse **pp_outResponse);
                                 
/*********************************************************
  Function:    at_send_command_multiline
  Description: ·¢ËÍATÖ¸Áî£¬²¢µÈ´ý½e¹û
  Input:

  Output:
  Return:       AT_ERROR_xx/AT_SUCCESS
  Others:
*********************************************************/

int at_send_command_multiline (const char *command,
                                const char *responsePrefix,
                                 ATResponse **pp_outResponse);
                                 
/*********************************************************
  Function:    at_send_command_sms
  Description: ·¢ËÍATÖ¸Áî£¬²¢µÈ´ý½e¹û
  Input:

  Output:
  Return:       AT_ERROR_xx/AT_SUCCESS
  Others:
*********************************************************/
int at_send_command_sms (const char *command,
                                const char *pdu,
                                const char *responsePrefix,
                                 ATResponse **pp_outResponse);

/*********************************************************
  Function:    at_send_command
  Description: ·¢ËÍATÖ¸Áî£¬²»µÈ´ý½e¹û
  Input:

  Output:
  Return:       AT_ERROR_xx/AT_SUCCESS
  Others:
*********************************************************/

int at_send_command (const char *command, ATResponse **pp_outResponse);

/*********************************************************
  Function:    at_response_free
  Description: ÊÍ·Å½e¹û
  Input:

  Output:
  Return:      
  Others:
*********************************************************/
void at_response_free(ATResponse *p_response);


VOID at_message(UINT8 *pData, UINT16 length);

void at_regSmsHanlerCb(ATUnsolSMSHandler cb);

#endif



