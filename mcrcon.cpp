//
// Created by mitsuha on 2022/5/3.
//

#include "mcrcon.h"

#include "spdlog/spdlog.h"

mcrcon::mcrcon(std::string _addr, ulong _port, std::string _password)
    : addr(std::move(_addr)),
      port(_port),
      password(std::move(_password)),
      status(false) {
  if (!connect()) {
    spdlog::error("connect error");
    return;
  }
  if (!auth()) {
    spdlog::error("auth error");
    return;
  }

  spdlog::info("logged in");

  status = true;
}

bool mcrcon::connect() {
  spdlog::info("server: {}:{}", addr, port);

  conn.connect(sockpp::inet_address(addr, (in_port_t)port));
  if (!conn) {
    spdlog::error("error connecting to server at {}\n\t{}",
                  sockpp::inet_address(addr, port).to_string(),
                  conn.last_error_str());

    return false;
  }

  return true;
}

bool mcrcon::auth() {
  auto packet = build_packet(rcon_command::AUTHENTICATE, password);
  if (packet == nullptr) {
    spdlog::error("build_packet error");

    return false;
  }

  if (!send_packet(packet)) {
    spdlog::error("send_packet error");

    return false;
  }

  auto response = recv_packet();
  if (!response) {
    spdlog::error("recv_packet error");

    return false;
  }
  assert(response->cmd == rcon_command::AUTH_RESPONSE);

  bool ret = false;

  if (response->id != -1)
    ret = true;
  else
    spdlog::error("password error");

  return ret;
}

std::shared_ptr<mcrcon::rc_packet> mcrcon::send_command(
    const std::string& command) {
  auto packet = build_packet(rcon_command::EXEC_COMMAND, command);
  if (packet == nullptr) {
    spdlog::error("build_packet error");

    return nullptr;
  }

  if (!send_packet(packet)) {
    spdlog::error("send_packet error");

    return nullptr;
  }

  auto response = recv_packet();
  if (!response) {
    spdlog::error("recv_packet error");

    return nullptr;
  }
  assert(response->cmd == rcon_command::VALUE_RESPONSE);

  return response;
}

void mcrcon::print_packet(const std::shared_ptr<rc_packet>& packet) {
  int i = 0;
  int def_color = 0;

  for (; packet->data[i] != 0; ++i) {
    if (packet->data[i] == 0x0A)
      print_color(def_color);
    else if (packet->data[i] == 0xc2 && packet->data[i + 1] == 0xa7) {
      print_color(packet->data[i += 2]);
      continue;
    }
    putchar(packet->data[i]);
  }
  print_color(def_color);

  if (packet->data[i - 1] != 10 && packet->data[i - 1] != 13) putchar('\n');
}

mcrcon::~mcrcon() {
  conn.close();
  status = false;
}

std::shared_ptr<mcrcon::rc_packet> mcrcon::build_packet(rcon_command command,
                                                        std::string data) {
  auto size = sizeof(uint) * 2 + data.size() + 2;
  std::shared_ptr<rc_packet> packet(
      (rc_packet*)new u_char[size + sizeof(uint)]{},
      [](rc_packet* packet) { delete[] packet; });
  if (packet == nullptr) {
    spdlog::error("allocate memory error");

    return nullptr;
  }

  packet->size = size;
  packet->id = 0xBADC0DE;
  packet->cmd = command;
  memcpy(packet->data, data.data(), data.size());
  packet->data[data.size()] = 0x00;

  return packet;
}

bool mcrcon::send_packet(const std::shared_ptr<rc_packet>& packet) {
  auto ret = conn.write_n(packet.get(), packet->size + sizeof(int));
  if (ret != packet->size + sizeof(int)) {
    spdlog::error("size error sending package, size:{}", ret);

    return false;
  }
  spdlog::debug("sent {} bytes", ret);

  return true;
}

void mcrcon::print_color(u_char color) {
  const static char* colors[] = {
      "\033[0;30m",   /* 00 BLACK     0x30 */
      "\033[0;34m",   /* 01 BLUE      0x31 */
      "\033[0;32m",   /* 02 GREEN     0x32 */
      "\033[0;36m",   /* 03 CYAN      0x33 */
      "\033[0;31m",   /* 04 RED       0x34 */
      "\033[0;35m",   /* 05 PURPLE    0x35 */
      "\033[0;33m",   /* 06 GOLD      0x36 */
      "\033[0;37m",   /* 07 GREY      0x37 */
      "\033[0;1;30m", /* 08 DGREY     0x38 */
      "\033[0;1;34m", /* 09 LBLUE     0x39 */
      "\033[0;1;32m", /* 10 LGREEN    0x61 */
      "\033[0;1;36m", /* 11 LCYAN     0x62 */
      "\033[0;1;31m", /* 12 LRED      0x63 */
      "\033[0;1;35m", /* 13 LPURPLE   0x64 */
      "\033[0;1;33m", /* 14 YELLOW    0x65 */
      "\033[0;1;37m", /* 15 WHITE     0x66 */
      "\033[4m"       /* 16 UNDERLINE 0x6e */
  };

  /* 0x72: 'r' */
  if (color == 0 || color == 0x72)
    fputs("\033[0m", stdout); /* CANCEL COLOR */
  else {
    if (color >= 0x61 && color <= 0x66)
      color -= 0x57;
    else if (color >= 0x30 && color <= 0x39)
      color -= 0x30;
    else if (color == 0x6e)
      color = 16; /* 0x6e: 'n' */
    else
      return;

    fputs(colors[color], stdout);
  }
}

std::shared_ptr<mcrcon::rc_packet> mcrcon::recv_packet() {
  uint packet_size = 0;

  auto ret = conn.read_n(&packet_size, sizeof(uint));
  if (ret != sizeof(uint)) {
    spdlog::error("size error receiving package size, size:{}", ret);

    return nullptr;
  }

  std::shared_ptr<rc_packet> packet(
      (rc_packet*)new u_char[packet_size + sizeof(uint)]{},
      [](rc_packet* packet) { delete[] packet; });

  packet->size = packet_size;
  ret = conn.read_n(&packet->id, packet_size);
  spdlog::debug("received {} bytes", ret + sizeof(uint));

  return packet;
}
