namespace voxel {
struct Block {
  constexpr Block(float x, float y, float z, char g) : bx(x), by(y), bz(z), glyph(g) {}
  float bx;
  float by;
  float bz;
  char glyph;
};
constexpr Block Wall(1.0f,1.0f,1.0f, '.');
} // namespace voxel