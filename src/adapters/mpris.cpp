#include <mpris.h>
#include <string.h>
#include <adapters/mpris.hpp>
#include <common.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

POLYBAR_NS

namespace mpris {

  // https://developer.gnome.org/glib/stable/glib-GVariant.html#g-variant-iter-loop

  shared_ptr<PolybarMediaPlayer2Player> connection::get_object() {
    if (++mpris_proxy_count > 10) {
      mpris_proxy_count = 0;
      player_proxy = create_player_proxy();
    }
    return player_proxy;
  }

  shared_ptr<PolybarMediaPlayer2> connection::get_mpris_proxy() {
    if (++player_proxy_count > 10) {
      player_proxy_count = 0;
      mpris_proxy = create_mpris_proxy();
    }
    return mpris_proxy;
  }

  shared_ptr<PolybarMediaPlayer2Player> connection::create_player_proxy() {
    GError* error = nullptr;

    auto destructor = [&](PolybarMediaPlayer2Player* proxy) { g_object_unref(proxy); };

    auto player_object = "org.mpris.MediaPlayer2." + player;

    auto proxy = polybar_media_player2_player_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, player_object.data(), "/org/mpris/MediaPlayer2", NULL, &error);

    if (error != nullptr) {
      m_log.err("Empty object. Error: %s", error->message);
      g_error_free(error);
      return nullptr;
    }

