#include "ps2asset/image/png.hpp"
#include "ps2asset/texture/tim2.hpp"

int main() {
    ps2asset::texture::RgbaImage image;
    image.width = 1;
    image.height = 1;
    image.pixels = {255U, 0U, 0U, 255U};
    const auto bytes = ps2asset::image::encode_png(image);
    const auto decoded = ps2asset::image::decode_png(bytes);
    return decoded.valid() && decoded.width == 1U && decoded.height == 1U &&
           decoded.pixels == image.pixels ? 0 : 1;
}
