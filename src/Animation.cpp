#include "../hdr/Animation.h"

Animation::Animation(sf::Texture* texture, sf::Vector2u imageCount, float switchTime){
	this->imageCount = imageCount;
	this->switchTime = switchTime;
	totalTime = 0.0f;
	currentImage.x = 0;

	sf::Vector2u textureSize = texture->getSize();
	uvRect.size.x = textureSize.x / float(imageCount.x);
	uvRect.size.y = textureSize.y / float(imageCount.y);
}

Animation::~Animation(){
}

void Animation::Update(int row, float deltaTime, bool faceRight){
	currentImage.y = row;
	totalTime += deltaTime;

	if (totalTime >= switchTime){
		totalTime -= switchTime;
		currentImage.x++;

		if (currentImage.x >= imageCount.x){
			currentImage.x = 0;
		}
	}
	
	uvRect.position.x = currentImage.y * uvRect.size.y;

	if (faceRight){
		uvRect.position.x = currentImage.x * uvRect.size.x;
		uvRect.size.x = abs(uvRect.size.x);
	}
	else {
		uvRect.position.x = (currentImage.x + 1) * abs(uvRect.size.x);
		uvRect.size.x = -abs(uvRect.size.x);
	}
}