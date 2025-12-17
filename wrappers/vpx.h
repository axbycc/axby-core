#pragma once

#include <vpx/vpx_decoder.h>
#include <vpx/vpx_encoder.h>
#include <vpx/vpx_image.h>

#include <memory>
#include <span>

inline std::span<const std::byte> to_span(const vpx_codec_cx_pkt_t* pkt) {
    return {(const std::byte*)pkt->data.frame.buf, pkt->data.frame.sz};
}

std::shared_ptr<vpx_codec_ctx> init_vpx_encoder(unsigned int profile,
                                                unsigned int width,
                                                unsigned int height,
                                                int fps,
                                                unsigned int bitrate,
                                                bool lossless);

std::shared_ptr<vpx_codec_ctx> init_vpx_decoder();

std::shared_ptr<vpx_image_t> init_vpx_img(vpx_img_fmt fmt,
                                          unsigned int width,
                                          unsigned int height,
                                          unsigned int align);
