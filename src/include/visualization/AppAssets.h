#ifndef VISUALIZATION_APP_ASSETS_H
#define VISUALIZATION_APP_ASSETS_H

#include <SFML/Graphics/Image.hpp>

#include <optional>

namespace visualization::app_assets {

    [[nodiscard]] std::optional<sf::Image> load_app_icon(unsigned int size = 256u);

}

#endif
