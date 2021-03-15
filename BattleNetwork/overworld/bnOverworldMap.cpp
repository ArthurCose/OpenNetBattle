#include <algorithm>
#include <cmath>

#include "bnOverworldMap.h"
#include "bnOverworldSceneBase.h"


namespace Overworld {
  Map::Map(unsigned cols, unsigned rows, int tileWidth, int tileHeight) {
    this->tileWidth = tileWidth;
    this->tileHeight = tileHeight;
    this->cols = cols;
    this->rows = rows;
    this->tileToTilesetMap.push_back(nullptr);
  }

  void Map::Update(SceneBase& scene, double elapsed) {
    for (auto& tileMeta : tileMetas) {
      if (tileMeta != nullptr) {
        tileMeta->animation.Update(elapsed, tileMeta->sprite);
      }
    }

    for (int i = 0; i < layers.size(); i++) {
      auto& layer = layers[i];

      for (auto& tileObject : layer.tileObjects) {
        tileObject.Update(*this);
      }

      for (auto& worldSprite : layer.spritesForAddition) {
        scene.AddSprite(worldSprite);
      }

      layer.spritesForAddition.clear();
    }
  }

  const sf::Vector2f Map::ScreenToWorld(sf::Vector2f screen) const
  {
    auto scale = getScale();
    return OrthoToIsometric(sf::Vector2f{ screen.x / scale.x, screen.y / scale.y });
  }

  const sf::Vector2f Map::WorldToScreen(sf::Vector2f world) const
  {
    return IsoToOrthogonal(world);
  }

  const sf::Vector2f Map::OrthoToIsometric(const sf::Vector2f& ortho) const {
    sf::Vector2f iso{};
    iso.x = (2.0f * ortho.y + ortho.x) * 0.5f;
    iso.y = (2.0f * ortho.y - ortho.x) * 0.5f;

    return iso;
  }

  const sf::Vector2f Map::IsoToOrthogonal(const sf::Vector2f& iso) const {
    sf::Vector2f ortho{};
    ortho.x = (iso.x - iso.y);
    ortho.y = (iso.x + iso.y) * 0.5f;

    return ortho;
  }

  const sf::Vector2i Map::GetTileSize() const {
    return sf::Vector2i(tileWidth, tileHeight);
  }

  const std::string& Map::GetName() const
  {
    return name;
  }

  const std::string& Map::GetBackgroundName() const
  {
    return backgroundName;
  }

  const std::string& Map::GetBackgroundCustomTexturePath() const
  {
    return backgroundCustomTexturePath;
  }

  const std::string& Map::GetBackgroundCustomAnimationPath() const
  {
    return backgroundCustomAnimationPath;
  }

  sf::Vector2f Map::GetBackgroundCustomVelocity() const
  {
    return backgroundCustomVelocity;
  }

  const std::string& Map::GetSongPath() const
  {
    return songPath;
  }

  void Map::SetName(const std::string& name)
  {
    this->name = name;
  }

  void Map::SetBackgroundName(const std::string& name)
  {
    backgroundName = name;
  }

  void Map::SetBackgroundCustomTexturePath(const std::string& path) {
    backgroundCustomTexturePath = path;
  }

  void Map::SetBackgroundCustomAnimationPath(const std::string& path) {
    backgroundCustomAnimationPath = path;
  }

  void Map::SetBackgroundCustomVelocity(float x, float y) {
    backgroundCustomVelocity.x = x;
    backgroundCustomVelocity.y = y;
  }

  void Map::SetBackgroundCustomVelocity(sf::Vector2f velocity) {
    backgroundCustomVelocity = velocity;
  }

  void Map::SetSongPath(const std::string& path)
  {
    songPath = path;
  }

  const unsigned Map::GetCols() const
  {
    return cols;
  }

  const unsigned Map::GetRows() const
  {
    return rows;
  }

  unsigned int Map::GetTileCount() {
    return static_cast<unsigned int>(tileMetas.size());
  }

  std::shared_ptr<TileMeta> Map::GetTileMeta(unsigned int tileGid) {
    if (tileGid < 0 || tileGid >= tileMetas.size()) {
      return tileMetas[0];
    }

    return tileMetas[tileGid];
  }

  std::shared_ptr<Tileset> Map::GetTileset(const std::string& name) {
    if (tilesets.find(name) == tilesets.end()) {
      return nullptr;
    }

    return tilesets[name];
  }

  std::shared_ptr<Tileset> Map::GetTileset(unsigned int tileGid) {
    if (tileToTilesetMap.size() <= tileGid) {
      return nullptr;
    }

    return tileToTilesetMap[tileGid];
  }

  void Map::SetTileset(const std::shared_ptr<Tileset>& tileset, const std::shared_ptr<TileMeta>& tileMeta) {
    auto tileGid = tileMeta->gid;

    if (tileToTilesetMap.size() <= tileGid) {
      tileToTilesetMap.resize(tileGid + 1);
      tileMetas.resize(tileGid + 1);
    }

    tileMetas[tileGid] = tileMeta;
    tileToTilesetMap[tileGid] = tileset;
    tilesets[tileset->name] = tileset;
  }

