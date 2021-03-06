#ifndef darknet_h_INCLUDED
#define darknet_h_INCLUDED

#include "yolo.h"

#include <dlib/dnn.h>

class visitor_count_convolutions
{
    public:
    visitor_count_convolutions(size_t& num_convolutions) : num_convolutions(num_convolutions) {}

    template <typename other_layer_type> void operator()(size_t, other_layer_type&) const {}

    template <long nf, long nr, long nc, int sy, int sx, int py, int px>
    void operator()(size_t, dlib::con_<nf, nr, nc, sy, sx, py, px>&)
    {
        ++num_convolutions;
    }

    private:
    size_t& num_convolutions;
};

template <typename net_type> size_t count_convolutions(net_type& net)
{
    size_t num_convolutions = 0;
    dlib::visit_computational_layers(net, visitor_count_convolutions(num_convolutions));
    return num_convolutions;
}

namespace darknet
{
    // clang-format off
    using namespace dlib;

    template <template <typename> class BN, template <typename> class ACT>
    struct def
    {
        template <long num_filters, int kernel_size, int stride, int padding, typename SUBNET>
        using convolutional = ACT<BN<add_layer<
            con_<num_filters, kernel_size, kernel_size, stride, stride, padding, padding>,
            SUBNET>>>;

        template <long num_filters, typename SUBNET>
        using residual = add_prev1<
            convolutional<num_filters, 3, 1, 1,
            convolutional<num_filters / 2, 1, 1, 0,
            tag1<SUBNET>>>>;

        template <long num_filters, typename SUBNET>
        using block3 = 
            convolutional<num_filters, 3, 1, 1,
            convolutional<num_filters / 2, 1, 1, 0,
            convolutional<num_filters, 3, 1, 1,
            SUBNET>>>;

        template <long num_filters, typename SUBNET>
        using block5 = 
            convolutional<num_filters, 3, 1, 1,
            convolutional<num_filters / 2, 1, 1, 0,
            convolutional<num_filters, 3, 1, 1,
            convolutional<num_filters / 2, 1, 1, 0,
            convolutional<num_filters, 3, 1, 1,
            SUBNET>>>>>;

        template <typename SUBNET> using residual_128 = residual<128, SUBNET>;
        template <typename SUBNET> using residual_256 = residual<256, SUBNET>;
        template <typename SUBNET> using residual_512 = residual<512, SUBNET>;
        template <typename SUBNET> using residual_1024 = residual<1024, SUBNET>;

        template <typename INPUT>
        using backbone19 = 
            block5<1024,
            max_pool<2, 2, 2, 2,
            block5<512,
            max_pool<2, 2, 2, 2,
            block3<256,
            max_pool<2, 2, 2, 2,
            block3<128,
            max_pool<2, 2, 2, 2,
            convolutional<64, 3, 1, 1,
            max_pool<2, 2, 2, 2,
            convolutional<32, 3, 1, 1,
            INPUT>>>>>>>>>>>;

        template <typename INPUT>
        using backbone53 =
            repeat<4, residual_1024,
            convolutional<1024, 3, 2, 1,
            repeat<8, residual_512,
            convolutional<512, 3, 2, 1,
            repeat<8, residual_256,
            convolutional<256, 3, 2, 1,
            repeat<2, residual_128,
            convolutional<128, 3, 2, 1,
            residual<64,
            convolutional<64, 3, 2, 1,
            convolutional<32, 3, 1, 1,
            INPUT>>>>>>>>>>>;

        template <typename SUBNET>
        using classification_head = loss_multiclass_log<fc<1000, avg_pool_everything<SUBNET>>>;

        template <typename SUBNET>
        using detection_head = loss_yolo<con<1, 1, 1, 1, 1, SUBNET>>;

        using classifier19_type = classification_head<backbone19<input_rgb_image>>;
        using classifier53_type = classification_head<backbone53<input_rgb_image>>;
        using detector19_type = detection_head<backbone19<input_rgb_image>>;
        using detector53_type = detection_head<backbone53<input_rgb_image>>;
    };

    using classifier19_train = def<bn_con, leaky_relu>::classifier19_type;
    using classifier19_infer = def<affine, leaky_relu>::classifier19_type;
    using detector19_train = def<bn_con, leaky_relu>::detector19_type;
    using detector19_infer = def<affine, leaky_relu>::detector19_type;

    using classifier53_train = def<bn_con, mish>::classifier53_type;
    using classifier53_infer = def<affine, mish>::classifier53_type;
    using detector53_train = def<bn_con, mish>::detector53_type;
    using detector53_infer = def<affine, mish>::detector53_type;
    // clang-format on

}  // namespace darknet

#endif  // darknet_h_INCLUDED
