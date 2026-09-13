#pragma once
#include <string>
#include <string_view>
#include <utility>

namespace Engine {

class MessageLog {
private:
  std::string current_message;

public:
  MessageLog() = default;
  explicit MessageLog(std::string initial_message)
      : current_message(std::move(initial_message)) {}

  void set(std::string msg) {
    current_message = std::move(msg);
  }

  void clear() {
    current_message.clear();
  }

  [[nodiscard]] std::string_view get() const noexcept {
    return current_message;
  }

  [[nodiscard]] bool empty() const noexcept {
    return current_message.empty();
  }
};

} // namespace Engine
