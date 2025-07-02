#ifndef IMG_DEINT_FFMPEG_DEC_NODE_H
#define IMG_DEINT_FFMPEG_DEC_NODE_H


#include "base/Decoder.h"

namespace img_deinterlace {

    class FFmpegDecNode : public Decoder {
    public:
        FFmpegDecNode() { }
        ~FFmpegDecNode() { }
        
    private:
        virtual void init() override {

        };
        virtual std::unique_ptr<PipelinePacket> getPacket() override {
            return nullptr;
        };
    };
}


#endif //!IMG_DEINT_FFMPEG_DEC_NODE_H