  std::size_t Map::GetLayerCount() const {
    return layers.size();
  }

  Map::Layer& Map::GetLayer(size_t index) {
    return layers.at(index);
  }

  Map::Layer& Map::AddLayer() {
    layers.push_back(std::move(Layer(cols, rows)));

    return layers[layers.size() - 1];
  }

  // todo: move to layer?
  // may require reference to map as tilemeta + tile size is used
  bool Map::CanMoveTo(float x, float y, int layerIndex) {
    auto& layer = GetLayer(layerIndex);
    auto& tile = layer.GetTile(x, y);

    // get decimal part
    float tileX;
    float tileY;
    auto testPosition = sf::Vector2f(
      std::modf(x, &tileX),
      std::modf(y, &tileY)
    );

    // get positive coords
    if (testPosition.x < 0) {
      testPosition.x += 1;
    }
    if (testPosition.y < 0) {
      testPosition.y += 1;
    }

    // convert to iso pixels
    testPosition.x *= tileWidth / 2;
    testPosition.y *= tileHeight;

    if (tile.Intersects(*this, testPosition.x, testPosition.y)) {
      return false;
    }

    testPosition.x += tileX * tileWidth / 2;
    testPosition.y += tileY * tileHeight;

    // todo: use a spatial map for increased performance
    for (auto& tileObject : layer.GetTileObjects()) {
      if (tileObject.Intersects(*this, testPosition.x, testPosition.y)) {
        return false;
      }
    }

    return true;
  }

  void Map::RemoveSprites(SceneBase& scene) {
    for (auto& layer : layers) {
      for (auto& tileObject : layer.tileObjects) {
        auto spriteProxy = tileObject.GetWorldSprite();

        if (spriteProxy) {
          scene.RemoveSprite(spriteProxy);
        }
      }
    }
  }

  static Tile nullTile = Tile(0);

  Map::Layer::Layer(unsigned cols, unsigned rows) {
    this->cols = cols;
    this->rows = rows;
    tiles.resize(cols * rows, nullTile);
  }

  Tile& Map::Layer::GetTile(int x, int y)
  {
    if (x < 0 || y < 0 || x >= (int)cols || y >= (int)rows) {
      // reset nullTile as it may have been mutated
      nullTile = Tile(0);
      return nullTile;
    }

    return tiles[y * cols + x];
  }

  Tile& Map::Layer::GetTile(float x, float y)
  {
    return GetTile(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)));
  }

  Tile& Map::Layer::SetTile(int x, int y, Tile tile)
  {
    if (x < 0 || y < 0 || x >= (int)cols || y >= (int)rows) {
      // reset nullTile as it may have been mutated
      nullTile = Tile(0);
      return nullTile;
    }

    return tiles[y * cols + x] = tile;
  }

  Tile& Map::Layer::SetTile(int x, int y, unsigned int gid)
  {
    return SetTile(x, y, Tile(gid));
  }

  Tile& Map::Layer::SetTile(float x, float y, unsigned int gid)
  {
    return SetTile(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)), gid);
  }

  std::optional<std::reference_wrapper<TileObject>> Map::Layer::GetTileObject(unsigned int id) {
    auto iterEnd = tileObjects.end();
    auto iter = std::find_if(tileObjects.begin(), tileObjects.end(), [id](TileObject& tileObject) { return tileObject.id == id; });

    if (iter == iterEnd) {
      return {};
    }

    return *iter;
  }

  std::optional<std::reference_wrapper<TileObject>> Map::Layer::GetTileObject(const std::string& name) {
    auto iterEnd = tileObjects.end();
    auto iter = std::find_if(tileObjects.begin(), tileObjects.end(), [name](TileObject& tileObject) { return tileObject.name == name; });

    if (iter == iterEnd) {
      return {};
    }

    return *iter;
  }

  const std::vector<TileObject>& Map::Layer::GetTileObjects() {
    return tileObjects;
  }

  void Map::Layer::AddTileObject(TileObject tileObject) {
    tileObjects.push_back(tileObject);
    spritesForAddition.push_back(tileObject.GetWorldSprite());
  }

  std::optional<std::reference_wrapper<ShapeObject>> Map::Layer::GetShapeObject(unsigned int id) {
    auto iterEnd = shapeObjects.end();
    auto iter = std::find_if(shapeObjects.begin(), shapeObjects.end(), [id](ShapeObject& shapeObject) { return shapeObject.id == id; });

    if (iter == iterEnd) {
      return {};
    }

    return *iter;
  }

  std::optional<std::reference_wrapper<ShapeObject>> Map::Layer::GetShapeObject(const std::string& name) {
    auto iterEnd = shapeObjects.end();
    auto iter = std::find_if(shapeObjects.begin(), shapeObjects.end(), [name](ShapeObject& shapeObject) { return shapeObject.name == name; });

    if (iter == iterEnd) {
      return {};
    }

    return *iter;
  }

  const std::vector<ShapeObject>& Map::Layer::GetShapeObjects() {
    return shapeObjects;
  }

  void Map::Layer::AddShapeObject(ShapeObject shapeObject) {
    shapeObjects.push_back(std::move(shapeObject));
  }
}
