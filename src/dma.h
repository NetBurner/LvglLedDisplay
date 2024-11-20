// dma.h
#ifndef DMA_H
#define DMA_H

#include <MIMXRT1061.h>
#include <MIMXRT1061_features.h>
#include <nbrtos.h>

const int dmaChannel = 0;
constexpr IRQn_Type s_edmaIRQNumber[][FSL_FEATURE_EDMA_MODULE_CHANNEL] = DMA_CHN_IRQS;

bool DMA_Init();
void DMA_MemoryStream(uint32_t *dest, uint32_t *src, int count);
void DMA_ISR(void);

#endif   // DMA_H