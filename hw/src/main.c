
/****************************************************************************/
/**
 *
 * @file		uart.c
 *
 * This file contains a design for UART
 *
 *
 *
 * MODIFICATION HISTORY:
 * <pre>
 * Ver   Who    Date     Changes
 * ----- ------ -------- ----------------------------------------------
 * 1.00  lyf 2023/07/13 First Release
 * </pre>
 ****************************************************************************/

/***************************** Include Files *******************************/
#include "sleep.h"
#include "time.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xscugic.h"
#include "xuartps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xil_cache.h>
#include <xparameters.h>

#include "axi_tangxi.h"
#include "xtime_l.h"

#include "ff.h"
#include "xstatus.h"

/************************** Constant Definitions **************************/

#define INTC XScuGic
#define UART_DEVICE_ID XPAR_XUARTPS_0_DEVICE_ID
#define INTC_DEVICE_ID XPAR_SCUGIC_SINGLE_DEVICE_ID
#define UART_INT_IRQ_ID XPAR_XUARTPS_0_INTR

#define PACKET_LENGTH 12
#define DATA_PACKET_LENGTH 528
#define PICTURE_FRAME_NUM 10
#define BIN_FRAME_NUM 10

#define BAUDRATE 115200 // 115200

// ��ʼ�����ݣ��׵�ַ4K����
// int data[1024*1024 / 4] __attribute__((aligned(4096)));

// �任����
#define WEIGHT_ADDR 0x15000000
#define WEIGHT_ADDR_PL 0x15000000
#define WEIGHT_SIZE 0x8d2e00
#define WEIGHT_BURST_SIZE 0x100000

#define QUANTIFY_ADDR 0x10010000
#define QUANTIFY_ADDR_PL 0x10010000
#define QUANTIFY_SIZE 0xC800

#define PICTURE_BASE_ADDR 0x1E000000
#define PICTURE_BASE_ADDR2 0x10000000   // PS DDRͼƬ����ַ
#define PICTURE_BASE_ADDR_PL 0x10000000 // PL DDRͼƬ����ַ
#define PICTURE_BASE_SIZE 0xfd2000      // ��ʾ���ٸ�128 bit
// #define PICTURE_BASE_SIZE 0xFF0000//��ʾ���ٸ�128 bit
#define PICTURE_BURST_SIZE 0x100000

#define TRANS_ADDR 0x20000000
#define TRANS_ADDR_PL 0x20000000
#define TRANS_SIZE0 0xfd200

#define TRANS_SIZE 0xFF0000

#define ENTROPY_ADDR 0x21000000

#define ENTROPY_ADDR_PS 0x21000000
#define ENTROPY_ADDR_PL 0x21000000
#define ENTROPY_SIZE0 0x8e62000

#define ENTROPY_SIZE 0x8F70000

#define ENTROPY_ADD0 0xfd200
#define ENTROPY_ADD 0xfefff

#define BIN_BASE_ADDR 0x32000000
#define BIN_BASE_SIZE 0x400

// ��֤
#define VERIFY 0x30000000
#define ADD 0x1000000

// SD��
#define FILE_W_NAME "1:/weight.bin"   // �����ļ���
#define FILE_Q_NAME "1:/quantify.bin" // �����ļ���
// #define FILE_P_NAME "1:/Ypad.bin"                //�����ļ���
#define FILE_P_NAME "1:/inputlle.bin"

#define FILE_T_NAME "1:/trans.bin"   // �����ļ���
#define FILE_E_NAME "1:/entropy.bin" // �����ļ���

static FATFS fatfs; // �ļ�ϵͳ

AxiTangxi_args weight_args;
AxiTangxi_args quantify_args;
AxiTangxi_args picture_args;
AxiTangxi_args trans_args;
AxiTangxi_args entropy_args;
iwave_args iargs;

/**************************** Type Definitions ******************************/

/************************** Function Prototypes *****************************/

// �任����
int iwave_initial(void);

/**************************
 *SD��
 *****************************/
// ��ʼ���ļ�ϵͳ
int platform_init_fs();
// ����SD(TF)��
int sd_mount();
// SD��������
// int sd_read_data(char *file_name,u8 *src_addr,u32 byte_len);
int sd_read_data(char *file_name, u32 src_addr, u32 byte_len);
// main����
int sd_test();
int sd_write();

/************************** Variable Definitions ***************************/

XUartPs UartPs;           /* Instance of the UART Device */
INTC InterruptController; /* Instance of the Interrupt Controller */

/*
 * The following buffers are used in this example to send and receive data
 * with the UART.
 */

/*
 * The following counters are used to determine when the entire buffer has
 * been sent and received.
 */
volatile int TotalReceivedCount = 0;
volatile int TotalSentCount;
volatile int ReceivedPictureCount = 0;
volatile int ReceivedFrameCount = 0;
int TotalErrorCount;

/**************************************************************************
 *
 * Main function to call the Uart.
 *
 * @param	None
 *
 * @return	XST_SUCCESS if successful, XST_FAILURE if unsuccessful
 *
 * @note		None
 *
 **************************************************************************/

