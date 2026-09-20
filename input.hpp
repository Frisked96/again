#pragma once
#include "terminal.hpp"

namespace Engine {

enum class ActionType {
  None,
  Move,
  Interact,
  Harvest,
  Wait,
  Quit
};

struct Action {
  ActionType type{ActionType::None};
  int dx{0};
  int dy{0};
};

class InputHandler {
public:
  static Action map_key_to_action(RawKey key) noexcept {
    switch (key.code) {
    case KeyCode::Up:
      return {ActionType::Move, 0, -1};
    case KeyCode::Down:
      return {ActionType::Move, 0, 1};
    case KeyCode::Left:
      return {ActionType::Move, -1, 0};
    case KeyCode::Right:
      return {ActionType::Move, 1, 0};
    case KeyCode::Space:
      return {ActionType::Interact, 0, 0};
    case KeyCode::Timeout:
      return {ActionType::Wait, 0, 0};
    case KeyCode::Escape:
      return {ActionType::Quit, 0, 0};
    case KeyCode::Char:
      switch (key.ch) {
      case 'w': case 'W': case 'k': case 'K':
        return {ActionType::Move, 0, -1};
      case 's': case 'S': case 'j': case 'J':
        return {ActionType::Move, 0, 1};
      case 'a': case 'A': case 'h': case 'H':
        return {ActionType::Move, -1, 0};
      case 'd': case 'D': case 'l': case 'L':
        return {ActionType::Move, 1, 0};
      case 'e': case 'E':
        return {ActionType::Interact, 0, 0};
      case '>': case '<':
        return {ActionType::Interact, 0, 0};
      case 'q': case 'Q':
        return {ActionType::Quit, 0, 0};
      case '.':
        return {ActionType::Wait, 0, 0};
      default:
        return {ActionType::None, 0, 0};
      }
    default:
      return {ActionType::None, 0, 0};
    }
  }
};

} // namespace Engine
