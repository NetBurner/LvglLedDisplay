#include "dma.h"
#include <MIMXRT1061.h>
#include <MIMXRT1061_features.h>
#include <fsl_clock.h>
#include <fsl_edma.h>
#include "nbrtos.h"

void SetIntc(IsrFn func, IRQn_Type vecNum, uint32_t priority);

OS_SEM dmaSemaphore;

bool DMA_Init()
{
    USER_ENTER_CRITICAL();
    uint32_t shift = ((uint32_t)kCLOCK_Dma) & 0x1FU;

    CCM->CCGR5 = (CCM->CCGR5 & ~(3UL << shift)) | (kCLOCK_ClockNeededRunWait << shift);

    DMAMUX->CHCFG[dmaChannel] &= ~(DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_TRIG_MASK);
    DMAMUX->CHCFG[dmaChannel] |= DMAMUX_CHCFG_A_ON_MASK | DMAMUX_CHCFG_ENBL_MASK;

    DMA0->CERQ = DMA_CERQ_CERQ(dmaChannel);
    DMA0->INT = (1 << dmaChannel);
    DMA0->ERR = (1 << dmaChannel);

    uint8_t enableRoundRobinArbitration = 0x00;
    uint8_t enableHaltOnError = 0x00;
    uint8_t enableContinuousLinkMode = 0x00;
    uint8_t enableDebugMode = 0x00;

    register uint32_t regCR = DMA0->CR;
    regCR &= ~(DMA_CR_HALT_MASK | DMA_CR_ERCA_MASK | DMA_CR_HOE_MASK | DMA_CR_CLM_MASK | DMA_CR_EDBG_MASK);
    regCR |= (DMA_CR_HALT(0) | DMA_CR_ERCA(enableRoundRobinArbitration) | DMA_CR_HOE(enableHaltOnError) |
              DMA_CR_CLM(enableContinuousLinkMode) | DMA_CR_EDBG(enableDebugMode) | DMA_CR_EMLM(1U));
    DMA0->CR = regCR;

    SetIntc(DMA_ISR, s_edmaIRQNumber[0][dmaChannel], 2);

    USER_EXIT_CRITICAL();

    memset((void *)&DMA0->TCD[dmaChannel], 0, sizeof(edma_tcd_t));
    return true;
}

void DMA_MemoryStream(uint32_t *dest, uint32_t *src, int count)
{
    volatile edma_tcd_t *pTCD = (volatile edma_tcd_t *)&DMA0->TCD[dmaChannel];
    pTCD->SADDR = (uint32_t)src;
    pTCD->DADDR = (uint32_t)dest;
    pTCD->ATTR = DMA_ATTR_SSIZE(kEDMA_TransferSize4Bytes) | DMA_ATTR_DSIZE(kEDMA_TransferSize4Bytes);
    pTCD->SOFF = 4;
    pTCD->DOFF = 0;
    pTCD->NBYTES = 4;
    pTCD->CITER = count / 4;
    pTCD->BITER = count / 4;
    pTCD->CSR |= DMA_CSR_DREQ_MASK | DMA_CSR_INTMAJOR_MASK;
    DMA0->SERQ = DMA_SERQ_SERQ(dmaChannel);
    dmaSemaphore.Pend();
}

void DMA_ISR(void)
{
    DMA0->INT = (1 << dmaChannel);
    if (DMA0->TCD[dmaChannel].CSR & DMA_CSR_DONE_MASK)
    {
        DMA0->CDNE = DMA_CDNE_CDNE(dmaChannel);
    }
    dmaSemaphore.Post();
}