/*********************************************
        ���ذ��ʼ����
            1.��ȡȨ�غ������������ӣ����ص�PS_DDR�У�
            2.��Ȩ�غ������������Ӱ��˵�PL_DDR�У�
 *********************************************/
int iwave_initial(void) {
  // 1
  sd_test();
  init_axi(&weight_args);
  // init_axi(&quantify_args);
  init_axi(&picture_args);
  // 2
  //	sleep(1);
  // ͼƬPS->PL

  for (int p_count = 0; p_count < 16; p_count++) {
    picture_args.tx_data_size = PICTURE_BURST_SIZE;
    picture_args.tx_data_ps_ptr =
        (uint32_t *)(0x10000000 + p_count * PICTURE_BURST_SIZE);
    picture_args.rx_data_pl_ptr =
        (uint32_t *)(0x10000000 + p_count * PICTURE_BURST_SIZE);
    batch_mm2s_ps_data_s2mm_pl(&picture_args);
    usleep(1000);
  }
  print("picture move success!");
  sleep(1);
  // ͼƬPL-��PS
  for (int p_count1 = 0; p_count1 < 16; p_count1++) {
    picture_args.rx_data_size = 0x100000;
    picture_args.rx_data_ps_ptr =
        (uint32_t *)(0x30000000 + p_count1 * 0x100000);
    picture_args.tx_data_pl_ptr =
        (uint32_t *)(0x10000000 + p_count1 * 0x100000);
    batch_s2mm_ps_data_mm2s_pl(&picture_args);
    usleep(1000);
  }
  sleep(1);
  // Ȩ��PS->PL
  for (int w_count = 0; w_count < 9; w_count++) {
    weight_args.tx_data_size = WEIGHT_BURST_SIZE;
    weight_args.tx_data_ps_ptr =
        (uint32_t *)(0x15000000 + w_count * WEIGHT_BURST_SIZE);
    weight_args.rx_data_pl_ptr =
        (uint32_t *)(0x15000000 + w_count * WEIGHT_BURST_SIZE);
    batch_mm2s_ps_data_s2mm_pl(&weight_args);
    usleep(1000);
  }
  //	sleep(1);
  print("weight move success!");
  sleep(1);
  // Ȩ��PL->PS
  //	sleep(1);
  for (int w_count = 0; w_count < 9; w_count++) {
    weight_args.rx_data_size = WEIGHT_BURST_SIZE;
    weight_args.rx_data_ps_ptr =
        (uint32_t *)(0x31000000 + w_count * WEIGHT_BURST_SIZE);
    weight_args.tx_data_pl_ptr =
        (uint32_t *)(0x15000000 + w_count * WEIGHT_BURST_SIZE);
    batch_s2mm_ps_data_mm2s_pl(&weight_args);
    usleep(1000);
  }

  sleep(1);
  iargs.weight_addr = (uint32_t)WEIGHT_ADDR_PL;
  iargs.weight_size = (uint32_t)WEIGHT_SIZE;
  iargs.quantify_addr = (uint32_t)QUANTIFY_ADDR_PL;
  iargs.quantify_size = (uint32_t)QUANTIFY_SIZE;
  iargs.picture_addr = (uint32_t)PICTURE_BASE_ADDR_PL;
  iargs.picture_size = (uint32_t)PICTURE_BASE_SIZE;

  //	while(1)
  //	{
  iwave_start(&iargs);
  print("network start!");
  uint32_t irq0 = 0;
  while (irq0 != NETWORK_ACC_DONE) {
    irq0 = get_irq();
  }
  //		sleep(40);
  clean_irq();

  print("network complete!");
  // �����ݴ�pl�ᵽps ddr
  for (int p_count2 = 0; p_count2 < 16; p_count2++) {
    picture_args.rx_data_size = 0x100000;
    picture_args.rx_data_ps_ptr =
        (uint32_t *)(0x70452000 + p_count2 * 0x100000);
    picture_args.tx_data_pl_ptr =
        (uint32_t *)(0x70452000 + p_count2 * 0x100000);
    batch_s2mm_ps_data_mm2s_pl(&picture_args);
    usleep(1000);
  }

  sleep(1);
  print("trans move ps!");
  //		sleep(1);
  //		print("entropy move ps!");
  sleep(1);
  //		sd_test();
  sd_write();
  sleep(1);
  print("Read or write sd card!");

  while (1)
    ;
  return 0;
}

/*****************************************************
  ͼƬ����״̬��
  1.���ڵȴ�����ͼƬ��
  2.���ڽ���ͼƬ��
  3.��ͼƬ���ݰ��˵�PL_DDR�У�

  ͼƬ����״̬��
  1.��ʼ���Ĵ�����
  2.�����任���磻
 ******************************************************/

