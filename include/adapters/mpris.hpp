#pragma once

#include <mpris.h>
#include <common.hpp>
#include <components/logger.hpp>

#include <chrono>
namespace chrono = std::chrono;
using namespace std::literals::chrono_literals;

POLYBAR_NS

namespace mpris {

  template <typename T>
  using deleted_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

  class song {
   public:
    song() : song("", "", "", 0us) {}
    song(string title, string album, string artist, chrono::microseconds length)
      : title(title), album(album), artist(artist), length(length) {}

    bool operator == (const song &other) {
      return title == other.title && album == other.album && artist == other.artist;
    }

    bool operator != (const song &other) {
      return !(*this == other);
    }

    string title;
    string album;
    string artist;
    chrono::microseconds length;
  };

  class status {
   public:
    int position = -1;
    bool shuffle = false;
    string loop_status = "";
    string playback_status = "";
  };

  class connection {
   public:
    connection(const logger& m_log, string player) : player(player), m_log(m_log) {
      player_proxy = create_player_proxy();
      mpris_proxy = create_mpris_proxy();
    };

    void play();
    void pause_play();
    void pause();
    void stop();
    void prev();
    void next();
    void seek(int change);

    song get_current_song();
    std::map<std::string, std::string> get_metadata();
    string get_loop_status();
    bool get_shuffle();
    song get_song();
    std::unique_ptr<status> get_status();
    chrono::microseconds get_elapsed();
    string get_formatted_elapsed();

    bool connected();
    bool has_event();

    void set_loop_status(const string &loop_status);
    void set_shuffle(bool shuffle);

    static string duration_to_string(const chrono::microseconds &);

   private:
    std::string player;
    std::string get(std::string property);
    string get_playback_status();
    shared_ptr<PolybarMediaPlayer2Player> get_object();
    shared_ptr<PolybarMediaPlayer2> get_mpris_proxy();
    const logger& m_log;
    shared_ptr<PolybarMediaPlayer2Player> player_proxy;
    shared_ptr<PolybarMediaPlayer2> mpris_proxy;
    shared_ptr<PolybarMediaPlayer2Player> create_player_proxy();
    shared_ptr<PolybarMediaPlayer2> create_mpris_proxy();
    int player_proxy_count = 0;
    int mpris_proxy_count = 0;
  };
}

POLYBAR_NS_END
