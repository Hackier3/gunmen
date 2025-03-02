﻿#include <tmxlite/TileLayer.hpp>
#include <tmxlite/Map.hpp>
#include <SFML/Graphics.hpp>
#include <pugixml.hpp>
#include <iostream>
#include <sstream> 
#include <string>  
#include <vector>
#include <iomanip>
#include <filesystem>

#include "../hdr/Decoder.h"
#include "../hdr/Map.h"

std::map<uint32_t, Map::Tile> Map::tilesInfo;

Map::Map(std::string mapPath)
{
    tmx::Map map;
    if (!map.load(mapPath) || !doc.load_file(mapPath.c_str())) {
        throw std::runtime_error("Failed to load the TMX map from path: " + mapPath);
    }

    mapWidth = map.getTileCount().x;
    mapHeight = map.getTileCount().y;
    tileSize = map.getTileSize().x;
    if (tilesInfo.empty()) {
        setTilesInfo();
    }

    processTileLayers();
    createLayersSprite();
}

void Map::setTilesInfo() {
	setTilesPath();
    setTilesAnimation();
	setTilesHitboxes();
}

void Map::setTilesPath() {
    // Przechodzimy przez wszystkie <tileset>
    for (pugi::xml_node tileset : doc.child("map").children("tileset")) {
        int firstGID = tileset.attribute("firstgid").as_int();
        int tileCount = tileset.attribute("tilecount").as_int();
        std::string tilesetName = tileset.attribute("name").as_string();

        // Sprawdzamy, czy tileset ma bezpośrednio <image> zamiast <tile>
        // Wersja dla pól z tilesetu
        pugi::xml_node image = tileset.child("image");
        if (image) { 
            std::string imagePath = image.attribute("source").as_string();
			imagePath.replace(0, 2, "resources");

            int width = image.attribute("width").as_int();
            int height = image.attribute("height").as_int();

            // Dla każdego GID od firstGID do firstGID + tileCount - 1
            for (int i = 0; i < tileCount; ++i) {
                int tileID = i; // ID kafelka w obrębie tilesetu
                int GID = firstGID + tileID; // Globalny ID kafelka

                // Tworzymy nowy obiekt Tile i zapisujemy go w tilesInfo
                Tile newTile;
                newTile.GID = GID;
                newTile.firstGID = firstGID;
                newTile.ID = tileID;
                newTile.path = imagePath;
                newTile.isFromTileSet = true;
                tilesInfo[newTile.GID] = newTile;
            }
        }
        // Wersja dla pól nie z tilesetu
        else { 
            for (pugi::xml_node tile : tileset.children("tile")) {
                int tileID = tile.attribute("id").as_int();
                pugi::xml_node tileImage = tile.child("image");

                if (tileImage) {
                    std::string imagePath = tileImage.attribute("source").as_string();
                    imagePath.replace(0, 2, "resources");

                    int width = tileImage.attribute("width").as_int();
                    int height = tileImage.attribute("height").as_int();

                    // Tworzymy nowy obiekt Tile i zapisujemy go w tilesInfo
                    Tile newTile;
                    newTile.GID = firstGID + tileID;
                    newTile.firstGID = firstGID;
                    newTile.ID = tileID;
                    newTile.path = imagePath;
                    newTile.isFromTileSet = false;
                    tilesInfo[newTile.GID] = newTile;
                }
            }
        }
    }
}

void Map::setTilesAnimation() {
    // Iteracja po wszystkich tilesetach w dokumencie XML
    for (pugi::xml_node tileset : doc.child("map").children("tileset")) {
        int firstGID = tileset.attribute("firstgid").as_int();

        // Iteracja po wszystkich tile w danym tilesecie
        for (pugi::xml_node tile : tileset.children("tile")) {
            int tileID = tile.attribute("id").as_int() + firstGID;
            pugi::xml_node animationNode = tile.child("animation");

            // Sprawdzenie, czy tile ma animację
            if (animationNode) {
                AnimatedData animationFrames;

                // Iteracja po wszystkich klatkach animacji
                for (pugi::xml_node frame : animationNode.children("frame")) {
                    int frameID = frame.attribute("tileid").as_int() + firstGID;
                    int duration = frame.attribute("duration").as_int();
                    animationFrames.push_back({ frameID, duration }); // Dodajemy klatkę do wektora
                }

                // Szukanie tileID w tilesInfo
                auto it = tilesInfo.find(tileID);
                if (it != tilesInfo.end()) {
                    // Ustawienie właściwości animacji
                    it->second.properties = TileProperty(animationFrames);
                }
            }
        }
    }
}

