#include <vpx/vp8cx.h>
#include <vpx/vp8dx.h>
#include <vpx/vpx_decoder.h>
#include <vpx/vpx_encoder.h>

#include <memory>

#include "debug/log.h"

void log_vpx_error(vpx_codec_ctx_t* ctx) {
    LOG(ERROR) << "vpx error " << vpx_codec_error(ctx);
}

std::shared_ptr<vpx_codec_ctx_t> init_vpx_encoder(unsigned int profile,
                                                  unsigned int width,
                                                  unsigned int height,
                                                  int fps,
                                                  unsigned int bitrate,
                                                  bool lossless) {
    vpx_codec_iface_t* iface = &vpx_codec_vp9_cx_algo;

    vpx_codec_enc_cfg_t cfg;
    const int res = vpx_codec_enc_config_default(iface, &cfg, 0);
    if (res) {
        LOG(ERROR) << "vpx config failed with " << res;
        return nullptr;
    }
    cfg.g_profile = profile;

    // Config copied from Google's udpsample project
    cfg.g_w = width;
    cfg.g_h = height;
    cfg.g_timebase.num = 1;
    cfg.g_timebase.den = fps;
    cfg.rc_end_usage = VPX_CBR;
    cfg.g_pass = VPX_RC_ONE_PASS;
    cfg.g_lag_in_frames = 0;
    cfg.rc_min_quantizer = 20;
    cfg.rc_max_quantizer = 50;
    cfg.rc_dropframe_thresh = 1;
    cfg.rc_buf_optimal_sz = 200;
    cfg.rc_buf_initial_sz = 200;
    cfg.rc_buf_sz = 200;
    cfg.g_error_resilient = 1;
    cfg.kf_mode = VPX_KF_DISABLED;
    cfg.kf_max_dist = 300;
    cfg.g_threads = 2;
    cfg.rc_resize_allowed = 0;
    cfg.rc_target_bitrate = bitrate;

    vpx_codec_flags_t flags = 0;

    auto encoder = std::shared_ptr<vpx_codec_ctx_t>(new vpx_codec_ctx_t,
                                                    [](vpx_codec_ctx* ctx) {
                                                        vpx_codec_destroy(ctx);
                                                        delete ctx;
                                                    });
    if (vpx_codec_enc_init(encoder.get(), iface, &cfg, flags)) {
        log_vpx_error(encoder.get());
        return nullptr;
    }

    // Options copied and pasted from Google's udpsample project
    vpx_codec_control(encoder.get(), VP8E_SET_CPUUSED, 8);
    vpx_codec_control(encoder.get(), VP8E_SET_STATIC_THRESHOLD, 1200);
    vpx_codec_control(encoder.get(), VP8E_SET_ENABLEAUTOALTREF, 0);
    vpx_codec_control(encoder.get(), VP9E_SET_AQ_MODE, 3);
    vpx_codec_control(encoder.get(), VP9E_SET_TILE_COLUMNS, 2);
    vpx_codec_control(encoder.get(), VP9E_SET_FRAME_PARALLEL_DECODING, 1);
    vpx_codec_control(encoder.get(), VP8E_SET_ENABLEAUTOALTREF, 0);
    vpx_codec_control(encoder.get(), VP8E_SET_GF_CBR_BOOST_PCT, 200);

    if (lossless) {
        vpx_codec_control_(encoder.get(), VP9E_SET_LOSSLESS, 1);
    }

    return encoder;
}

std::shared_ptr<vpx_codec_ctx> init_vpx_decoder() {
    vpx_codec_iface_t* iface = &vpx_codec_vp9_dx_algo;
    vpx_codec_dec_cfg_t cfg = {0};
    int dec_flags = 0;

    auto decoder = std::shared_ptr<vpx_codec_ctx_t>(new vpx_codec_ctx,
                                                    [](vpx_codec_ctx* ctx) {
                                                        vpx_codec_destroy(ctx);
                                                        delete ctx;
                                                    });
    if (vpx_codec_dec_init(decoder.get(), iface, &cfg, dec_flags)) {
        log_vpx_error(decoder.get());
        return nullptr;
    }

    return decoder;
}

std::shared_ptr<vpx_image_t> init_vpx_img(vpx_img_fmt fmt,
                                          unsigned int width,
                                          unsigned int height,
                                          unsigned int align) {
    return std::shared_ptr<vpx_image_t>(
        vpx_img_alloc(nullptr, fmt, width, height, align),
        [](vpx_image_t* img) { vpx_img_free(img); });
}
