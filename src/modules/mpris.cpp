#include "modules/mpris.hpp"
#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "utils/factory.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

using namespace mpris;

namespace modules {
  template class module<mpris_module>;

  mpris_module::mpris_module(const bar_settings& bar, string name_) : event_module<mpris_module>(bar, move(name_)) {
    m_player = m_conf.get(name(), "player", m_player);
    m_synctime = m_conf.get(name(), "interval", m_synctime);

    // Add formats and elements {{{

    m_formatter->add(FORMAT_ONLINE, TAG_LABEL_SONG,
        {TAG_BAR_PROGRESS, TAG_TOGGLE, TAG_TOGGLE_STOP, TAG_LABEL_SONG, TAG_LABEL_TIME, TAG_ICON_RANDOM,
            TAG_ICON_PREV, TAG_ICON_STOP, TAG_ICON_PLAY, TAG_ICON_PAUSE,
            TAG_ICON_NEXT, TAG_ICON_SEEKB, TAG_ICON_SEEKF, TAG_ICON_LOOP});

    m_formatter->add(FORMAT_OFFLINE, "", {TAG_LABEL_OFFLINE});

    m_icons = factory_util::shared<iconset>();

    if (m_formatter->has(TAG_ICON_PLAY) || m_formatter->has(TAG_TOGGLE) || m_formatter->has(TAG_TOGGLE_STOP)) {
      m_icons->add("play", load_icon(m_conf, name(), TAG_ICON_PLAY));
    }
    if (m_formatter->has(TAG_ICON_PAUSE) || m_formatter->has(TAG_TOGGLE)) {
      m_icons->add("pause", load_icon(m_conf, name(), TAG_ICON_PAUSE));
    }
    if (m_formatter->has(TAG_ICON_STOP) || m_formatter->has(TAG_TOGGLE_STOP)) {
      m_icons->add("stop", load_icon(m_conf, name(), TAG_ICON_STOP));
    }
    if (m_formatter->has(TAG_ICON_PREV)) {
      m_icons->add("prev", load_icon(m_conf, name(), TAG_ICON_PREV));
    }
    if (m_formatter->has(TAG_ICON_NEXT)) {
      m_icons->add("next", load_icon(m_conf, name(), TAG_ICON_NEXT));
    }
    if (m_formatter->has(TAG_ICON_SEEKB)) {
      m_icons->add("seekb", load_icon(m_conf, name(), TAG_ICON_SEEKB));
    }
    if (m_formatter->has(TAG_ICON_SEEKF)) {
      m_icons->add("seekf", load_icon(m_conf, name(), TAG_ICON_SEEKF));
    }
    if (m_formatter->has(TAG_ICON_RANDOM)) {
      m_icons->add("random", load_icon(m_conf, name(), "<icon-random>"));
      m_icons->add("no-random", load_icon(m_conf, name(), "<icon-no-random>"));
    }
    if (m_formatter->has(TAG_ICON_LOOP)) {
      m_icons->add("loop-none", load_icon(m_conf, name(), "<icon-loop-none>"));
      m_icons->add("loop-track", load_icon(m_conf, name(), "<icon-loop-track>"));
      m_icons->add("loop-playlist", load_icon(m_conf, name(), "<icon-loop-playlist>"));
    }

    if (m_formatter->has(TAG_LABEL_SONG)) {
      m_label_song = load_optional_label(m_conf, name(), TAG_LABEL_SONG, "%artist% - %title%");
    }
    if (m_formatter->has(TAG_LABEL_TIME)) {
      m_label_time = load_optional_label(m_conf, name(), TAG_LABEL_TIME, "%elapsed% / %total%");
    }
    if (m_formatter->has(TAG_LABEL_OFFLINE, FORMAT_OFFLINE)) {
      m_label_offline = load_label(m_conf, name(), TAG_LABEL_OFFLINE);
    }
    if (m_formatter->has(TAG_BAR_PROGRESS)) {
      m_bar_progress = load_progressbar(m_bar, m_conf, name(), TAG_BAR_PROGRESS);
    }

    // }}}

    m_lastsync = chrono::system_clock::now();
    m_connection = factory_util::unique<mpris::connection>(m_log, m_player);
  }

  void mpris_module::idle() {
    sleep(100ms);
  }

  inline bool mpris_module::connected() const {
    return m_connection->connected();
  }

  bool mpris_module::has_event() {
    if (!connected() && m_status == nullptr) {
      return false;
    } else if (!connected()) {
      return true;
    } else if (m_status == nullptr) {
      return true;
    }

    auto new_song = m_connection->get_song();

    if (m_song != new_song) {
      return true;
    }

    auto new_status = m_connection->get_status();

    if (new_status == nullptr ||
        m_status->playback_status != new_status->playback_status ||
        m_status->loop_status != new_status->loop_status ||
        m_status->shuffle != new_status->shuffle)
    {
      return true;
    }

    if ((m_label_time || m_bar_progress) && m_connection->connected()) {
      auto now = chrono::system_clock::now();
      auto diff = now - m_lastsync;

      if (chrono::duration_cast<chrono::milliseconds>(diff).count() > m_synctime * 1000) {
        m_lastsync = now;
        return true;
      }
    }

    return false;
  }

