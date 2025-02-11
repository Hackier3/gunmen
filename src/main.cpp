#include <SFML/Window.hpp>
#include <iostream>
#include "../hdr/Player.h"

int main()
{
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "My window", sf::Style::Close);
    sf::Texture playerTexture("../../../resources/graphics/BODY_skeleton.png");

    Player player(&playerTexture, sf::Vector2u(9, 4), 0.1f, 100.0f);

    float deltaTime = 0.0f;
    sf::Clock clock;
    window.setFramerateLimit(150);

    while (window.isOpen()) {
        deltaTime = clock.restart().asSeconds();
        
        while (const std::optional event = window.pollEvent()){
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        player.Update(deltaTime);

        window.clear(sf::Color(150, 150, 150));
        player.Draw(window);
        window.display();
    }

    return 0;
}