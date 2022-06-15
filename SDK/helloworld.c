#include <stdio.h>
#include "xparameters.h"
#include "platform.h"
#include "xil_printf.h"
#include "xmatrixmul.h"
#include "xaxidma.h"

#define MEM_BASE_ADDRESS 0x0A000000
#define TX_BUFFER_BASE (MEM_BASE_ADDRESS + 0X00100000)
#define RX_BUFFER_BASE (MEM_BASE_ADDRESS + 0X00300000)

XMatrixmul matrixmul;
XMatrixmul_Config *matrix_mul_config;

XAxiDma axidma;
XAxiDma_Config *axi_dma_config;

void init_peripherals()
{
	printf("Initializing Matrix Mul... \n");
	matrix_mul_config= XMatrixmul_LookupConfig(XPAR_MATRIXMUL_0_DEVICE_ID);
	if (matrix_mul_config)
	{
		int status= XMatrixmul_CfgInitialize(&matrixmul, matrix_mul_config);

		if(status!= XST_SUCCESS)
		{
			printf("ERROR IN MATRIX MUL CORE \n");
		}
	}

	printf("Initializing AXI DMA core\n");
	axi_dma_config=XAxiDma_LookupConfig(XPAR_AXIDMA_0_DEVICE_ID);
	if (axi_dma_config)
	{
		int status= XAxiDma_CfgInitialize(&axidma, axi_dma_config);

		if (status!= XST_SUCCESS)
		{
			printf("AXI DMA error \n");
		}
	}

	//XAxiDma_IntrDisable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	//XAxiDma_IntrDisable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	//XAxiDma_IntrEnable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	//XAxiDma_IntrEnable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
}


int main()
{
    init_platform();

    int instreamdata[32];
    int *m_dma_buffer_Rx= (int*) RX_BUFFER_BASE;
    print("\nHello World\n");

    init_peripherals();

    for(int idx= 0; idx<32; idx++)
    {
    	instreamdata[idx]=idx;
    }

    printf("\nInput Matrices:\n");
    //print output as a matrix
    for(int i = 0; i < 4; i++) {
      	for(int j = 0; j < 4; j++) {
       		printf("%d\t", instreamdata[i*4 + j]);
        }
        printf("\t\t\t");
      	for(int j = 0; j < 4; j++) {
      	       		printf("%d\t", instreamdata[i*4 + j + 16]);
      	        }
        printf("\n");
    }

    XMatrixmul_Set_gain(&matrixmul, 4);
    XMatrixmul_Start(&matrixmul);

   	Xil_DCacheFlushRange((u32)instreamdata, 32*sizeof(int));
   	Xil_DCacheFlushRange((u32)m_dma_buffer_Rx, 16*sizeof(int));

   	printf("Send input data... \n");
   	XAxiDma_SimpleTransfer(&axidma, (u32)instreamdata, 32*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
   	printf("Wait for Data Transfer to Matrix Mul IP.\n");
   	while(XAxiDma_Busy(&axidma, XAXIDMA_DMA_TO_DEVICE));

   	printf("Read output data .... \n");
   	XAxiDma_SimpleTransfer(&axidma, (u32)m_dma_buffer_Rx, 16*sizeof(int), XAXIDMA_DEVICE_TO_DMA);

    printf("Wait for Data Transfer to DMA.\n");
    while(XAxiDma_Busy(&axidma, XAXIDMA_DEVICE_TO_DMA) || XAxiDma_Busy(&axidma, XAXIDMA_DMA_TO_DEVICE));

    Xil_DCacheInvalidateRange((u32)m_dma_buffer_Rx, 16*sizeof(int));

    printf("Wait for Matrix Mult to complete. \n");
    //while(!XMatrixmul_IsDone(&matrixmul));

    printf("Output Matrix:\n");
    //print output as a matrix
    for(int i = 0; i < 4; i++) {
    	for(int j = 0; j < 4; j++) {
    		printf("%d\t", m_dma_buffer_Rx[i*4 + j]);
        }
        printf("\n");
    }

    cleanup_platform();
    return 0;
}
