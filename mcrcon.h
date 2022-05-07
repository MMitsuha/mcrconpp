//
// Created by mitsuha on 2022/5/3.
//

#ifndef MCRCONPP_MCRCON_H
#define MCRCONPP_MCRCON_H
#include <iostream>
#include <memory>

#include "sockpp/socket.h"
#include "sockpp/tcp_connector.h"
#include "sockpp/version.h"

class mcrcon {
 public:
  enum class rcon_command : uint {
    EXEC_COMMAND = 2,
    AUTHENTICATE = 3,
    VALUE_RESPONSE = 0,
    AUTH_RESPONSE = 2
  };

  // rcon packet structure
  typedef struct _rc_packet {
    uint size;
    int id;
    rcon_command cmd;
    u_char data[1];  // extended
  } rc_packet;

  mcrcon() = default;
  explicit mcrcon(std::string _addr = "127.0.0.1", ulong _port = 25575ul,
                  std::string _password = "");

  bool connect();
  bool auth();

  inline bool get_status() const { return status; }

  std::shared_ptr<rc_packet> send_command(const std::string& command);

  static void print_packet(const std::shared_ptr<rc_packet>& packet);

  ~mcrcon();

 private:
  static std::shared_ptr<rc_packet> build_packet(rcon_command command,
                                                 std::string data);

  bool send_packet(const std::shared_ptr<rc_packet>& packet);
  std::shared_ptr<rc_packet> recv_packet();

  static void print_color(u_char color);

  std::string addr{};
  in_port_t port{};
  std::string password{};
  sockpp::socket_initializer sock_init{};
  sockpp::tcp_connector conn{};

  bool status{false};
};

#endif  // MCRCONPP_MCRCON_H