  bool mpris_module::update() {
    if (!connected() && m_status == nullptr) {
      return false;
    } else if (!m_connection->connected()) {
      m_status = nullptr;
      return true;
    }

    string artist;
    string album;
    string title;
    string date;
    string elapsed_str;
    string total_str;


    elapsed_str = m_connection->get_formatted_elapsed();
    m_song = m_connection->get_song();
    total_str = mpris::connection::duration_to_string(m_song.length);
    m_status = m_connection->get_status();

    title = m_song.title;
    artist = m_song.artist;
    album = m_song.album;

    if (m_label_song) {
      m_label_song->reset_tokens();
      m_label_song->replace_token("%artist%", !artist.empty() ? artist : "untitled artist");
      m_label_song->replace_token("%album%", !album.empty() ? album : "untitled album");
      m_label_song->replace_token("%title%", !title.empty() ? title : "untitled track");
    }

    if (m_label_time) {
      m_label_time->reset_tokens();
      m_label_time->replace_token("%elapsed%", elapsed_str);
      m_label_time->replace_token("%total%", total_str);
    }

    return true;
  }

  string mpris_module::get_format() const {
    return connected() ? FORMAT_ONLINE : FORMAT_OFFLINE;
  }

  bool mpris_module::build(builder* builder, const string& tag) const {
    bool is_playing = m_status && m_status->playback_status == "Playing";
    bool is_paused = m_status && m_status->playback_status == "Paused";
    bool is_stopped = m_status && m_status->playback_status == "Stopped";

    if (tag == TAG_LABEL_SONG && !is_stopped) {
      builder->node(m_label_song);
    } else if (tag == TAG_LABEL_TIME && !is_stopped) {
      builder->node(m_label_time);
    } else if (tag == TAG_BAR_PROGRESS && !is_stopped) {
      auto elapsed_percent = 100 * m_connection->get_elapsed() / m_song.length;
      builder->node(m_bar_progress->output(elapsed_percent));
    } else if (tag == TAG_LABEL_OFFLINE) {
      builder->node(m_label_offline);
    } else if (tag == TAG_ICON_RANDOM) {
      string shuffle;
      if (!m_status || !m_status->shuffle) {
        shuffle = "no-random";
      } else {
        shuffle = "random";
      }
      builder->cmd(mousebtn::LEFT, EVENT_RANDOM, m_icons->get(shuffle));
    } else if (tag == TAG_ICON_LOOP) {
      string loop_status;
      if (!m_status || m_status->loop_status == "None") {
        loop_status = "loop-none";
      } else if (m_status->loop_status == "Track") {
        loop_status = "loop-track";
      } else if (m_status->loop_status == "Playlist") {
        loop_status = "loop-playlist";
      } else {
        loop_status = "loop-none";
      }
      builder->cmd(mousebtn::LEFT, EVENT_NEXT_LOOP_MODE, m_icons->get(loop_status));
    } else if (tag == TAG_ICON_PREV) {
      builder->cmd(mousebtn::LEFT, EVENT_PREV, m_icons->get("prev"));
    } else if ((tag == TAG_ICON_STOP || tag == TAG_TOGGLE_STOP) && (is_playing || is_paused)) {
      builder->cmd(mousebtn::LEFT, EVENT_STOP, m_icons->get("stop"));
    } else if ((tag == TAG_ICON_PAUSE || tag == TAG_TOGGLE) && is_playing) {
      builder->cmd(mousebtn::LEFT, EVENT_PAUSE, m_icons->get("pause"));
    } else if ((tag == TAG_ICON_PLAY || tag == TAG_TOGGLE || tag == TAG_TOGGLE_STOP) && !is_playing) {
      builder->cmd(mousebtn::LEFT, EVENT_PLAY, m_icons->get("play"));
    } else if (tag == TAG_ICON_NEXT) {
      builder->cmd(mousebtn::LEFT, EVENT_NEXT, m_icons->get("next"));
    } else if (tag == TAG_ICON_SEEKB) {
      builder->cmd(mousebtn::LEFT, EVENT_SEEK + "-5"s, m_icons->get("seekb"));
    } else if (tag == TAG_ICON_SEEKF) {
      builder->cmd(mousebtn::LEFT, EVENT_SEEK + "+5"s, m_icons->get("seekf"));
    } else {
      return false;
    }

    return true;
  }

  bool mpris_module::input(string&& cmd) {
    if (cmd == EVENT_PLAY) {
      m_connection->play();
    } else if (cmd == EVENT_PAUSE) {
      m_connection->pause();
    } else if (cmd == EVENT_STOP) {
      m_connection->stop();
    } else if (cmd == EVENT_PREV) {
      m_connection->prev();
    } else if (cmd == EVENT_NEXT) {
      m_connection->next();
    } else if (cmd == EVENT_RANDOM) {
      if (m_status) {
        m_connection->set_shuffle(!m_status->shuffle);
      }
    } else if (cmd == EVENT_NEXT_LOOP_MODE) {
      if (!m_status || m_status->loop_status == "None" || m_status->loop_status == "") {
        m_connection->set_loop_status("Track");
      } else if (m_status->loop_status == "Track") {
        m_connection->set_loop_status("Playlist");
      } else if (m_status->loop_status == "Playlist") {
        m_connection->set_loop_status("None");
      }
    } else {
      return false;
    }

    return true;
  }
}

POLYBAR_NS_END
