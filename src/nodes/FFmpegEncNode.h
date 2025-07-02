#ifndef IMG_DEINT_FFMPEG_ENC_NODE_H
#define IMG_DEINT_FFMPEG_ENC_NODE_H


#include "base/Encoder.h"

namespace img_deinterlace {

    class FFmpegEncNode : public Encoder {
    public:
        FFmpegEncNode() { 

        }
        ~FFmpegEncNode() { 

        }
    private:
        virtual void init() override {

        };
        virtual void writePacket(std::unique_ptr<PipelinePacket> packet) override {
            
        };
    };
}


#endif //!IMG_DEINT_FFMPEG_ENC_NODE_H