int main(void) {
  // ���û��棬ȷ������ֱ��д��DDR
  Xil_DCacheDisable();

  int Status;
  // ��ʼ������
  //	Status = Uart(&InterruptController, &UartPs,
  //				  UART_DEVICE_ID, UART_INT_IRQ_ID);
  //	if (Status != XST_SUCCESS)
  //	{
  //		printf("\nUART Test Failed\n");
  //		return XST_FAILURE;
  //	}
  //	printf("\nSuccessfully ran UART Test\n");

  // �����ж���Ӧ
  Status = set_intc();
  if (Status != XST_SUCCESS) {
    printf("\nirq set Failed\n");
    return XST_FAILURE;
  }
  printf("\nSuccessfully set irq\n");

  // ��ʼ���任����
  Status = iwave_initial();
  if (Status != XST_SUCCESS) {
    printf("\niwave test failed\n");
    return XST_FAILURE;
  }
  printf("\nSuccessfully ran iwave Test\n");

  /***********************
   * running program
   ***********************/

  return XST_SUCCESS;
}

/*************************************************************
 * SD����ȡ
 *
 *
 *
 *
 *
 *
 *
 * *******************************************************************/

// ��ʼ���ļ�ϵͳ
int platform_init_fs() {
  FRESULT status;
  TCHAR *Path = "1:/";
  BYTE work[FF_MAX_SS];

  // ע��һ��������(���ط����ļ�ϵͳ)
  // ��ʹ���κ������ļ�����֮ǰ������ʹ��f_mount����Ϊÿ��ʹ�þ�ע��һ��������
  status = f_mount(&fatfs, Path, 1); // ����SD��
  if (status != FR_OK) {
    xil_printf("Volume is not FAT formatted; formatting FAT\r\n");
    // ��ʽ��SD��
    // status = f_mkfs(Path, FM_FAT32,0, work, sizeof work);
    status = f_mkfs(Path, (void *)FM_FAT32, work, sizeof work);
    if (status != FR_OK) {
      xil_printf("Unable to format FATfs\r\n");
      return -1;
    }
    // ��ʽ��֮�����¹���SD��
    status = f_mount(&fatfs, Path, 1);
    if (status != FR_OK) {
      xil_printf("Unable to mount FATfs\r\n");
      return -1;
    }
  }
  return 0;
}

// ����SD(TF)��
int sd_mount() {
  FRESULT status;
  // ��ʼ���ļ�ϵͳ������SD����������ز��ɹ������ʽ��SD����
  status = platform_init_fs();
  if (status) {
    xil_printf("ERROR: f_mount returned %d!\n", status);
    return XST_FAILURE;
  }
  return XST_SUCCESS;
}

////SD��д����
int sd_write_data(char *file_name, u32 src_addr, u32 byte_len) {
  FIL fil; // �ļ�����
  UINT bw; // f_write����������д����ֽ���

  // ��һ���ļ�,��������ڣ��򴴽�һ���ļ�
  f_open(&fil, file_name, FA_CREATE_ALWAYS | FA_WRITE);
  // �ƶ��򿪵��ļ�������ļ���/дָ��     0:ָ���ļ���ͷ
  f_lseek(&fil, 0);
  // ���ļ���д������
  f_write(&fil, (void *)src_addr, byte_len, &bw);
  // �ر��ļ�
  f_close(&fil);
  return 0;
}

int sd_read_data(char *file_name, u32 src_addr, u32 byte_len) {
  FIL fil; // �ļ�����
  UINT br; // f_read���������Ѷ������ֽ���

  // ��һ��ֻ�����ļ�
  f_open(&fil, file_name, FA_READ);
  // �ƶ��򿪵��ļ�������ļ���/дָ��     0:ָ���ļ���ͷ
  f_lseek(&fil, 0);
  // ��SD���ж�������
  f_read(&fil, (void *)src_addr, byte_len, &br);
  // �ر��ļ�
  f_close(&fil);
  return 0;
}

// main����
int sd_test() {
  int status;
  u32 len_w = WEIGHT_SIZE;
  u32 len_p = PICTURE_BASE_SIZE;
  //	u32 len_t = 10000;
  u32 ptr_w = WEIGHT_ADDR;
  u32 ptr_p = PICTURE_BASE_ADDR2;
  //	u32 ptr_t = TRANS_ADDR;

  status = sd_mount(); // ����SD��
  if (status != XST_SUCCESS) {
    xil_printf("Failed to open SD card!\n");
    return 0;
  } else
    xil_printf("Success to open SD card!\n");

  sd_read_data(FILE_W_NAME, ptr_w, len_w);
  sd_read_data(FILE_P_NAME, ptr_p, len_p);
  //	sd_write_data(FILE_T_NAME,ptr_t,len_t);
  return 0;
}

int sd_write() {
  int status;
  //	int len_t = TRANS_SIZE;
  // u32 len_t = 0x7f80000;
  u32 len_t = 0x7e9000;
  // u32 len_t = 0x3f4800;
  //	int len_e = ENTROPY_SIZE;

  u32 ptr_t = 0x70452000;
  //	u32 ptr_e = ENTROPY_ADDR;

  status = sd_mount(); // ����SD��
  if (status != XST_SUCCESS) {
    xil_printf("Failed to open SD card!\n");
    return 0;
  } else
    xil_printf("Success to open SD card!\n");

  sd_write_data(FILE_T_NAME, ptr_t, len_t);
  //	sd_write_data(FILE_T_NAME,ptr_t,len_t);
  return 0;
}
