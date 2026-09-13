#pragma once
#include <string_view>
#include <termios.h>

namespace Engine {

enum class KeyCode {
  None,
  Char,
  Up,
  Down,
  Left,
  Right,
  Enter,
  Escape,
  Space,
  Timeout
};

struct RawKey {
  KeyCode code{KeyCode::None};
  char ch{0};
};

class Terminal {
private:
  struct termios orig_termios;
  bool raw_mode_enabled{false};

  static void setup_signals();
  static void signal_handler(int signum);

public:
  Terminal();
  ~Terminal();

  // Prevent copying
  Terminal(const Terminal &) = delete;
  Terminal &operator=(const Terminal &) = delete;

  void enable_raw_mode();
  void disable_raw_mode();

  // Non-blocking read with timeout in milliseconds (default 1500 ms for hybrid time).
  // If timeout_ms < 0, blocks indefinitely.
  RawKey read_key(int timeout_ms = 1500);

  // Output encapsulation
  void present(std::string_view frame);
  void write(std::string_view text);

  static void clear_screen();
  static void hide_cursor();
  static void show_cursor();
  static void restore_terminal();
  static bool get_size(int &rows, int &cols);
};

} // namespace Engine
