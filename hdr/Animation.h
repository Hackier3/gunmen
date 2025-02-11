#pragma once
#include <SFML/Graphics.hpp>

class Animation{
public:
	Animation(sf::Texture* texture, sf::Vector2u imageCount, float switchTime);
	~Animation();

	void Update(int row, float deltaTime, bool faceRight, bool faceUp);
	void setCurrentImageColumn(unsigned int xSetter);
public:
	sf::IntRect uvRect; // wyciety prostokat tekstury

private:
	sf::Vector2u imageCount; // przechowuje liczbe klatek w pionie i poziome dla uvRect
	sf::Vector2u currentImage; // aktualna klatka w jednostce z imageCount

	float totalTime;
	float switchTime;
};