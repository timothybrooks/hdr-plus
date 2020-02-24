#include <Halide.h>

#include "align.h"
#include "merge.h"
#include "finish.h"

namespace {

    class HdrPlusPipeline : public Halide::Generator<HdrPlusPipeline> {
    public:
        // 'inputs' is really a series of raw 2d frames; extent[2] specifies the count
        Input<Halide::Buffer<uint16_t>> inputs{"inputs", 3};
        Input<uint16_t> black_point{"black_point"};
        Input<uint16_t> white_point{"white_point"};
        Input<float> white_balance_r{"white_balance_r"};
        Input<float> white_balance_g0{"white_balance_g0"};
        Input<float> white_balance_g1{"white_balance_g1"};
        Input<float> white_balance_b{"white_balance_b"};
        Input<float> compression{"compression"};
        Input<float> gain{"gain"};

        // RGB output
        Output<Halide::Buffer<uint8_t>> output{"output", 3};

        void generate() {
            // Algorithm
            Func alignment = align(inputs, inputs.width(), inputs.height());
            Func merged = merge(inputs, inputs.width(), inputs.height(), inputs.dim(2).extent(), alignment);
            CompiletimeWhiteBalance wb{ white_balance_r, white_balance_g0, white_balance_g1, white_balance_b };
            Func finished = finish(merged, inputs.width(), inputs.height(), black_point, white_point, wb, compression, gain);
            output = finished;
            // Schedule handled inside included functions
        }
    };

}  // namespace

HALIDE_REGISTER_GENERATOR(HdrPlusPipeline, hdrplus_pipeline)