    return shared_ptr<PolybarMediaPlayer2Player>(proxy, destructor);
  }

  shared_ptr<PolybarMediaPlayer2> connection::create_mpris_proxy() {
    GError* error = nullptr;

    auto player_object = "org.mpris.MediaPlayer2." + player;

    auto destructor = [&](auto* proxy) { g_object_unref(proxy); };

    auto proxy_raw = polybar_media_player2_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, player_object.data(), "/org/mpris/MediaPlayer2", NULL, &error);

    auto proxy = shared_ptr<PolybarMediaPlayer2>(proxy_raw, destructor);

    if (error != nullptr) {
      m_log.err("Empty object. Error: %s", error->message);
      g_error_free(error);
      return nullptr;
    }

    return proxy;
  }

  bool connection::connected() {
    auto entry = polybar_media_player2_get_desktop_entry(get_mpris_proxy().get());
    return entry != nullptr;
  }

  void connection::pause_play() {
    GError* error = nullptr;

    auto object = get_object();

    if (object == nullptr) {
      return;
    }

    polybar_media_player2_player_call_play_pause_sync(object.get(), NULL, &error);

    if (error != nullptr) {
      m_log.err("Empty session bus");
      m_log.err(std::string(error->message));
      g_error_free(error);
    }
  }

  void connection::play() {
    GError* error = nullptr;

    auto object = get_object();

    if (object == nullptr) {
      return;
    }

    polybar_media_player2_player_call_play_sync(object.get(), NULL, &error);

    if (error != nullptr) {
      m_log.err("Empty session bus");
      m_log.err(std::string(error->message));
    }
  }

  void connection::pause() {
    GError* error = nullptr;

    auto object = get_object();

    if (object == nullptr) {
      return;
    }

    polybar_media_player2_player_call_pause_sync(object.get(), NULL, &error);

    if (error != nullptr) {
      m_log.err("Empty session bus");
      m_log.err(std::string(error->message));
      g_error_free(error);
    }
  }

  void connection::prev() {
    GError* error = nullptr;

    auto object = get_object();

    if (object == nullptr) {
      return;
    }

    polybar_media_player2_player_call_previous_sync(object.get(), NULL, &error);

    if (error != nullptr) {
      m_log.err("Empty session bus");
      m_log.err(std::string(error->message));
      g_error_free(error);
    }
  }

  void connection::next() {
    GError* error = nullptr;

    auto object = get_object();

    if (object == nullptr) {
      return;
    }

    polybar_media_player2_player_call_next_sync(object.get(), NULL, &error);

    if (error != nullptr) {
      m_log.err("Empty session bus");
      m_log.err(std::string(error->message));
      g_error_free(error);
    }
  }

  void connection::stop() {
    GError* error = nullptr;

    auto object = get_object();

    if (object == nullptr) {
      return;
    }

    polybar_media_player2_player_call_stop_sync(object.get(), NULL, &error);

    if (error != nullptr) {
      m_log.err("Empty session bus");
      m_log.err(std::string(error->message));
      g_error_free(error);
    }
  }

  string connection::get_loop_status() {
    auto object = get_object();

    if (object == nullptr) {
      return "";
    }

    auto arr = polybar_media_player2_player_get_loop_status(object.get());

    if (arr == nullptr) {
      return "";
    } else {
      return arr;
    }
  }

  string connection::get_playback_status() {
    auto object = get_object();

    if (object == nullptr) {
      return "";
    }

    auto arr = polybar_media_player2_player_get_playback_status(object.get());

    if (arr == nullptr) {
      return "";
    } else {
      return arr;
    }
  }

  bool connection::get_shuffle() {
    auto object = get_object();
    if (!object) return false;

    return polybar_media_player2_player_get_shuffle(object.get());
  }

  song connection::get_song() {
    auto object = get_object();

    if (object == nullptr) {
      return song();
    }

    GVariantIter iter;
    GVariant* value;
    gchar* key;

    string title;
    string album;
    string artist;
    chrono::microseconds length = 0us;

    auto variant = polybar_media_player2_player_get_metadata(object.get());

    if (variant == nullptr) {
      return song();
    }

    auto size = g_variant_iter_init(&iter, variant);

    if (size == 0) {
      return song();
    }

    while (g_variant_iter_loop(&iter, "{sv}", &key, &value)) {
      if (strcmp(key, "xesam:album") == 0) {
        album = g_variant_get_string(value, nullptr);
      } else if (strcmp(key, "xesam:title") == 0) {
        title = g_variant_get_string(value, nullptr);
      } else if (strcmp(key, "xesam:artist") == 0) {
        GVariantIter iter1;
        g_variant_iter_init(&iter1, value);
        auto var = g_variant_iter_next_value(&iter1);
        if (var != nullptr) {
          artist = g_variant_get_string(var, nullptr);
          g_variant_unref(var);
        }
      } else if (strcmp(key, "mpris:length") == 0) {
        length = chrono::microseconds(g_variant_get_int64(value));
      }
    }

    return song(title, album, artist, length);
  }

  unique_ptr<status> connection::get_status() {
    auto object = get_object();

    if (object == nullptr) {
      return nullptr;
    }

    auto loop_status = get_loop_status();
    auto playback_status = get_playback_status();
    auto shuffle = get_shuffle();

    auto st = new status();
    st->loop_status = loop_status;
    st->playback_status = playback_status;
    st->shuffle = shuffle;
    return unique_ptr<status>(st);
  }

  string connection::get_formatted_elapsed() {
    auto object = get_object();
    if (!object) {
      return "N/A"s;
    }

    auto position_us = polybar_media_player2_player_get_position(object.get());
    return connection::duration_to_string(chrono::microseconds(position_us));
  }

  void connection::set_loop_status(const string &loop_status) {
    auto object = get_object();
    if (!object) return;

    polybar_media_player2_player_set_loop_status(object.get(), loop_status.c_str());
  }

  void connection::set_shuffle(bool shuffle) {
    auto object = get_object();
    if (!object) return;

    polybar_media_player2_player_set_shuffle(object.get(), shuffle);
  }

  string connection::duration_to_string(const chrono::microseconds &duration) {
    auto duration_s = chrono::duration_cast<chrono::seconds>(duration);
    auto duration_m = chrono::duration_cast<chrono::minutes>(duration);
    duration_s -= chrono::duration_cast<chrono::seconds>(duration_m);

    std::ostringstream result;
    result << duration_m.count() << ':'
           << std::setfill('0') << std::setw(2) << duration_s.count();
    return result.str();
  }
}

POLYBAR_NS_END