void Map::setTilesHitboxes() {
    // Iteracja po wszystkich tilesetach w dokumencie XML
    for (pugi::xml_node tileset : doc.child("map").children("tileset")) {
        int firstGID = tileset.attribute("firstgid").as_int();

        // Iteracja po wszystkich tile w danym tilesecie
        for (pugi::xml_node tile : tileset.children("tile")) {
            int tileID = tile.attribute("id").as_int() + firstGID;
            pugi::xml_node objectgroup = tile.child("objectgroup");

            // Sprawdzenie, czy tile ma objectgroup (hitboxy)
            if (objectgroup) {
                std::vector<Hitbox> hitboxes;

                // Iteracja po wszystkich obiektach (hitboxach) w objectgroup
                for (pugi::xml_node object : objectgroup.children("object")) {
                    int x = object.attribute("x").as_int();
                    int y = object.attribute("y").as_int();

                    // Sprawdzenie typu hitboxa
                    if (object.child("ellipse")) {
                        int radius = object.attribute("width").as_int() / 2; // Zakładamy, że szerokość i wysokość są równe
                        hitboxes.emplace_back(x, y, radius);
                    }
                    else if (object.child("polygon")) {
                        pugi::xml_node polygon = object.child("polygon");
                        std::string pointsStr = polygon.attribute("points").as_string();

                        std::vector<std::pair<int, int>> points;
                        std::istringstream iss(pointsStr);
                        std::string point;
                        while (std::getline(iss, point, ' ')) {
                            size_t commaPos = point.find(',');
                            int px = std::stoi(point.substr(0, commaPos));
                            int py = std::stoi(point.substr(commaPos + 1));
                            points.emplace_back(px, py);
                        }
                        hitboxes.emplace_back(x, y, points);
                    }
                    else if (object.child("point")) {
                        hitboxes.emplace_back(x, y);
                    }
                    else {
                        // Prostokąt (domyślny typ)
                        int width = object.attribute("width").as_int();
                        int height = object.attribute("height").as_int();
                        hitboxes.emplace_back(x, y, width, height);
                    }
                }

                auto it = tilesInfo.find(tileID);
                if (it != tilesInfo.end()) {
                    it->second.properties = TileProperty(hitboxes);
                }
            }
        }
    }
}

void Map::printInfo() {
    for (const auto& entry : tilesInfo) {
        std::cout << "Tile ID: " << entry.first << "\n";
        std::cout << "Path: " << entry.second.path << "\n";
        std::cout << "Is From TileSet: " << (entry.second.isFromTileSet ? "Yes" : "No") << "\n";

            // Sprawdzenie, czy tile ma hitboxy
            const auto* hitboxes = std::get_if<std::vector<Hitbox>>(&entry.second.properties);
            if (hitboxes && !hitboxes->empty()) {
            std::cout << "Hitboxes: \n";
            for (const auto& hitbox : *hitboxes) {
                std::cout << "  Hitbox Type: ";
                switch (hitbox.type) {
                case Hitbox::Type::Rectangle:
                    std::cout << "Rectangle, x=" << hitbox.x << ", y=" << hitbox.y << ", ";
                    if (std::holds_alternative<std::pair<int, int>>(hitbox.data)) {
                        const auto& rect = std::get<std::pair<int, int>>(hitbox.data);
                        std::cout << "width=" << rect.first << ", height=" << rect.second;
                    }
                    std::cout << "\n";
                    break;
                case Hitbox::Type::Circle:
                    std::cout << "Circle, x=" << hitbox.x << ", y=" << hitbox.y << ", ";
                    if (std::holds_alternative<int>(hitbox.data)) {
                        std::cout << "radius=" << std::get<int>(hitbox.data);
                    }
                    std::cout << "\n";
                    break;
                case Hitbox::Type::Polygon:
                    std::cout << "Polygon, x=" << hitbox.x << ", y=" << hitbox.y << ", ";
                    if (std::holds_alternative<std::vector<std::pair<int, int>>>(hitbox.data)) {
                        const auto& points = std::get<std::vector<std::pair<int, int>>>(hitbox.data);
                        std::cout << "points: ";
                        for (const auto& point : points) {
                            std::cout << "(" << point.first << "," << point.second << ") ";
                        }
                    }
                    std::cout << "\n";
                    break;
                case Hitbox::Type::Point:
                    std::cout << "Point, x=" << hitbox.x << ", y=" << hitbox.y << "\n";
                    break;
                }
            }
        }

        // Sprawdzenie, czy tile ma animację
        const auto* animationData = std::get_if<AnimatedData>(&entry.second.properties);
        if (animationData && !animationData->empty()) {
            std::cout << "Animation Frames: \n";
            for (const auto& frame : *animationData) {
                std::cout << "  Frame ID: " << frame.first << ", Duration: " << frame.second << " ms\n";
            }
        }

        std::cout << "-----------------------------------------------------\n";
    }
}

