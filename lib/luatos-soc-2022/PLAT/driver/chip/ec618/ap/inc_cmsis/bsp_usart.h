#ifndef  BSP_USART_H
#define  BSP_USART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "string.h"
#include "ec618.h"
#include "bsp.h"


// USART flags
#define USART_FLAG_INITIALIZED          (1U << 0)     // USARTinitialized
#define USART_FLAG_POWERED              (1U << 1)     // USART powered on
#define USART_FLAG_CONFIGURED           (1U << 2)     // USART configured
#define USART_FLAG_TX_ENABLED           (1U << 3)     // USART TX enabled
#define USART_FLAG_RX_ENABLED           (1U << 4)     // USART RX enabled
#define USART_FLAG_SEND_ACTIVE          (1U << 5)     // USART send active

// USART IRQ
typedef const struct _USART_IRQ {
  IRQn_Type             irq_num;         // USART IRQ Number
  IRQ_Callback_t        cb_irq;
} USART_IRQ;

// USART TX DMA
typedef struct _USART_TX_DMA {
  DmaInstance_e         instance;                               //  DMA instance
  int8_t                channel;                                //  Channel number
  uint8_t               request;                                //  DMA request number
  void                  (*callback)(uint32_t event);            //  Tx callback
} USART_TX_DMA;

// USART RX DMA
typedef struct _USART_RX_DMA {
  DmaInstance_e         instance;                               //  DMA instance
  int8_t                channel;                                //  Channel number
  uint8_t               request;                                //  DMA request number
  DmaDescriptor_t      *descriptor;                             //  Rx descriptor
  void                  (*callback)(uint32_t event);            //  Rx callback
} USART_RX_DMA;

// USART PINS
typedef const struct _USART_PIN {
  const PIN               *pin_tx;                                //  TX Pin identifier
  const PIN               *pin_rx;                                //  RX Pin identifier
  const PIN               *pin_cts;                               //  CTS Pin identifier
  const PIN               *pin_rts;                               //  RTS Pin identifier
} USART_PINS;


typedef struct _USART_TRANSFER_INFO {
  uint32_t              rx_num;         // Total number of receive data
  uint32_t              tx_num;         // Total number of transmit data
  uint8_t              *rx_buf;         // Pointer to in data buffer
  uint8_t              *tx_buf;         // Pointer to out data buffer
  uint32_t              rx_cnt;         // Number of data received
  uint32_t              tx_cnt;         // Number of data sent
  uint8_t               tx_def_val;     // Default transmit value
  uint8_t               rx_dump_val;    // Receive dump value
  uint8_t               send_active;    // Send active flag
  uint8_t               sync_mode;      // Synchronous mode flag
} USART_TRANSFER_INFO;

typedef struct _USART_STATUS {
  uint32_t rx_busy               :1;         // Receiver busy flag
  uint32_t rx_overflow           :1;         // Receive data overflow detected (cleared on start of next receive operation)
  uint32_t rx_break              :1;         // Break detected on receive (cleared on start of next receive operation)
  uint32_t rx_framing_error      :1;         // Framing error detected on receive (cleared on start of next receive operation)
  uint32_t rx_parity_error       :1;         // Parity error detected on receive (cleared on start of next receive operation)
  uint32_t reserved              :27;        //
} USART_STATUS;

typedef struct _USART_INFO {
  ARM_USART_SignalEvent_t cb_event;            // Event Callback
  USART_STATUS            rx_status;           // Recieve Status flags
  USART_TRANSFER_INFO     xfer;                // USART transfer information
  uint8_t                 flags;               // Current USART flags
  uint32_t                frame_code;          // Current USART frame setting code
  uint32_t                baudrate;            // Baudrate
} USART_INFO;

// USART Resources definition
typedef const struct {
  USART_TypeDef           *reg;                  // USART peripheral pointer
  USART_PINS               pins;                 // USART PINS config
  USART_TX_DMA            *dma_tx;               // USART DMA register interface
  USART_RX_DMA            *dma_rx;               // USART DMA register interface
  USART_IRQ               *usart_irq;            // USART IRQ
  uint8_t                 tx_fifo_trig_lvl;      // USART TX FIFO trigger level
  uint8_t                 rx_fifo_trig_lvl;      // USART RX FIFO trigger level
  uint16_t                is_unilog_mode;        // Act as unilog output
  USART_INFO              *info;                 // Run-Time Information
  uint8_t                 *hw_rxfifo_buf;        // Pointer to extra buffer for rx data after hw rxfifo is full
} USART_RESOURCES;

#ifdef __cplusplus
}
#endif

#endif /* BSP_USART_H */