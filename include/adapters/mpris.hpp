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

    bool operator==(song other) {
      return title == other.get_title() && album == other.get_album() && artist == other.get_artist();
    }

    bool operator!=(song other) {
      return !(*this == other);
    }

    // TODO: Macro ???
    string get_title() {
      return title;
    }
    string get_album() {
      return album;
    }
    string get_artist() {
      return artist;
    }

    chrono::microseconds get_length() {
      return length;
    }

   private:
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
    string get_formatted_elapsed();
    string get_formatted_total();
    bool random() {
      return true;
    }
    bool repeat() {
      return true;
    }
    bool single() {
      return true;
    }
  };

  class connection {
   public:
       connection(const logger& m_log, string player) : player(player), m_log(m_log){
           player_proxy = create_player_proxy();
           mpris_proxy = create_mpris_proxy();
       };
    song get_current_song();
    void pause_play();
    void seek(int change);
    void set_random(bool mode);
    std::map<std::string, std::string> get_metadata();
    void play();
    void pause();
    void stop();
    void prev();
    void next();
    bool connected();
    bool has_event();
    string get_loop_status();
    song get_song();
    std::unique_ptr<status> get_status();
    string get_formatted_elapsed();
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