void Map::processTileLayers() {
    // Iteracja po wszystkich warstwach w dokumencie XML
    for (pugi::xml_node layerNode : doc.child("map").children("layer")) {
        auto layer = std::make_unique<Layer>();
        layer->id = layerNode.attribute("id").as_int();
        layer->name = layerNode.attribute("name").as_string();
        layer->width = layerNode.attribute("width").as_int();
        layer->height = layerNode.attribute("height").as_int();

        // Przejście przez dane warstwy
        pugi::xml_node dataNode = layerNode.child("data");
        if (dataNode) {
            std::string encoding = dataNode.attribute("encoding").as_string();
            std::string compression = dataNode.attribute("compression").as_string();

            if (encoding == "base64" && compression == "zlib") {
                // Dekodowanie base64
                std::string base64Data = dataNode.text().as_string();
                std::string compressedData = Decoder::base64_decode(base64Data);

                // Dekompresja zlib
                std::vector<uint8_t> decompressedData;
                Decoder::decompressZlib(compressedData, decompressedData);

                // Przetwarzanie zdekompresowanych danych
                size_t tileCount = layer->width * layer->height;
                for (size_t i = 0; i < tileCount; ++i) {
                    // Każdy kafelek jest reprezentowany przez 4 bajty (uint32_t)
                    uint32_t gid = *reinterpret_cast<uint32_t*>(&decompressedData[i * 4]);

                    // Odkodowanie gid
                    uint32_t tileID;
                    bool flipHorizontal, flipVertical, flipDiagonal;
                    Decoder::decodeGID(gid, tileID, flipHorizontal, flipVertical, flipDiagonal);

                    if (tileID != 0) { // Pomijamy puste kafelki
                        // Znajdź kafelek w tilesInfo
                        auto it = tilesInfo.find(tileID);
                        if (it != tilesInfo.end()) {
                            // Utwórz TileInfo
                            TileInfo tileInfo;
                            tileInfo.tile = it->second;
                            tileInfo.x = i % layer->width;
                            tileInfo.y = i / layer->width;
                            tileInfo.flipHorizontal = flipHorizontal;
                            tileInfo.flipVertical = flipVertical;
                            tileInfo.flipDiagonal = flipDiagonal;

                            // Dodaj TileInfo do warstwy
                            layer->tiles.push_back(tileInfo);
                        }
                    }
                }
            }
            else {
                std::cerr << "Nieobsługiwany format danych warstwy: encoding=" << encoding
                    << ", compression=" << compression << "\n";
            }
        }
        else {
            std::cerr << "Błąd: Brak węzła <data> dla warstwy: " << layer->name << "\n";
        }

        layers.push_back(std::move(layer)); // Przenieś warstwę do wektora
    }
}

void Map::createLayersSprite() {
    for (auto& layer : this->layers) {
        if (!layer->canvasTexture.create(mapWidth * tileSize, mapHeight * tileSize)) {
            throw std::runtime_error("Failed to create canvas texture");
        }

        layer->canvasTexture.clear(sf::Color::Transparent);
        sf::Texture texture;
        if (!texture.loadFromFile(tilesInfo[1].path)) {
            throw std::runtime_error("Cannot load file");
        };
        sf::Sprite sprite(texture);
        layer->canvasTexture.draw(sprite);
        layer->canvasTexture.display();
        layer->sprite.setTexture(layer->canvasTexture.getTexture());
    }
}

void Map::drawMap(sf::RenderWindow& window) {
    for (auto& layer : this->layers) {
        window.draw(layer->sprite);
    }
}

Map::~Map() {
}