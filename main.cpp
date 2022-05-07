

#include <readline/history.h>
#include <readline/readline.h>
#include <string.h>

#include <iostream>

#include "argparse/argparse.hpp"
#include "mcrcon.h"
#include "spdlog/spdlog.h"

std::string trim(const std::string& s) {
  if (s.length() == 0) return std::string{s};

  return s.substr(s.find_first_not_of(' '),
                  s.find_last_not_of(' ') - s.find_first_not_of(' ') + 1);
}

char* dupstr(const char* src, const size_t len) {
  char* dst = (char*)malloc(len + 1);
  if (dst == nullptr) return nullptr;
  strncpy(dst, src, len);
  dst[len] = 0;
  return dst;
}

std::vector<std::string> split(const std::string& str,
                               const std::string& pattern) {
  std::vector<std::string> res;
  if (str == "") return res;
  //在字符串末尾也加入分隔符，方便截取最后一段
  std::string strs = str + pattern;
  size_t pos = strs.find(pattern);

  while (pos != strs.npos) {
    std::string temp = strs.substr(0, pos);
    res.push_back(temp);
    //去掉已分割的字符串,在剩下的字符串中进行分割
    strs = strs.substr(pos + 1, strs.size());
    pos = strs.find(pattern);
  }

  return res;
}

void process_help_packet(std::vector<std::string>& command_map,
                         const std::shared_ptr<mcrcon::rc_packet>& packet) {
  auto items = split((std::string)(const char*)packet->data, "\n");
  bool is_first = true;

  for (const auto& item : items) {
    if (!is_first) {
      auto start_pos = item.find('/');
      auto end_pos = item.find(':');
      if (start_pos != std::string::npos && end_pos != std::string::npos) {
        ++start_pos;
        command_map.push_back(
            std::move(item.substr(start_pos, end_pos - start_pos)));
      } else {
        // spdlog::warn("error processing help");
      }
    } else
      is_first = false;
  }
}

std::vector<std::string> command_map;
char* command_generator(const char* text, int state) {
  static size_t list_index, len;

  if (!state) {
    list_index = 0;
    len = strlen(text);
  }

  for (; list_index < command_map.size();) {
    const auto& name = command_map[list_index];
    list_index++;

    if (strncmp(name.c_str(), text, len) == 0)
      return dupstr(name.c_str(), name.size());
  }

  return nullptr;
}

char** mccron_completion(const char* text, int start, int end) {
  char** matches = nullptr;

  //  if (start == 0)
  matches = rl_completion_matches(text, command_generator);

  return matches;
}

int main(int argc, char* argv[]) {
  spdlog::info("mcrconpp started");
  argparse::ArgumentParser app{"mcrconpp"};
  app.add_description("Minecraft rcon client for Linux");

  app.add_argument("-a", "--addr")
      .help("rcon server address")
      .default_value(std::string{"127.0.0.1"});

  app.add_argument("-p", "--port")
      .help("rcon server port")
      .default_value(25575ul);

  app.add_argument("-pw", "--password")
      .help("rcon server password")
      .default_value(std::string{""});

  try {
    app.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << app;

    return 1;
  }

  auto param_addr = app.get<std::string>("--addr");
  auto param_port = app.get<ulong>("--port");
  auto param_password = app.get<std::string>("--password");

  mcrcon client(param_addr, param_port, param_password);
  std::string prompt_line = param_addr;
  prompt_line += ':';
  prompt_line += std::to_string(param_port);
  prompt_line += " > ";

  rl_attempted_completion_function = mccron_completion;

  spdlog::info("client running, press \"exit\" to exit");

  std::vector<std::shared_ptr<mcrcon::rc_packet>> help_pages;
  auto packet_help_count = client.send_command("help");
  std::string str_help_count = (const char*)packet_help_count->data;
  auto count_start_pos = str_help_count.find('/');
  auto count_end_pos = str_help_count.find(')');
  if (count_start_pos != std::string::npos &&
      count_end_pos != std::string::npos) {
    ++count_start_pos;
    auto str_count = std::stoul(str_help_count.substr(
        count_start_pos, count_end_pos - count_start_pos));

    for (int i = 1; i <= str_count; ++i) {
      std::string help_command = "help " + std::to_string(i);
      auto packet_help = client.send_command(help_command);
      help_pages.push_back(packet_help);
    }
  } else
    spdlog::warn("error finding the count of help page");

  for (const auto& item : help_pages) {
    // mcrcon::print_packet(item);
    process_help_packet(command_map, item);
  }

  auto player_packet = client.send_command("list");
  mcrcon::print_packet(player_packet);

  std::string command;
  while (auto input = readline(prompt_line.c_str())) {
    add_history(input);
    command = input;
    command = std::move(trim(command));
    free(input);
    if (command == "exit") break;

    auto packet = client.send_command(command);
    mcrcon::print_packet(packet);
  }

  return 0;
}
