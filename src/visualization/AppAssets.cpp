#include "../include/visualization/AppAssets.h"

#include <nanosvg/nanosvg.h>
#include <nanosvg/nanosvgrast.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {
    constexpr int kAppSvgResourceId = 101;

    struct SvgImageDeleter {
        void operator()(NSVGimage* image) const {
            nsvgDelete(image);
        }
    };

    struct SvgRasterizerDeleter {
        void operator()(NSVGrasterizer* rasterizer) const {
            nsvgDeleteRasterizer(rasterizer);
        }
    };

    using SvgImagePtr = std::unique_ptr<NSVGimage, SvgImageDeleter>;
    using SvgRasterizerPtr = std::unique_ptr<NSVGrasterizer, SvgRasterizerDeleter>;

#ifdef _WIN32
    std::optional<std::string> load_embedded_svg() {
        const HRSRC resource = FindResourceW(nullptr,
                                             MAKEINTRESOURCEW(kAppSvgResourceId),
                                             MAKEINTRESOURCEW(10));
        if (resource == nullptr) {
            return std::nullopt;
        }

        const DWORD resource_size = SizeofResource(nullptr, resource);
        if (resource_size == 0) {
            return std::nullopt;
        }

        const HGLOBAL resource_data = LoadResource(nullptr, resource);
        if (resource_data == nullptr) {
            return std::nullopt;
        }

        const auto* bytes = static_cast<const char*>(LockResource(resource_data));
        if (bytes == nullptr) {
            return std::nullopt;
        }

        return std::string(bytes, bytes + resource_size);
    }
#else
    std::optional<std::string> load_embedded_svg() {
        return std::nullopt;
    }
#endif
}

namespace visualization::app_assets {

    std::optional<sf::Image> load_app_icon(unsigned int size) {
        if (size == 0u) {
            return std::nullopt;
        }

        auto svg_source = load_embedded_svg();
        if (!svg_source) {
            return std::nullopt;
        }

        std::vector<char> svg_buffer(svg_source->begin(), svg_source->end());
        svg_buffer.push_back('\0');

        SvgImagePtr svg(nsvgParse(svg_buffer.data(), "px", 96.0f));
        if (!svg || svg->width <= 0.0f || svg->height <= 0.0f) {
            return std::nullopt;
        }

        SvgRasterizerPtr rasterizer(nsvgCreateRasterizer());
        if (!rasterizer) {
            return std::nullopt;
        }

        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(size) * size * 4u, 0u);
        const float scale = std::min(static_cast<float>(size) / svg->width,
                                     static_cast<float>(size) / svg->height);
        const float offset_x = (static_cast<float>(size) - svg->width * scale) * 0.5f;
        const float offset_y = (static_cast<float>(size) - svg->height * scale) * 0.5f;

        nsvgRasterize(rasterizer.get(),
                      svg.get(),
                      offset_x,
                      offset_y,
                      scale,
                      pixels.data(),
                      static_cast<int>(size),
                      static_cast<int>(size),
                      static_cast<int>(size * 4u));

        return sf::Image({size, size}, pixels.data());
    }

}
