#include "terminal.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace Engine {

static Terminal *g_active_terminal = nullptr;
static int s_terminal_instance_count = 0;

void Terminal::setup_signals() {
  struct sigaction sa;
  sa.sa_handler = Terminal::signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);
  sigaction(SIGQUIT, &sa, nullptr);

  std::atexit(Terminal::restore_terminal);
}

void Terminal::signal_handler(int signum) {
  Terminal::restore_terminal();
  _exit(128 + signum);
}

void Terminal::restore_terminal() {
  if (g_active_terminal) {
    std::cout << "\033[?1049l\033[?25h" << std::flush;
    g_active_terminal->disable_raw_mode();
  }
}

Terminal::Terminal() {
  g_active_terminal = this;
  if (s_terminal_instance_count == 0) {
    setup_signals();
    enable_raw_mode();
    hide_cursor();
    std::cout << "\033[?1049h\033[2J\033[H" << std::flush;
  }
  ++s_terminal_instance_count;
}

Terminal::~Terminal() {
  --s_terminal_instance_count;
  if (s_terminal_instance_count <= 0) {
    restore_terminal();
    s_terminal_instance_count = 0;
  }
  if (g_active_terminal == this) {
    g_active_terminal = nullptr;
  }
}

void Terminal::enable_raw_mode() {
  if (raw_mode_enabled) {
    return;
  }
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
    return;
  }

  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
  raw.c_iflag &= ~(IXON | ICRNL);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != -1) {
    raw_mode_enabled = true;
  }
}

void Terminal::disable_raw_mode() {
  if (!raw_mode_enabled) {
    return;
  }
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
  raw_mode_enabled = false;
}

RawKey Terminal::read_key(int timeout_ms) {
  struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
  int ret = poll(&pfd, 1, timeout_ms);

  if (ret == 0) {
    return {KeyCode::Timeout, 0};
  }
  if (ret < 0) {
    return {KeyCode::Escape, 0};
  }

  char c = 0;
  ssize_t n = read(STDIN_FILENO, &c, 1);
  if (n <= 0) {
    return {KeyCode::Escape, 0};
  }

  // Handle escape sequences (Arrows)
  if (c == '\033') {
    struct pollfd pfd_seq = {STDIN_FILENO, POLLIN, 0};
    int ret_seq = poll(&pfd_seq, 1, 50);
    if (ret_seq > 0 && (pfd_seq.revents & POLLIN)) {
      char seq[2] = {0, 0};
      if (read(STDIN_FILENO, &seq[0], 1) > 0) {
        if (seq[0] == '[') {
          if (poll(&pfd_seq, 1, 50) > 0 && (pfd_seq.revents & POLLIN)) {
            if (read(STDIN_FILENO, &seq[1], 1) > 0) {
              switch (seq[1]) {
              case 'A':
                return {KeyCode::Up, 0};
              case 'B':
                return {KeyCode::Down, 0};
              case 'C':
                return {KeyCode::Right, 0};
              case 'D':
                return {KeyCode::Left, 0};
              default:
                break;
              }
            }
          }
        }
      }
    }
    return {KeyCode::Escape, '\033'};
  }

  if (c == '\n' || c == '\r') {
    return {KeyCode::Enter, c};
  }
  if (c == 127 || c == 8) {
    return {KeyCode::Backspace, c};
  }
  if (c == ' ') {
    return {KeyCode::Space, ' '};
  }
  if (c == 3 || c == 4) { // Ctrl-C, Ctrl-D
    return {KeyCode::Escape, c};
  }

  return {KeyCode::Char, c};
}

void Terminal::present(std::string_view frame) {
  std::cout << frame << std::flush;
}

void Terminal::write(std::string_view text) {
  std::cout << text;
}

void Terminal::clear_screen() {
  std::cout << "\033[2J\033[H" << std::flush;
}

void Terminal::hide_cursor() {
  std::cout << "\033[?25l" << std::flush;
}

void Terminal::show_cursor() {
  std::cout << "\033[?25h" << std::flush;
}

bool Terminal::get_size(int &rows, int &cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
    rows = ws.ws_row;
    cols = ws.ws_col;
    return true;
  }
  return false;
}

} // namespace Engine
