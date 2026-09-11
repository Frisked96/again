#pragma once
#include <termios.h>

namespace Engine {

enum class Key {
  None,
  Up,
  Down,
  Left,
  Right,
  Quit,
  Unknown
};

class Terminal {
private:
  struct termios orig_termios;
  bool raw_mode_enabled{false};

public:
  Terminal();
  ~Terminal();

  // Prevent copying
  Terminal(const Terminal &) = delete;
  Terminal &operator=(const Terminal &) = delete;

  void enable_raw_mode();
  void disable_raw_mode();

  Key read_key();

  static void clear_screen();
  static void hide_cursor();
  static void show_cursor();
};

} // namespace Engine
