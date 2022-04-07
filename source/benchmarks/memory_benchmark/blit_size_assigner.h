/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/error.h"

class BlitSizeAssigner {
  public:
    BlitSizeAssigner(size_t bufferSize) : bufferSize(bufferSize) {}

    void addMainCopyEngine() {
        FATAL_ERROR_IF(mainCopyEnginePresent, "Multiple main copy engines detected");
        mainCopyEnginePresent = true;
    }

    void addLinkCopyEngine() {
        linkCopyEnginesCount++;
    }

    size_t getChunksCount() const {
        if (linkCopyEnginesCount == 0) {
            return 1;
        }
        return linkCopyEnginesCount + static_cast<size_t>(mainCopyEnginePresent) * linkCopyEnginesCount;
    }

    size_t getChunkSize() const {
        return bufferSize / getChunksCount();
    }

    size_t getCopySizeForEngine(bool isMainCopyEngine) const {
        FATAL_ERROR_IF(isMainCopyEngine && !mainCopyEnginePresent, "Unexpected main copy engine");
        FATAL_ERROR_IF(!isMainCopyEngine && linkCopyEnginesCount == 0, "Unexpected link copy engine");

        size_t result = getChunkSize();
        if (isMainCopyEngine && linkCopyEnginesCount > 0) {
            result *= linkCopyEnginesCount;
        }
        return result;
    }

    std::pair<size_t, size_t> getSpaceForBlit(bool isMainCopyEngine) {
        const size_t offsetForBlit = this->offset;
        const size_t sizeForBlit = getCopySizeForEngine(isMainCopyEngine);
        this->offset += sizeForBlit;
        return {offsetForBlit, sizeForBlit};
    }

    void validate() const {
        const auto chunksCount = getChunksCount();

        DEVELOPER_WARNING_IF(offset < bufferSize, "Buffer is divided to ", chunksCount, " chunks, but bufferSize is ",
                             "not divisible by ", chunksCount, ". Only ", offset, " out of specified ", bufferSize, " bytes will be copied.");
        DEVELOPER_WARNING_IF(offset > bufferSize, "Access out of buffer bounds will happen");
    }

    const size_t bufferSize;
    size_t linkCopyEnginesCount = 0;
    bool mainCopyEnginePresent = false;
    size_t offset = 0;
};
